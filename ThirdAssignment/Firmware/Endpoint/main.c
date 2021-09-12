// Libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <matstat.h>

#include "xtimer.h"
#include "thread.h"
#include "timex.h"     // Utility library for comparing and computing timestamp

#include "periph/gpio.h"

// To get CPU_ID
//#include "cpu_identity.c"

//  Temperature sensor libraries
#include "hts221.h"
#include "hts221_params.h"

// Libraries for implementing Light sensor
#include "random.h"

//Network libraries
//LoRa libraries
#include "fmt.h" // Formatting library for Hex address management
//#include "net/loramac.h"
#include "semtech_loramac.h"

#include "sx127x.h" //SX1276
#include "sx127x_netdev.h"
#include "sx127x_params.h"

//Global variables
#define MAX 1000 //Max elements in Temperature & Light values

//  Temperature sensor
static hts221_t hts221;
int temperature_sample_rate = 15; // 10 m = 600 s for TEMPERATURE (The SAME as FIRST Assignment)
#define MAX_TEMPERATURE 24 // 24°C is the highest temperature the poultry can survive
char temperature_thread_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for Temperature thread
int temperature[MAX]; // Temperature accuracy: ± 0.5 °C, 15 to +40 °C [15, 40]
int temp_idx;

//  Light sensor (Simulated light sensor)
//static isl29020_t isl29020;
int light_sample_rate = 30; // 30 m = 1800 s per Light (The SAME as FIRST Assignment)
#define MIN_LIGHT 2 //2 lux is the light threshold to switch on the lamp
char light_thread_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for BRIGHTNESS thread
int light[MAX]; // It could be "uint8_t" because "light" is always positive, but since -1 is used as error return value then a signed integer is needed
int light_idx;

//Aggregation thread
char aggregation_thread_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for BRIGHTNESS thread

//Output LEDs - board.h File Reference - https://doc.riot-os.org/b-l072z-lrwan1_2include_2board_8h.html
#define BUZZER_LED0_PIN   GPIO_PIN(PORT_B, 5)  // Led0 simultaes BUZZER (wrt First ind. assign.)
#define FAN_LED1_PIN   GPIO_PIN(PORT_B, 6)  // Led1 simulates FAN (wrt First ind. assign.)
#define LAMP_LED2_PIN   GPIO_PIN(PORT_B, 7)  // Led2 simulates LAMP (wrt First ind. assign.)


//LoRa Global variables - constants
static semtech_loramac_t loramac;
static sx127x_t sx127x;

static uint8_t deveui[LORAMAC_DEVEUI_LEN];
static uint8_t appeui[LORAMAC_APPEUI_LEN];
static uint8_t appkey[LORAMAC_APPKEY_LEN];

//LoRa messages
//"device" field is removed since it is already in TTN message
const char message_structure[] = "{\"last_temperature\":\"%d\", \"min_temperature\":\"%ld\", \"max_temperature\":\"%ld\", \"avg_temperature\":\"%ld\", \"last_light\":\"%d, \"min_light\":\"%ld\", \"max_light\":\"%ld\", \"avg_light\":\"%ld\"}";
#define MESSAGE_MAXLEN      (300U)
char message[MESSAGE_MAXLEN];

const char actuator_message_structure[] = "{\"actuator\":\"%s\", \"value\":\"%d\"}";
char actuator_message[MESSAGE_MAXLEN];

//Functions
// Initialization of Sensors
int sensor_initialization(void) {
    int res;

    // Initialize the Temperature sensor
    res = hts221_init(&hts221, &hts221_params[0]);
    if (res != HTS221_OK) {
        printf("Sensor initialization failed \n");
        return res;
    }
    res = hts221_power_on(&hts221);
    if (res != HTS221_OK) {
        printf("Sensor initialization *power on* failed \n");
        return res;
    }
    res = hts221_set_rate(&hts221, hts221.p.rate);
    if (res != HTS221_OK) {
        printf("Sensor *continuous mode* setup failed \n");
        return res;
    }

    // Initialize the Light sensor
    //Empty since now it is a simulated sensor
    return 0; //On success
}

// Initialization of Actuators
int actuator_initialization(void) {
    int res;

    //  Led-0 simulates BUZZER of the 1st assignment
    if ((res = (gpio_init(BUZZER_LED0_PIN, GPIO_OUT))) == -1) {
        printf("Led-0 NOT initialized! \n");
        return res;
    }
    //  Led-1 simulates FAN of the 1st assignment
    if ((res = (gpio_init(FAN_LED1_PIN, GPIO_OUT))) == -1) {
        printf("Led-1 NOT initialized! \n");
        return res;
    }
    //  Led-2 simulates LAMP of the 1st assignment
    if ((res = (gpio_init(LAMP_LED2_PIN, GPIO_OUT))) == -1) {
        printf("Led-2 NOT initialized! \n");
        return res;
    }
    return 0; //on success
}

// LoRa initialization
int lora_initialization(void) {

    printf("\n String DEVEUI: %s \n", DEVEUI); //TODO Debug only
    printf("\n String APPEUI: %s \n", APPEUI); //TODO Debug only
    printf("\n String APPKEY: %s \n", APPKEY); //TODO Debug only

    // Convert identifiers and application key
    //fmt_hex_bytes(deveui, "70B3D57ED0045254");
    //fmt_hex_bytes(appeui, "0000000000000000");
    //fmt_hex_bytes(appkey, "F159FCCD151F4DE8CD0EC6635EB3BD89");

    fmt_hex_bytes(deveui, DEVEUI); // From MAKEFILE variable
    fmt_hex_bytes(appeui, APPEUI); // From MAKEFILE variable
    fmt_hex_bytes(appkey, APPKEY); // From MAKEFILE variable

    // Initialize the radio driver
    sx127x_setup(&sx127x, &sx127x_params[0], 0);
    loramac.netdev = &sx127x.netdev; //Device - Driver association
    loramac.netdev->driver = &sx127x_driver;

    // Initialize the loramac stack
    if (semtech_loramac_init(&loramac) == -1) {
        printf("semtech_loramac_init() failed! \n");
        return (-1);
    };
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    /* Debug only
    u_int8_t tmp1[50];
    semtech_loramac_get_deveui(&loramac, tmp1);
    printf("\n %u %u %u %u %u %u %u %u \n", tmp1[0], tmp1[1], tmp1[2], tmp1[3], tmp1[4], tmp1[5], tmp1[6], tmp1[7]);

    u_int8_t tmp2[50];
    semtech_loramac_get_appeui(&loramac, tmp2);
    printf("\n %u %u %u %u %u %u %u %u \n", tmp2[0], tmp2[1], tmp2[2], tmp2[3], tmp2[4], tmp2[5], tmp2[6], tmp2[7]);

    u_int8_t tmp3[50];
    semtech_loramac_get_appeui(&loramac, tmp3);
    printf("\n %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u \n", tmp3[0], tmp3[1], tmp3[2], tmp3[3], tmp3[4], tmp3[5], tmp3[6], tmp3[7], tmp3[8], tmp3[9], tmp3[10], tmp3[11], tmp3[12], tmp3[13], tmp3[14], tmp3[15]);
    */

    // Use a fast datarate, e.g. BW125/SF7 in EU868
    semtech_loramac_set_dr(&loramac, LORAMAC_DR_5);
    //semtech_loramac_set_dr(&loramac, 5);

    // Start the Over-The-Air Activation (OTAA) procedure to retrieve the
    // generated device address and to get the network and application session
    // keys.

    printf("Starting join procedure \n");
    u_int8_t res = semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA);
    switch (res) {
    case SEMTECH_LORAMAC_JOIN_FAILED:
        printf("semtech_loramac_join() - SEMTECH_LORAMAC_JOIN_FAILED \n");
        return 1;
        break;
    case SEMTECH_LORAMAC_BUSY:
        printf("semtech_loramac_join() - SEMTECH_LORAMAC_BUSY - the MAC is already active (join or tx in progress) \n");
        return 1;
        break;
    case SEMTECH_LORAMAC_ALREADY_JOINED:
        printf("semtech_loramac_join() - SEMTECH_LORAMAC_ALREADY_JOINED - network was already joined \n");
        return 1;
        break;
    default: //No error found, exit switch-case statement
        break;
    }
    printf("Join procedure succeeded \n");
    return 0; // SEMTECH_LORAMAC_JOIN_SUCCEEDED
}

void blink_pin(gpio_t pin, u_int16_t duration) {
    gpio_write(pin, 1); //Switch ON to inform about the start of operation
    //printf("Pin is ON");
    xtimer_sleep(duration);
    gpio_write(pin, 0); //Switch OFF to inform about the end of operation
    //printf("Pin is OFF");
}

//Thread handler for Temperature
void* temperature_thread_handler(void *arg) {
    (void) arg;
    uint8_t res;
    int fan_status;
    temp_idx = 0;

    // Sample continuously
    while (1) {
        int16_t tmp_val = 0;
        // Read temperature value
        if (hts221_read_temperature(&hts221, &tmp_val) != HTS221_OK) {
            printf(" -- failed to read temperature! \n");
            temperature[temp_idx] = 0; // A 0 °C temperature means error, since hts221 carries out  [15, 40] °C temperature
            temp_idx += 1;
        } else {
            //It should send temperature just retrieved over LoRa,
            // but because of aggregation operation Temperature is sent together with Light within a unique message

            //Despite the change in message transmission,
            //the program goes on locally interacting with the environment
            temperature[temp_idx] = tmp_val;
            printf("Raw Integer Temperature: %d °C\n", temperature[temp_idx]); //TODO Debug only

            fan_status = gpio_read(FAN_LED1_PIN); // Led1 acts as the FAN actuator of the First assignment
            if (temperature[temp_idx] >= MAX_TEMPERATURE && fan_status == 0) {
                printf("ROOM TEMPERATURE is greater than threshold! WARNING!!! \n");

                // Blink (ON-OFF) BUZZER for 10 seconds
                blink_pin(BUZZER_LED0_PIN, 10);
                //TODO Lack of communication about Actuators. Add it

                //Switch ON the FAN
                fan_status = 1;
                gpio_write(FAN_LED1_PIN, fan_status);
                // Send data via Lora
                sprintf(actuator_message, actuator_message_structure, "fan", fan_status);
                res = semtech_loramac_send(&loramac, (uint8_t*) actuator_message, strlen(actuator_message));
                if (res != SEMTECH_LORAMAC_TX_DONE) {
                    printf("Cannot send message '%s', returned code: %u\n", actuator_message, res);
                    //return res; //TODO It should be return null
                } else{
                    printf("Message *%s* sent over LoRa successfully \n", actuator_message);
                }

            } else if (temperature[temp_idx] < MAX_TEMPERATURE && fan_status == 1) {
                printf("ROOM TEMPERATURE is less than threshold! TEMPERATURE is OK \n");

                //Switch OFF the FAN
                fan_status = 0;
                gpio_write(FAN_LED1_PIN, fan_status);
                sprintf(actuator_message, actuator_message_structure, "fan", fan_status);
                res = semtech_loramac_send(&loramac, (uint8_t*) actuator_message, strlen(actuator_message));
                if (res != SEMTECH_LORAMAC_TX_DONE) {
                    printf("Cannot send message '%s', returned code: %u\n", actuator_message, res);
                } else{
                    printf("\n Message *%s* sent over LoRa successfully", actuator_message);
                }
            }
            temp_idx += 1;
        }
        xtimer_sleep(temperature_sample_rate);
    }
    return NULL; //Should never be reached
}

//Thread handler for light
void* light_thread_handler(void *arg) {
    (void) arg;
    uint8_t res;
    int lamp_status;
    light_idx = 0;

    // Simulated Light sensor
    uint32_t seed = 29;
    random_init(seed);
    uint32_t min = 0;
    uint32_t max = 1000; // From ISL29020 light sensor range [0, 1000] or [0, 64000]

    // Sample continuously
    while (1) {
        // Read the Light values
        light[light_idx] = random_uint32_range(min, max);
        if (light[light_idx] == -1 /* !=  */) {
            printf("Error while reading Light sensor\n");
            light[light_idx] = -1; // A -1 LUX light means error, since there exist no negative light value (physically impossible)
            light_idx += 1;
        } else {
            //It should send light value just retrieved over LoRa,
            // but because of aggregation operation Light value is sent together with Temperature one within a unique message

            //Despite the change in message transmission,
            //the program goes on locally interacting with the environment

            printf("Light: %d lux \n", light[light_idx]);

            lamp_status = gpio_read(LAMP_LED2_PIN); //Read LAMP status
            if (light[light_idx] <= MIN_LIGHT && lamp_status == 0) {
                printf("LIGHT is greater than threshold! WARNING!!! \n");

                // Switch ON the LAMP
                lamp_status = 1;
                gpio_write(LAMP_LED2_PIN, lamp_status);
                printf("LAMP is ON \n");

                // Send data via Lora
                sprintf(actuator_message, actuator_message_structure, "lamp", lamp_status);
                res = semtech_loramac_send(&loramac, (uint8_t*) actuator_message, strlen(actuator_message));
                if (res != SEMTECH_LORAMAC_TX_DONE) {
                    printf("Cannot send message '%s', returned code: %u\n", actuator_message, res);
                } else{
                    printf("Message *%s* sent over LoRa successfully \n", actuator_message);
                }

            } else if (light[light_idx] > MIN_LIGHT && lamp_status > 0) {
                // Switch OFF the LAMP
                lamp_status = 0;
                gpio_write(LAMP_LED2_PIN, lamp_status);
                printf("ROOM ILLUMINANCE is greater than threshold, LAMP is OFF \n");

                // Send data via Lora
                sprintf(actuator_message, actuator_message_structure, "lamp", lamp_status);
                res = semtech_loramac_send(&loramac, (uint8_t*) actuator_message, strlen(actuator_message));
                if (res != SEMTECH_LORAMAC_TX_DONE) {
                    printf("Cannot send message '%s', returned code: %u\n", actuator_message, res);
                } else {
                    printf("Message *%s* sent over LoRa successfully \n", actuator_message);
                }
            }
            light_idx += 1;
        }
        xtimer_sleep(light_sample_rate);
    }
    return NULL;
}

void* aggregation_thread_handler(void *arg) {
    (void) arg;
    uint8_t res;
    matstat_state_t temperature_state = MATSTAT_STATE_INIT;
    matstat_state_t light_state = MATSTAT_STATE_INIT;
    //int32_t mean;

    while (1) {
        xtimer_sleep(light_sample_rate + 15);

        printf("\n Temperature values till now: ");
        for (int idx = 0; idx < temp_idx; idx++) {
            printf("%d, ", temperature[idx]);
            matstat_add(&temperature_state, temperature[idx]);
        }
        printf("\n");

        printf("Min temperature: %ld \n", temperature_state.min);
        printf("Max temperature: %ld \n", temperature_state.max);
        //mean = matstat_mean(&temperature_state);
        matstat_mean(&temperature_state);
        //printf("Average temperature: %ld \n", mean);
        printf("Average temperature: %ld", temperature_state.mean);

        printf("\n Light values till now: ");
        for (int idx = 0; idx < light_idx; idx++) {
            printf("%d, ", light[idx]);
            matstat_add(&light_state, light[idx]);
        }
        printf("\n");

        printf("Min light: %ld \n", light_state.min);
        printf("Max light: %ld \n", light_state.max);
        //printf("Average light: %ld", matstat_mean(light_state));
        matstat_mean(&light_state);
        printf("Average light: %ld \n", light_state.mean);

        // Send data via Lora
        sprintf(message, message_structure, temperature[temp_idx-1], temperature_state.min, temperature_state.max, temperature_state.mean, light[light_idx-1], light_state.min, light_state.max, light_state.mean);
        res = semtech_loramac_send(&loramac, (uint8_t*) message, strlen(message));
        if (res != SEMTECH_LORAMAC_TX_DONE) {
            printf("Cannot send message '%s', returned code: %u\n", message, res);
        } else {
            printf("\nMessage *%s* sent over LoRa successfully \n", message);
        }
    }
    return NULL;
}

int main(void) {
    printf("\n RIOT application - B-L072Z-LRWAN1 \n");
    printf("LoRa Henhouse \n");

    int res;

    // Input/Sensors initialization
    res = sensor_initialization();
    if (res != 0) {
        printf("Sensor initialization failed with **Error %d** \n", res);
        return res;
    }
    printf("Sensors initialized successfully\n");

    // Output/Actuator Initialization
    res = actuator_initialization();
    if (res != 0) {
        printf("Actuator initialization failed with **Error %d** \n", res);
        return res;
    }
    printf("Actuators initialized successfully \n");

    //Connection initialization (LoRa)
    res = lora_initialization();
    if (res != 0) {
        printf("LoRa initialization failed with **Error %d** \n", res);
        printf("\n Program will go on, without network transmissions! \n Only Local processing \n");
    }

    // Thread initialization -The lower the priority value, the higher the priority of the thread, with 0 being the highest possible priority.
    // Temperature thread
    thread_create(temperature_thread_stack, sizeof(temperature_thread_stack),
    THREAD_PRIORITY_MAIN + 1, 0, temperature_thread_handler,
    NULL, "temperature_thread");

    // Light thread
    thread_create(light_thread_stack, sizeof(light_thread_stack),
    THREAD_PRIORITY_MAIN + 2, 0, light_thread_handler,
    NULL, "light_thread");

    // Aggregation thread
    thread_create(
            aggregation_thread_stack,
            sizeof(aggregation_thread_stack),
            THREAD_PRIORITY_MAIN + 3,
            0,
            aggregation_thread_handler,
            NULL,
            "aggregation_thread"
    );

    // should be never reached
    return 0;
}
