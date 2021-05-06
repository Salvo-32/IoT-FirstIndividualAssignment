# IoT First individual assignment - Title - Salvatore FAVA

## The problem and IoT need (What is the problem and why do you need IoT?)
A hen house is a building where poultry is bred in order to produce meat and eggs. Nowadays a small eco-friendly structure (with fewer than 100 poultry) has to consider three main factors:
- the free movement of the animals inside the building;
- the use of solar light for the heating;
- the introduction of artificial lighting to regulate the circadian cycle (sleeping and waking cycle).

In order to make the most of the sunlight, the roof of the building consists of/includes some parts made of glass to let the solar light and the heat pass. Unfortunately, in hot periods the temperature can reach very high values( more than 24°C) that can be dangerous for the animals. To keep the temperature at safe levels (between 13°C and 24°C) the owners make use of fans that force the entrance of fresh air from the outside to the inside of the building, thus lowering the temperature.

To improve the quality of the animals’ life and their productivity, poultry farmers use a system of artificial lighting with hot light (less than 3000K) at a low value of illuminance (~1,5 lux). Specifically the illumination is activated at evening hours to keep that illuminance.

To automate the scenario above the IoT technologies fit very well, in particular with a set of sensors (temperature and brightness) and a set of actuators (cooling fans, led lamps, alarm/buzzer) used in the following way. Following the ‘Sense-Think-Act’ paradigm it is possible: 
    1. to measure the temperature at fixed intervals and with the pertinent sensor; 
    2. to elaborate the values previously taken 
    3. to switch the cooling fan actuator on, together with a siren to warn the farmer about high temperature. 
In the same way it is possible:
    1. to point out  the ‘Absence of light’ through the brightness sensor; 
    2. to process the values previously taken 
    3. to operate the lighting system actuator

### Assumptions
- A 220V domestic fan is used to reproduce the cooling fan through relay module.
- To simulate the lighting behaviour, a 220V bulb is used through relay module

## Sensors (What data are collected and by which sensors?)
1. Temperature sensor (Thermistor RS PRO 151-237) [RS Components Website](https://it.rs-online.com/web/p/termistori/0151237/)
    - A thermistor is an electronic component used to measure room temperature. It is a special type of resistor, in particular its resistance changes according to the room temperature. The thermistor employed in this project is a NTC (Negative Temperature Coefficient) one, i.e. the component resistance is inversely proportional to the temperature
    - The following graph shows the qualitative behaviour of the thermistor ![ThermistorGraph](Picture/ThermistorGraph.jpg "ThermistorGraph")
    - Since temperature is the most critical factor to assure life within the henhouse, **periodic** measurements (10 minute intervals) are mandatory   

2. Brightness sensor (Photoresistor LDR Luna Optoelectronics NSL-19M51) [RS Components Website](https://it.rs-online.com/web/p/ldr-fotoresistenze/9146710/)
    - A photoresistor is an electronic component used to sense brightness. It is also called LDR (Light Dependent Resistor) because the value of resistance changes according to the light, namely the higher the light is the lower the resistance is
    - The graph below describes qualitative behaviour of the photoresistor ![PhotoresistorGraph](Picture/PhotoresistorGraph.png "PhotoresistorGraph")
    - Brightness depends on day/night cycle and on weather conditions. 
    - The light factor is less critical than the temperature one, because poultry can still survive in the darkness, then a longer **periodic** sensing (30 minute intervals) is enough

### Collected data from Sensors
By the use of the above-mentioned temperature sensor,an analog signal relative to the real room temperature is produced with respect to the **datasheet** specifications:
|   Parameter   |    Value    |
| ------------- | ------------- |
| Temperature coefficient type  | NTC  |
| Resistance @ 25°C  | 10kΩ  |
| **Tolerance**  | 	**±0.9%**  |
| Max power  | 75mW  |
| Max operative temperature  | +150°C |
| Min operative temperature  | -80°C |

This sensor is wired connected to the [STM Nucleo](README.md/#STM32-Nucleo-64-development-board-&-RIOT-OS) board as in the picture below. 
**Data analysis first step** The board collects the analog signal every **10 minutes** and samples it using the internal 12-bit ADC (Analog-Digital Converter). Finally, the digital sample is converted in **°C** using the ``` adc_util_map() ``` function inside the ``` analog_util.h ``` offered by the RIOT OS.

By the use of the above-mentioned brightness sensor, an analog signal relative to the real room light is produced with respect to the **datasheet** specifications:
|   Parameter   |    Value    |
| ------------- | ------------- |
| Power Dissipation @ 25°C | 50 mW |
| Max Light Resistance | 100 kΩ  |
| Min Light Resistance | 20 kΩ  |
| Dark Resistance  | 20 MΩ  |
| Max operative temperature  | +75°C |
| Max operative temperature  | -60°C |

This sensor is wired connected to the STM Nucleo board as in the picture below. 
**Data analysis first step** The board collects the analog signal every **30 minutes** and samples it using the internal 12-bit ADC (Analog-Digital Converter). Finally, the digital sample is converted in **LUX** using the ``` adc_util_map() ``` function inside the ``` analog_util.h ``` offered by the RIOT OS.  


## Actuators
1. Active buzzer (Buzzer RS PRO 171-0898) [RS Components Website](https://it.rs-online.com/web/p/buzzer-magnetici/1710898/)
    - A buzzer, also named beeper, is an electronic devices used to emit a fixed sound whenever it is powered on. 
    - In details, this project involves an electromagnetic buzzer that lets out a constant tone (85 dB)
    - The actuator is wired connected to the [STM Nucleo](README.md/#STM32-Nucleo-64-development-board-&-RIOT-OS) board as in the picture below.
    - As described in [The problem and IoT need](README.md/#The-problem-and-IoT-need) section, the whole system makes use of the buzzer to acoustically inform the farmer about high temperature inside the poultry. Remember according the temperature sensor data, the buzzer is turned on or off.    
2. Relay Module with Optocoupler (8 Channels) [Elegoo Website](https://www.elegoo.com/collections/electronic-component-kits/products/elegoo-8-channel-relay-module-kit?variant=32467576324144)
    - The relay module is an electrically operated switch that allows you to turn on or off a circuit with a voltage and/or current that is much higher than a microcontroller could handle. 
    - There is no connection between the low voltage circuit operated by the microcontroller and the high power circuit. The relay protects each circuit from the other.
    - This actuator is wired connected to the [STM Nucleo](README.md/#STM32-Nucleo-64-development-board-&-RIOT-OS) board as in the picture below. First channel is devoted to the cooling fan and second channel to lamp.
    - As described in [The problem and IoT need](README.md/#The-problem-and-IoT-need) section, the relay is employed to separately activate both the cooling fan and the lamp. The former lowers the temperature whenever this one is above the threshold (24°C) while the latter keeps the illuminance constant (1.5 lux) whenever this one is below the threshold (<1.5 lux). Rememeber according to the processed temperature data the cooling fan is turned on or off and according to processed brigthness data the lamp is switched on or off. 

## STM32 Nucleo-64 development board & RIOT-OS
The core of the whole system is [STM NUCLEO-F401RE](https://www.st.com/en/evaluation-tools/nucleo-f401re.html) development board. It allows to build a prototype of the real system within an affordable all-in-one platform (ST-LINK debugger/programmer are included).

To develop the software needed to manage and process all the data of the infrastructure, the STM board relies on the **RIOT operating system**.
[RIOT](https://www.riot-os.org/)  is a free, open source operating system developed by a grassroots community gathering companies, academia, and hobbyists, distributed all around the world. RIOT supports most low-power IoT devices and microcontroller architectures (32-bit, 16-bit, 8-bit). RIOT aims to implement all relevant open standards supporting an Internet of Things that is connected, secure, durable & privacy-friendly.


## What are the connected components, the protocols to connect them and the overall IoT architecture?
### Network

## Sources
* https://www.agraria.org/polli/ricoveri.htm
* https://www.poultryledlights.com/
* https://en.wikipedia.org/wiki/International_System_of_Units
