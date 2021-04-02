#include <stdio.h>
#include <string.h>

/*
#include "xtimer.h"       // https://doc.riot-os.org/xtimer_8h.html
#include "periph/gpio.h"  // https://doc.riot-os.org/gpio_8h.html
#include "periph/adc.h"   // https://doc.riot-os.org/adc_8h.html
#include "analog_util.h"  // https://doc.riot-os.org/analog__util_8h.html
#include "thread.h"       //

*/


#include "/home/salvo/RIOT/sys/include/xtimer.h"           // https://doc.riot-os.org/xtimer_8h.html
#include "/home/salvo/RIOT/drivers/include/periph/gpio.h"  // https://doc.riot-os.org/gpio_8h.html
#include "/home/salvo/RIOT/drivers/include/periph/adc.h"   // https://doc.riot-os.org/adc_8h.html
#include "/home/salvo/RIOT/sys/include/analog_util.h"      // https://doc.riot-os.org/analog__util_8h.html
#include "/home/salvo/RIOT/core/include/thread.h"


// Global Constants
#define NUM_INPUT_LINE 2
#define NUM_OUTPUT_LINE 3

#define MAX_TEMPERATURE 24 // 24°C is the highest temperature the poultry can survive
#define MIN_LIGHT 2 //2 lux is the light threshold to switch on the lamp


// Global variables
    //Analog inputs
    int analog_input_line[NUM_INPUT_LINE] = {0, 1}; //0 for Photoresistor, 1 for Thermistor
    int adc_res[NUM_INPUT_LINE] = {ADC_RES_10BIT, ADC_RES_12BIT}; //For the Photoresistor 10 bits are enough
    //int sample_delay[NUM_INPUT_LINE] = {10000LU * US_PER_MS, 7000LU * US_PER_MS}; // 500 ms per BRIGHTNESS, 100 ms per TEMPERATURE
    int sample_delay[NUM_INPUT_LINE] = {10, 1}; // 10 s per BRIGHTNESS, 1 s per TEMPERATURE

    //Digital Outputs
    int port = PORT_A;
    int digital_pin[NUM_OUTPUT_LINE] = {10, 8, 9};
    gpio_t digital_input_line[NUM_OUTPUT_LINE]; //0 for FAN, 1 for LAMP, 2 for BUZZER

    //Threads
    char thread_stack[THREAD_STACKSIZE_MAIN]; //Stack for thread


//Function, Procedures, Thread handlers
    //Thread for Brightness
/*
void *brightness_thread_handler(void *arg){
    (void) arg;

    int line_num = 0; // 0 for Brightness Analog line
    //xtimer_ticks32_t last = xtimer_now();
    int sample;
    int physical_quantity; // LUX

    // Sample continously the ADC line
	while(1){
		puts("I'm the BRIGHTNESS THREAD I'm ALWAYS active, but I sleep for 20 seconds\n");

        sample = adc_sample(analog_input_line[line_num], adc_res[line_num]);
        if (sample == -1) {
            printf("ADC_LINE(%d): selected resolution not applicable\n", analog_input_line[line_num]);
            //return -1;
        }
        else {
            physical_quantity = adc_util_map(sample, adc_res[line_num], 0, 100); //LUX
            printf("ADC_LINE(%d): raw value: %i, lux: %i\n", analog_input_line[line_num], sample, physical_quantity);
            if(physical_quantity <= MIN_LIGHT){
                 // Switch on the LAMP
                gpio_write(digital_input_line[1], 0); //Relay module works in Negated logic, 1 for LAMP
                printf("ROOM ILLUMINANCE is less than threshold, LAMP is ON \n");
            }
            else{
                // Switch OFF the LAMP
                gpio_write(digital_input_line[1], 1); // Relay Module works in Negated logic, 1 for LAMP
                printf("ROOM ILLUMINANCE is greather than threshold, LAMP is OFF \n");
            }
        }
        //xtimer_periodic_wakeup(&last, sample_delay[line_num]);
        xtimer_sleep(sample_delay[line_num]);
    }
	return NULL;
}
*/
void *temperature_thread_handler(void *arg){
    (void) arg;

    int line_num = 1; // 1 for Temperature Analog line
    //xtimer_ticks32_t last = xtimer_now();
    int sample;
    int physical_quantity; // CELSIUS

    // Sample continously the ADC line
	while(1){
		puts("I'm the TEMPERATURE THREAD I'm ALWAYS active, but I sleep for 10 seconds\n");

        sample = adc_sample(analog_input_line[line_num], adc_res[line_num]);
        if (sample == -1) {
            printf("ADC_LINE(%d): selected resolution not applicable\n", analog_input_line[line_num]);
            //return -1;
        }
        else {
            physical_quantity = adc_util_map(sample, adc_res[line_num], 10, 100); //CELSIUS
            printf("ADC_LINE(%d): raw value: %i, CELSIUS: %i\n", analog_input_line[line_num], sample, physical_quantity);
            if(physical_quantity >= MAX_TEMPERATURE){
                printf("ROOM TEMPERATURE is greather than threshold! WARNING!!! \n");

                // Switch on the BUZZER for 10 seconds
                gpio_write(digital_input_line[2], 1); //Switch ON 2 for BUZZER
                puts("Buzzer is ON");
                xtimer_sleep(2);
                gpio_write(digital_input_line[2], 0); //Switch OFF 2 for buzzer
                puts("Buzzer is OFF");

                //Switch ON the FAN
                puts("FAN is ON");
                gpio_write(digital_input_line[0], 0); //0 for FAN, Relay module works in NEGATED logic
            }
            else{
                /*Switch OFF the LAMP */
                gpio_write(digital_input_line[0], 1); // 0 for FAN, Relay module works in NEGATED logic
                printf("ROOM TEMPERATURE is less than threshold! TEMPERATURE is OK \n");
            }
        }
        //xtimer_periodic_wakeup(&last, sample_delay[line_num]);
        xtimer_sleep(sample_delay[line_num]);
    }
	return NULL;
}

int main(void){
    int res;
    int i;

    printf("\n RIOT HenHouse application \n");

    // Initialization of the Sensors
    printf("\n Analog line:%d, with resolution: %d\n", analog_input_line[1], adc_res[1]);

        // initialize the ADC lines: 0 for Photoresistor, 1 for Thermistor
    for(i = 0; i < NUM_INPUT_LINE; i++){
        if ((res = adc_init(analog_input_line[i])) == -1) {
            printf("Initialization of ADC_LINE(%d) failed\n", analog_input_line[i]);
            return res;
        }
        else { //To delete
            printf("Successfully initialized ADC_LINE(%d)\n", analog_input_line[i]);
        }
    }

    // Initialization of the Actuators
    for(i=0; i < NUM_OUTPUT_LINE; i++){
        digital_input_line[i] = GPIO_PIN(port, digital_pin[i]);
        if((res = (gpio_init(digital_input_line[i], GPIO_OUT))) == -1){
            printf("Pin %d of PORT %d  NOT initialized! \n", digital_pin[i], port);
            return res;
        }
        else{ //To delete
            printf("Pin %d of PORT %d initialized! \n", digital_pin[i], port);
        }
    }
    gpio_write(digital_input_line[0], 1); // Relay Module works in NEGATED Logic (1 for LOW, 0 for HIGH)
    gpio_write(digital_input_line[1], 1); // Relay Module works in NEGATED Logic (1 for LOW, 0 for HIGH)

    // Thread initialization
/*
    kernel_pid_t brightness_pid = thread_create( //To improve with array of PID
		thread_stack,
		sizeof(thread_stack),
    	THREAD_PRIORITY_MAIN - 1,
        THREAD_CREATE_STACKTEST,
        brightness_thread_handler,
        NULL,
        "BrightnessThread"
	);
    printf("\n From MAIN, the brightness pid is: %d \n", brightness_pid);
*/
    kernel_pid_t temperature_pid = thread_create( //To improve with array of PID
        thread_stack,
		sizeof(thread_stack),
    	THREAD_PRIORITY_MAIN - 2,
        THREAD_CREATE_STACKTEST,
        temperature_thread_handler,
        NULL,
        "TemperatureThread"
	);
    printf("\n From MAIN, the temperature pid is: %d", temperature_pid);

/*
	if(pid == EINVAL){
		printf("\n Error: Priority of the thread greater than or equal  error %d\n", SCHED_PRIO_LEVELS);
		return 1;
	}
	else if(pid == EOVERFLOW){
		printf("\n Error: Too many thread already running \n");
		return 1;
	}
	else{
		printf("Thread %d created correctly \n", pid);
	}
*/


    return 0;
}
