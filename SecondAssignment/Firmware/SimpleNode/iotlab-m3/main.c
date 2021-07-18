// Libraries - They have to reside before every other thing
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xtimer.h"
#include "thread.h"
#include "timex.h"      // Utility library for comparing and computing timestamps
#include "/home/salvo/RIOT/sys/include/net/emcute.h"

#include "periph/gpio.h"
#include "msg.h"
#include "net/ipv6/addr.h"

// To get CPU_ID
#include "cpu_conf.h"
#include "/home/salvo/RIOT/drivers/include/periph/cpuid.h"

//  Pressure/Temperature sensor libraries
//#include "lpsxxx.h"
#include "/home/salvo/RIOT/drivers/include/lpsxxx.h"
#include "lpsxxx_params.h"

//  Accelerometer/Magnetometer sensor libraries
//#include "lsm303dlhc.h"
#include "/home/salvo/RIOT/drivers/include/lsm303dlhc.h"

//#include "lsm303dlhc_params.h"
#include "/home/salvo/RIOT/drivers/lsm303dlhc/include/lsm303dlhc_params.h"

//  Gyroscope sensor libraries
//#include "l3g4200d.h"
#include "/home/salvo/RIOT/drivers/include/l3g4200d.h"
#include "l3g4200d_params.h"

//	Light sensor libraries
//#include "isl29020.h"
#include "/home/salvo/RIOT/drivers/include/isl29020.h"
#include "isl29020_params.h"


//Global variables and constants
//  Pressure/Temperature sensor
static lpsxxx_t lpsxxx;
int temperature_sample_rate = 15; // 10 m = 600 s for TEMPERATURE (The SAME as FIRST Assignment)
#define MAX_TEMPERATURE 24 // 24°C is the highest temperature the poultry can survive
char temperature_thread_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for Temperature thread

//  Light sensor
static isl29020_t isl29020;
int light_sample_rate = 30; // 30 m = 1800 s per Light (The SAME as FIRST Assignment)
#define MIN_LIGHT 2 //2 lux is the light threshold to switch on the lamp
char light_thread_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for BRIGHTNESS thread

// IoT-Lab M3 Actuators - https://doc.riot-os.org/group__boards__common__iotlab.html
#define LED0_PIN   GPIO_PIN(PORT_D, 2)  // Led0 simultaes BUZZER (wrt First ind. assign.)
#define	LED1_PIN   GPIO_PIN(PORT_B, 5)  // Led1 simulates FAN (wrt First ind. assign.)
#define LED2_PIN   GPIO_PIN(PORT_C, 10) // Led2 simulates LAMP (wrt First ind. assign.)

#define CPU_ID_MAXLEN 36
#define CPU_NAME_MAXLEN 25
typedef struct board_info
{
    char cpu_id[CPU_ID_MAXLEN];
    char name[CPU_NAME_MAXLEN];
} board_info;


//Both the following information are taken by submitting an experiment at FIT IoT-Lab Saclay using the CLI
// Starting from CPU_ID the program retrieve CPU_NAME, and in turn it becomes EMCUTE_ID
// (to identify uniquely a board)
#define SITE_BOARD_NUM 11
board_info site_board_info[SITE_BOARD_NUM] =   //I take into account Saclay site!
{
    {"32-ff-d6-05-33-48-32-36-20-53-02-43", "m3-1.saclay"},
    {"32-ff-dc-05-33-48-32-36-26-61-02-43", "m3-2.saclay"},
//  {"AliveNodeInSaclay",                   "m3-3.saclay"},
    {"32-ff-d9-05-33-48-32-36-23-53-02-43", "m3-4.saclay"},
    {"32-ff-da-05-33-48-32-36-43-74-03-43", "m3-5.saclay"},
    {"32-ff-d9-05-33-48-32-36-27-60-02-43", "m3-6.saclay"},
    {"32-ff-d8-05-33-48-32-36-24-53-02-43", "m3-7.saclay"},
    {"32-ff-d3-05-33-48-32-36-20-61-02-43", "m3-8.saclay"},
    {"32-ff-dc-05-33-48-32-36-24-60-02-43", "m3-9.saclay"},
    {"32-ff-dd-05-33-48-32-36-40-61-02-43", "m3-10.saclay"},
    {"32-ff-dc-05-33-48-32-36-44-71-03-43", "m3-11.saclay"},
    {"32-ff-da-05-33-48-32-36-35-60-02-43", "m3-12.saclay"},
}; //Since there are 11 M3-boards actually in Saclay


// MQTT-S
//Subscription
char EMCUTE_ID[CPU_NAME_MAXLEN];
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)

#define NUMOFSUBS           5
#define TOPIC_MAXLEN        (64U)
#define MESSAGE_MAXLEN      (80U)

#define MQTTS_BROKER_PORT 1885
#define MQTTS_BROKER_ADDR "2001:660:3207:400::2" //saclay a8-2

// Although it is possible to use directly 'subscriptions' to store topics, it is clearer to create an array with predefined lenghts
static char topics[NUMOFSUBS][TOPIC_MAXLEN] =
{
    "sensor/temperature",   // 0
    "sensor/light",         // 1

    "actuator/buzzer",      // 2
    "actuator/fan",         // 3
    "actuator/lamp"         // 4
};

const char message_structure[] = "{\"device\":\"%s\", \"value\":\"%d\"}";

static emcute_sub_t subscriptions[NUMOFSUBS];
unsigned flags = EMCUTE_QOS_0;
char mqtts_client_thread_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for EMCUTE thread


//Functions, Thread handlers
//Thread handler for EMCUTE
static void *mqtts_client_thread_handler(void *arg)
{
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL;    // should never be reached
}

//CallBack function for EMCUTE subscribe
static void on_pub(const emcute_topic_t *topic, void *data, size_t len)
{
    char *in = (char *)data; //Buffer sporco
    char message[MESSAGE_MAXLEN] = "";

    printf("### got publication for topic '%s' [%i] ###\n", topic->name, (int)topic->id);
    size_t i;
    for (i = 0; i < len; i++)
    {
        message[i] = in[i];
        printf("%c", in[i]);
    }
    message[i+1]='\0';
    puts("");

    // If message is a remoteMessage and it is for the current device...
    if(strstr(message, "RemoteON") != NULL && strstr(message, EMCUTE_ID) != NULL)
    {
        if(strcmp(topic->name, topics[2]) == 0)  //  "actuator/buzzer/status"
        {
            gpio_write(LED0_PIN, 1); //Switch ON BUZZER
            printf("Remote ON of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[3]) == 0)  // "actuator/fan/status"
        {
            gpio_write(LED1_PIN, 1);
            printf("Remote ON of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[4]) == 0)  // "actuator/lamp/status"
        {
            gpio_write(LED2_PIN, 1);
            printf("Remote ON of %s \n", topic->name);
        }
    }
    else if(strstr(message, "RemoteOFF") != NULL && strstr(message, EMCUTE_ID) != NULL)
    {
        if(strcmp(topic->name, topics[2]) == 0)  //  "actuator/buzzer/status"
        {
            gpio_write(LED0_PIN, 0);
            printf("Remote OFF of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[3]) == 0)  // "actuator/fan/status"
        {
            gpio_write(LED1_PIN, 0);
            printf("Remote OFF of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[4]) == 0)  // "actuator/lamp/status"
        {
            gpio_write(LED2_PIN, 0);
            printf("Remote OFF of %s \n", topic->name);
        }
    }
}

//Thread handler for Temperature
void *temperature_thread_handler(void *arg)
{
    (void) arg;
    int res;
    int status;
    int topic_number;
    char buffer[MESSAGE_MAXLEN];
    strcpy(buffer, "");

    int16_t physical_quantity = 0;

    // Sample continuously
    while(1)
    {
        res = lpsxxx_read_temp(&lpsxxx, &physical_quantity);
        if (res == LPSXXX_ERR_I2C)
        {
            printf("Error on I2C communication with LPSXXX sensor\n");
            return NULL;
        }
        physical_quantity = physical_quantity/100;
        //printf("Temperature: %d °C\n", physical_quantity); //Debug only

        //MQTT-S Publication of retrieved value
        topic_number = 0;
        sprintf(buffer, message_structure, EMCUTE_ID, physical_quantity); // "{"device":"m3-1.saclay", "value":"32"}"
        if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
        {
            printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
            return NULL;
        }

        status = gpio_read(LED1_PIN); // Led1 acts as the FAN actuator of the First assignment
        if(physical_quantity >= MAX_TEMPERATURE  && status == 0)
        {
            printf("ROOM TEMPERATURE is greater than threshold! WARNING!!! \n");

            // Switch on the BUZZER for 10 seconds
            gpio_write(LED0_PIN, 1); //Switch ON for BUZZER
            // MQTT-S Pubblication
            topic_number = 2;
            sprintf(buffer, message_structure, EMCUTE_ID, 1); // "{"device":"m3-1.saclay", "value":"1"}"
            if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
            {
                printf("\n error: unable to publish data to topic '%s'\n", subscriptions[topic_number].topic.name);
                return NULL;
            }

            xtimer_sleep(2);

            gpio_write(LED0_PIN, 0); //Switch OFF for buzzer
            // MQTT-S Pubblication
            topic_number = 2;
            sprintf(buffer, message_structure, EMCUTE_ID, 0); // "{"device":"m3-1.saclay", "value":"0"}"
            if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
            {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
                return NULL;
            }

            //Switch ON the FAN
            gpio_write(LED1_PIN, 1);
            topic_number = 3;
            sprintf(buffer, message_structure, EMCUTE_ID, 1); // "{"device":"m3-1.saclay", "value":"1"}"
            if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
            {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
                return NULL;
            }
        }
        else if(physical_quantity < MAX_TEMPERATURE  && status == 1)
        {
            //Switch OFF the FAN
            gpio_write(LED1_PIN, 0);
            printf("ROOM TEMPERATURE is less than threshold! TEMPERATURE is OK \n");
            topic_number = 3;
            sprintf(buffer, message_structure, EMCUTE_ID, 0); // "{"device":"m3-1.saclay", "value":"0"}"
            if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
            {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
                return NULL;
            }
        }
        xtimer_sleep(temperature_sample_rate);
    }
    return NULL; //Should never be reached
}

//Thread handler for light
void *light_thread_handler(void *arg)
{
    (void) arg;
    int physical_quantity; // LUX
    int status;
    int topic_number;
    char buffer[MESSAGE_MAXLEN];
    strcpy(buffer,"");

    // Sample continously the ADC line
    while(1)
    {
        // Read the Light values
        physical_quantity = isl29020_read(&isl29020);
        if(physical_quantity == -1)
        {
            printf("Error while reading Light sensor\n");
            return NULL;
        }
        printf("Light: %d lux \n", physical_quantity);

        //MQTT-S Publication of retrieved value
        topic_number = 1;
        sprintf(buffer, message_structure, EMCUTE_ID, physical_quantity); // "{"device":"m3-1.saclay", "value":"32"}"
        if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
        {
            printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
            return NULL;
        }

        status = gpio_read(LED2_PIN);
        if(physical_quantity <= MIN_LIGHT && status == 0)
        {
            // Switch on the LAMP
            gpio_write(LED2_PIN, 1);
            printf("ROOM ILLUMINANCE is less than threshold, LAMP is ON \n");

            // MQTT-S Pubblication
            topic_number = 4;
            sprintf(buffer, message_structure, EMCUTE_ID, 1); // "{"device":"m3-1.saclay", "value":"1"}"
            if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
            {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
        }
        else if(physical_quantity > MIN_LIGHT && status > 0)
        {
            // Switch OFF the LAMP
            gpio_write(LED2_PIN, 0); // Relay Module works in Negated logic, 1 for LAMP
            printf("ROOM ILLUMINANCE is greather than threshold, LAMP is OFF \n");

            // MQTT-S Pubblication
            topic_number = 4;
            sprintf(buffer, message_structure, EMCUTE_ID, 0); // "{"device":"m3-1.saclay", "value":"0"}"
            if (emcute_pub(&subscriptions[topic_number].topic, buffer, strlen(buffer), flags) != EMCUTE_OK)
            {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[topic_number].topic.name, subscriptions[topic_number].topic.id);
        }
        xtimer_sleep(light_sample_rate);
    }
    return NULL;
}



// Initialization of Sensors
int sensor_initialization(void)
{
    int res;

    //  Initialize the Temperature sensor
    res = lpsxxx_init(&lpsxxx, &lpsxxx_params[0]);
    if ( res == LPSXXX_ERR_NODEV)
    {
        printf("No valid LPSXXX device found \n");
        return -1;
    }
    else if ( res == LPSXXX_ERR_I2C)
    {
        printf("I2C error for LPSXXX device \n");
        return -1;
    }

    //  Initialize the Light sensor
    res = isl29020_init(&isl29020, &isl29020_params[0]);
    if ( res == -1)
    {
        printf("isl29020 device error on initialization \n");
        return -1;
    }

    return 0; //on success
}

// Initialization of Actuators
int actuator_initilization(void)
{
    int res;

    //  Led-0 simulates BUZZER of the 1st assignment
    if((res = (gpio_init(LED0_PIN, GPIO_OUT))) == -1)
    {
        printf("Led-0 NOT initialized! \n");
        return res;
    }
    //  Led-1 simulates FAN of the 1st assignment
    if((res = (gpio_init(LED1_PIN, GPIO_OUT))) == -1)
    {
        printf("Led-1 NOT initialized! \n");
        return res;
    }
    //  Led-2 simulates LAMP of the 1st assignment
    if((res = (gpio_init(LED2_PIN, GPIO_OUT))) == -1)
    {
        printf("Led-2 NOT initialized! \n");
        return res;
    }
    return 0; //on success
}

int mqtts_client_initialization(void)
{
    int res = 0;
    int i = 0;
    // initialize our subscription buffers
    memset(subscriptions, 0, (NUMOFSUBS * sizeof(emcute_sub_t)));

    // start the emcute thread
    thread_create(
        mqtts_client_thread_stack,
        sizeof(mqtts_client_thread_stack),
        EMCUTE_PRIO,
        0,
        mqtts_client_thread_handler,
        NULL,
        "emcute_thread"
    );

    sock_udp_ep_t mqtts_gateway = { .family = AF_INET6, .port = MQTTS_BROKER_PORT };

    //parse address
    if (ipv6_addr_from_str((ipv6_addr_t *) &mqtts_gateway.addr.ipv6, MQTTS_BROKER_ADDR) == NULL)
    {
        printf("error parsing IPv6 address\n");
        return 1;
    }


    char predefined_will_topic[TOPIC_MAXLEN] = "LastWill";
    char predefined_will_message[MESSAGE_MAXLEN] = "Abnormal disconnect";

    //strcat(predefined_will_message, EMCUTE_ID);
    size_t predefined_will_msg_len = strlen(predefined_will_message);


    res = emcute_con(
              &mqtts_gateway,
              true,
              predefined_will_topic,
              predefined_will_message,
              predefined_will_msg_len,
              0
          );
    if(res != EMCUTE_OK)
    {
        printf("error: unable to connect to [%s]:%i\n", MQTTS_BROKER_ADDR, MQTTS_BROKER_PORT);
        return 1;
    }
    printf("\n Successfully connected to gateway at [%s]:%i\n", MQTTS_BROKER_ADDR, MQTTS_BROKER_PORT);


    // setup subscription to topic
    for(i = 0; i < NUMOFSUBS; i++)
    {
        subscriptions[i].cb = on_pub; //See "emcute.h" to understand! I pass a function to cb callback function
        subscriptions[i].topic.name = topics[i]; //Pointer sub.topic.name is a pointer to a char variable (topics[0])


        if (emcute_sub(&subscriptions[i], flags) != EMCUTE_OK)
        {
            printf("\n error: unable to subscribe to %s\n", topics[i]);
            return 1;
        }

        //Get the Topic ID
        if (emcute_reg(&subscriptions[i].topic) != EMCUTE_OK) //IN: &subscriptions[i].topic.name -> OUT: &subscriptions[i].topic.id
        {
            puts("error: unable to obtain topic ID");
            return 1;
        }
        printf("\n Subscribed to Topic %s with ID: %d \n", subscriptions[i].topic.name, subscriptions[i].topic.id);
    }
    return 0; // on success
}

int get_device_name(char *result){
    uint8_t hex_cpu_id[CPUID_LEN]; //CPUID_LEN is already defined into cpu_conf.h
    cpuid_get(hex_cpu_id); //It returns CPU ID in hexadecimal form (look at libraries)
    char current_cpu_id[CPUID_LEN*3]; //String version of CPU_ID

    sprintf(
        current_cpu_id,
        "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
        hex_cpu_id[0], hex_cpu_id[1], hex_cpu_id[2], hex_cpu_id[3], hex_cpu_id[4], hex_cpu_id[5], hex_cpu_id[6], hex_cpu_id[7], hex_cpu_id[8], hex_cpu_id[9], hex_cpu_id[10], hex_cpu_id[11]
    );

    for(int i = 0; i < SITE_BOARD_NUM; i++)
    {
        if (strcmp(current_cpu_id, site_board_info[i].cpu_id) == 0)
        {
            strcpy(result, site_board_info[i].name);
             //= ; //Passage of address
            return 0;
        }
    }
    return -1; //If it reaches here, then no device found -> empty string returned
}

int main(void)
{
    int res = 0;
    printf("\n RIOT HenHouse application \n");

    //Set emcute_id
    res = get_device_name(EMCUTE_ID); //By ref paramter
    if (res == -1){
        puts("Your board is not present in this site!");
        puts("The program terminates here");
        return -1;
    } //else
    printf("%s", EMCUTE_ID);
    puts("Board found!");

    // Initialization of Sensors
    res = sensor_initialization();
    if ( res == -1 )
    {
        // Failure on actuator initialization
        return res;
    }

    // Initialization of Actuators
    res = actuator_initilization();
    if ( res == -1 )
    {
        // Failure on actuator initialization
        return res;
    }

    // MQTTS
    res = mqtts_client_initialization();
    if ( res == 1 )
    {
        // Failure on mqtts client initialization
        return res;
    }


    // Thread initialization
    // Temperature thread
    thread_create(
        temperature_thread_stack,
        sizeof(temperature_thread_stack),
        THREAD_PRIORITY_MAIN + 2,
        0,
        temperature_thread_handler,
        NULL,
        "temperature_thread"
    );
    // Light thread
    thread_create(
        light_thread_stack,
        sizeof(light_thread_stack),
        THREAD_PRIORITY_MAIN + 6,
        0,
        light_thread_handler,
        NULL,
        "light_thread"
    );

    // should be never reached
    return 0;
}
