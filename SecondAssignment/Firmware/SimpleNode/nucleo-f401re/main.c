// Libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "xtimer.h"       // https://doc.riot-os.org/xtimer_8h.html
#include "periph/gpio.h"  // https://doc.riot-os.org/gpio_8h.html
#include "periph/adc.h"   // https://doc.riot-os.org/adc_8h.html
#include "analog_util.h"  // https://doc.riot-os.org/analog__util_8h.html
#include "thread.h"       // https://doc.riot-os.org/core_2include_2thread_8h.html
#include "shell.h"        // https://doc.riot-os.org/shell_8h.html

// Global Constants
#define NUM_INPUT_LINE 2
#define NUM_OUTPUT_LINE 3

#define MAX_TEMPERATURE 24 // 24°C is the highest temperature the poultry can survive
#define MIN_LIGHT 2 //2 lux is the light threshold to switch on the lamp

    //MQTT-S
#ifndef EMCUTE_ID
#define EMCUTE_ID           ("nucleof401re-1.local")
#endif

#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 3)

#define NUMOFSUBS           5
#define TOPIC_MAXLEN        (64U)
#define MESSAGE_MAXLEN      (80U)


// Global variables
    //Analog inputs
    int analog_input_line[NUM_INPUT_LINE] = {0, 1}; //0 for Photoresistor, 1 for Thermistor
    int adc_res[NUM_INPUT_LINE] = {ADC_RES_10BIT, ADC_RES_12BIT}; //For the Photoresistor 10 bits are enough
    int sample_delay[NUM_INPUT_LINE] = {20, 10}; // 10 s per BRIGHTNESS, 1 s per TEMPERATURE

    //Digital Outputs
    int port = PORT_A;
    int digital_pin[NUM_OUTPUT_LINE] = {10, 8, 9};
    gpio_t digital_input_line[NUM_OUTPUT_LINE]; //0 for FAN, 1 for LAMP, 2 for BUZZER

    //Threads - TODO Matrix of stack
    char thread1_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for BRIGHTNESS thread
    char thread2_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for TEMPERATURE thread
    char thread3_stack[THREAD_STACKSIZE_DEFAULT]; //Stack for EMCUTE thread

    // MQTT-S
        //Subscription
    static emcute_sub_t subscriptions[NUMOFSUBS];


        // Although it is possible to use directly 'subscriptions' to store topics, it is clearer to create an array with predefined lenghts
    static char topics[NUMOFSUBS][TOPIC_MAXLEN] = { // sensor/light sensor/temp actuator/buzzer actuator/fan actuator/lamp
        "sensor/light",
        "sensor/temp",

        "actuator/buzzer",
        "actuator/fan",
        "actuator/lamp"
    };
    unsigned flags = EMCUTE_QOS_0;
    const char message_structure[] = "{\"device\":\"%s\", \"value\":\"%d\"}";

//Functions, Thread handlers
    //Thread handler for Temperature
void *temperature_thread_handler(void *arg){
    (void) arg;

    int line_num = 1; // 1 for Temperature Analog line
    int sample;
    int physical_quantity; // CELSIUS
    int status;
    char buffer[MESSAGE_MAXLEN];
    strcpy(buffer, "");

    // Sample continuously the ADC line
	while(1){
        sample = adc_sample(analog_input_line[line_num], adc_res[line_num]);
        if (sample == -1) {
            printf("ADC_LINE(%d): selected resolution not applicable\n", analog_input_line[line_num]);
            return NULL;
        }
        physical_quantity = adc_util_map(sample, adc_res[line_num], 10, 100); //CELSIUS

            //MQTT-S Publication of retrieved value
        sprintf(buffer, message_structure, EMCUTE_ID, physical_quantity); // "{"device":"nucleof401re-1.local", "value":"32"}"
        if (emcute_pub(&subscriptions[1].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
            printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[1].topic.name, subscriptions[1].topic.id);
            return NULL;
        }
        printf("\n Published %d CELSIUS to topic '%s [ID: %i]'\n", physical_quantity, subscriptions[1].topic.name, subscriptions[1].topic.id);

        status = gpio_read(digital_input_line[0]); //0 for FAN
        if(physical_quantity >= MAX_TEMPERATURE  && status > 0){ //Negated logic relay
            printf("ROOM TEMPERATURE is greater than threshold! WARNING!!! \n");

            // Switch on the BUZZER for 10 seconds
            gpio_write(digital_input_line[2], 1); //Switch ON 2 for BUZZER

            // MQTT-S Pubblication
            sprintf(buffer, message_structure, EMCUTE_ID, 1); // "{"device":"m3-1.saclay", "value":"1"}"
            if (emcute_pub(&subscriptions[2].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[2].topic.name, subscriptions[2].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[2].topic.name, subscriptions[2].topic.id);

            xtimer_sleep(2);

            gpio_write(digital_input_line[2], 0); //Switch OFF 2 for buzzer
               // MQTT-S Pubblication
            sprintf(buffer, message_structure, EMCUTE_ID, 0); // "{"device":"m3-1.saclay", "value":"0"}"
            if (emcute_pub(&subscriptions[2].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[2].topic.name, subscriptions[2].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[2].topic.name, subscriptions[2].topic.id);

            //Switch ON the FAN
            gpio_write(digital_input_line[0], 0); //0 for FAN, Relay module works in NEGATED logic
            sprintf(buffer, message_structure, EMCUTE_ID, 0); // "{"device":"m3-1.saclay", "value":"0"}"
            if (emcute_pub(&subscriptions[3].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[3].topic.name, subscriptions[3].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[3].topic.name, subscriptions[3].topic.id);
        }
        else if(physical_quantity < MAX_TEMPERATURE  && status == 0){ //Negated logic relay
            /*Switch OFF the FAN */
            gpio_write(digital_input_line[0], 1); // 0 for FAN, 1 for Relay module works in NEGATED logic
            printf("ROOM TEMPERATURE is less than threshold! TEMPERATURE is OK \n");

            sprintf(buffer, message_structure, EMCUTE_ID, 1); // "{"device":"m3-1.saclay", "value":"1"}"
            if (emcute_pub(&subscriptions[3].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[3].topic.name, subscriptions[3].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[3].topic.name, subscriptions[3].topic.id);

        }
        xtimer_sleep(sample_delay[line_num]);
    }
	return NULL;
}

    //Thread handler for Brightness
void *brightness_thread_handler(void *arg){
    (void) arg;

    int line_num = 0; // 0 for Brightness Analog line
    int sample;
    int physical_quantity; // LUX
    int status;
    char buffer[MESSAGE_MAXLEN];
    strcpy(buffer,"");

    // Sample continously the ADC line
	while(1){
        sample = adc_sample(analog_input_line[line_num], adc_res[line_num]);
        if (sample == -1) { // Error with sample resolution
            printf("ADC_LINE(%d): selected resolution not applicable\n", analog_input_line[line_num]);

            return NULL;
        }
        physical_quantity = adc_util_map(sample, adc_res[line_num], 0, 100); //LUX

        //MQTT-S Publication of retrieved value
        sprintf(buffer, message_structure, EMCUTE_ID, physical_quantity); // "{"device":"m3-1.saclay", "value":"1"}"
        if (emcute_pub(&subscriptions[0].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
            printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[0].topic.name, subscriptions[0].topic.id);
            return NULL;
        }
        printf("\n Published %d LUX to topic '%s [ID: %i]'\n", physical_quantity, subscriptions[0].topic.name, subscriptions[0].topic.id);

        status = gpio_read(digital_input_line[1]);
        if(physical_quantity <= MIN_LIGHT && status > 0){ //Negated logic relay
            // Switch on the LAMP
            gpio_write(digital_input_line[1], 0); //Relay module works in Negated logic, 1 for LAMP
            printf("ROOM ILLUMINANCE is less than threshold, LAMP is ON \n");

            // MQTT-S Pubblication
            //const char buffer[MESSAGE_MAXLEN] = "ON";
            strcpy(buffer, message_structure);
            if (emcute_pub(&subscriptions[4].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[4].topic.name, subscriptions[4].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[4].topic.name, subscriptions[4].topic.id);
        }
        else if(physical_quantity > MIN_LIGHT && status == 0) {
            // Switch OFF the LAMP
            gpio_write(digital_input_line[1], 1); // Relay Module works in Negated logic, 1 for LAMP
            printf("ROOM ILLUMINANCE is greather than threshold, LAMP is OFF \n");

            // MQTT-S Pubblication
            //const char buffer[MESSAGE_MAXLEN] = "OFF";
            strcpy(buffer, message_structure);
            if (emcute_pub(&subscriptions[4].topic, buffer, strlen(buffer), flags) != EMCUTE_OK) {
                printf("\n error: unable to publish data to topic '%s [%d]'\n", subscriptions[4].topic.name, subscriptions[4].topic.id);
                return NULL;
            }
            printf("\n Published '%s' to topic '%s [ID: %i]'\n", buffer, subscriptions[4].topic.name, subscriptions[4].topic.id);
        }
        xtimer_sleep(sample_delay[line_num]);
    }

    return NULL;
}


    //Thread handler for EMCUTE
static void *emcute_thread_handler(void *arg){
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL;    /* should never be reached */
}

   //CallBack function for EMCUTE subscribe
static void on_pub(const emcute_topic_t *topic, void *data, size_t len){
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
    if(strstr(message, "RemoteON") != NULL && strstr(message, EMCUTE_ID) != NULL){
        if(strcmp(topic->name, topics[2]) == 0){ //  "actuator/buzzer/status"
            gpio_write(digital_input_line[2], 1); //Switch ON 2 for BUZZER
            printf("Remote ON of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[3]) == 0){ // "actuator/fan/status"
            gpio_write(digital_input_line[0], 0); //0 for FAN, Relay module works in NEGATED logic
            printf("Remote ON of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[4]) == 0){ // "actuator/lamp/status"
            gpio_write(digital_input_line[1], 0); //Relay module works in Negated logic, 1 for LAMP
            printf("Remote ON of %s \n", topic->name);
        }
    }
    else if(strstr(message, "RemoteOFF") != NULL && strstr(message, EMCUTE_ID) != NULL){
        if(strcmp(topic->name, topics[2]) == 0){ //  "actuator/buzzer/status"
            gpio_write(digital_input_line[2], 0); //Switch OFF 2 for BUZZER
            printf("Remote OFF of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[3]) == 0){ // "actuator/fan/status"
            gpio_write(digital_input_line[0], 1); //0 for FAN, Relay module works in NEGATED logic
            printf("Remote OFF of %s \n", topic->name);
        }
        else if(strcmp(topic->name, topics[4]) == 0){ // "actuator/lamp/status"
            gpio_write(digital_input_line[1], 1); //Relay module works in Negated logic, 1 for LAMP
            printf("Remote OFF of %s \n", topic->name);
        }
    }
}

int main(void){
    int res;
    int i;
    printf("\n RIOT HenHouse application \n");

    // Initialization of the Sensors
        // initialize the ADC lines: 0 for Photoresistor, 1 for Thermistor
    for(i = 0; i < NUM_INPUT_LINE; i++){
        if ((res = adc_init(analog_input_line[i])) == -1) {
            printf("Initialization of ADC_LINE(%d) failed\n", analog_input_line[i]);
            return res;
        }
    }

    // Initialization of the Actuators
    for(i=0; i < NUM_OUTPUT_LINE; i++){
        digital_input_line[i] = GPIO_PIN(port, digital_pin[i]);
        if((res = (gpio_init(digital_input_line[i], GPIO_OUT))) == -1){
            printf("Pin %d of PORT %d  NOT initialized! \n", digital_pin[i], port);
            return res;
        }
    }
    gpio_write(digital_input_line[0], 1); // Relay Module works in NEGATED Logic (1 for LOW, 0 for HIGH)
    gpio_write(digital_input_line[1], 1); // Relay Module works in NEGATED Logic (1 for LOW, 0 for HIGH)

    // MQTTS
    // initialize our subscription buffers
    memset(subscriptions, 0, (NUMOFSUBS * sizeof(emcute_sub_t)));

    // start the emcute thread */

    thread_create(
        thread3_stack,
        sizeof(thread3_stack),
        EMCUTE_PRIO,
        0,
        emcute_thread_handler,
        NULL,
        "emcute_thread"
    );


    sock_udp_ep_t mqtts_gateway = { .family = AF_INET6, .port = SERVER_PORT };

    /* parse address */

    if (ipv6_addr_from_str((ipv6_addr_t *) &mqtts_gateway.addr.ipv6, SERVER_ADDR) == NULL) {
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
    if(res != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", SERVER_ADDR, SERVER_PORT);
        return 1;
    }
    printf("\n Successfully connected to gateway at [%s]:%i\n", SERVER_ADDR, SERVER_PORT);


     // setup subscription to topic


    for(i = 0; i < NUMOFSUBS; i++){
        subscriptions[i].cb = on_pub; //See "emcute.h" to understand! I pass a function to cb
        subscriptions[i].topic.name = topics[i]; //Pointer sub.topic.name is a pointer to a char variable (topics[0])


        if (emcute_sub(&subscriptions[i], flags) != EMCUTE_OK) {
            printf("\n error: unable to subscribe to %s\n", topics[i]);
            return 1;
        }

        //Get the Topic ID
        if (emcute_reg(&subscriptions[i].topic) != EMCUTE_OK) {
            puts("error: unable to obtain topic ID");
            return 1;
        }
        //printf("\n Topic %s has ID: %d \n", topics[i], subscriptions[i].topic.id);
        printf("\n Subscribed to Topic %s with ID: %d \n", subscriptions[i].topic.name, subscriptions[i].topic.id);
    }

    // Thread initialization
    //kernel_pid_t brightness_pid =
    thread_create( //To improve with array of PID
		thread1_stack,
		sizeof(thread1_stack),
    	THREAD_PRIORITY_MAIN - 1,
        THREAD_CREATE_STACKTEST,
        brightness_thread_handler,
        NULL,
        "BrightnessThread"
	);

    //kernel_pid_t temperature_pid =
    thread_create( //To improve with array of PID
        thread2_stack,
		sizeof(thread2_stack),
    	THREAD_PRIORITY_MAIN - 2,
        THREAD_CREATE_STACKTEST,
        temperature_thread_handler,
        NULL,
        "TemperatureThread"
	);

    /* should be never reached */
    return 0;
}
