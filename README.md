# IoT First individual assignment - Title - Salvatore FAVA

## The problem and IoT need
A henhouse è una costruzione fisica che consente l'allevamento di poultry galline allo scopo di produrre carni e uova. Oggigiorno, una struttura di piccole dimensioni (< 100 galline) ecosostenibile, prevede principalmente tre fattori:
- Il libero movimento di tali animali all'interno dello stabile, 
- L'impiego di luce solare per il riscaldamento
- L'introduzione di illuminazione artificiale per regolare il ciclo circadiano (ciclo sonno/veglia)

Per sfruttare al meglio le ore di luce, il tetto dello stabile comprende delle parti/zone in vetro per favorire l'ingresso di luce solare e calore. Unfortunately, nei periodi caldi la temperatura può raggiungere valori molto alti (> 24°C), mettendo in pericolo la vita degli animali. Per raffrescare e mantenere sicura la temperatura (13°C - 24°C), i proprietari impiegano dei ventilatori che forzano l'ingresso di aria fresca dall'esterno verso l'interno abbassando la temperatura. 

Per migliorare la qualità di vita degli animali e la loro produttività, poultry farmer impiegano un sistema di illuminazione artificiale a luce calda (< 3000 K) a bassa valore di illuminance (~ 1,5 lux). Nello specifico l'illuminazione viene attivata durante le ore notturne per mantenere such illuminance. 

To automate of the above scenario the IoT technologies fit very well, namely implementing set of sensors (temperature and brightness) and a set of actuators (colling fan, led lamp, alarm) in the following way. Seguendo il paradigma Sense-Think-Act, è possibile rilevare ad intervalli temporali fissati la temperatura tramite il relativo sensore, elaborare e valutare il valore rilevato ed azionare l'attuatore cooling fan, notificando l'accensione con una sirena. Allo stesso modo è possibile rilevare l'evento assenza di luce attraverso il sensore di luminosità, processare i valori rilevati ed azionere l'attuatore relè relativo all'impianto di illuminazione

### Assumptions
- Per riprodurre il cooling fan viene impiegato un ventilatore domestico a 220V azionato tramite relay module
- To simulate the lighting behaviour, a 220v bulb is used through relay module


### Sensors
1. Temperature sensor (Thermistor RS PRO 151-237) [RS Components Website](https://it.rs-online.com/web/p/termistori/0151237/)
    - A thermistor is an electronic component used to measure room temperature. It is a special type of resistor, namely its resistance changes according to room temperature. The one (that is) deployed in this project is a NTC (Negative Temperature Coefficient) thermistor, i.e. the component resistance is inversely proportional to temperature
    - The following graph shows the qualitative behaviour of the thermistor ![ThermistorGraph](Picture/ThermistorGraph.jpg "ThermistorGraph")
    - Since temperature is the most critical factor to assure life within the poultry farm, **periodic** measurements (10 mins) are mandatory   
2. Brightness sensor (Photoresistor LDR Luna Optoelectronics NSL-19M51) [RS Components Website](https://it.rs-online.com/web/p/ldr-fotoresistenze/9146710/)
    - A photoresistor is an electronic component used to sense the brightness. It is also called LDR (Light Dependent Resistor) because the value of resistance changes according to the light, namely the higher the light is the lower the resistance is
    - The below graph describes qualitative behaviour of the photoresistor  ![PhotoresistorGraph](Picture/PhotoresistorGraph.png "PhotoresistorGraph")
    - Brightness depends on the weather condition during the daytime and the day/night variation. With respect to the temperature, the light factor is less critical, then a longer (1 hour) **periodic** sensing is enough

### Actuators
1. Active buzzer (Buzzer RS PRO 171-0898) [RS Components Website](https://it.rs-online.com/web/p/buzzer-magnetici/1710898/)
    - A buzzer, also named beeper, is an electronic devices used to emit a fixed sound whenever it is powered on. 
    - More in details, this project involves an electromagnetic buzzer that plays a constant tone (85 dB) 
2. Relay Module with Optocoupler (8 Channels) [Elegoo Website](https://www.elegoo.com/collections/electronic-component-kits/products/elegoo-8-channel-relay-module-kit?variant=32467576324144)
    - The relay module is an electrically operated switch that allows you to turn on or off a circuit using voltage and/or current much higher than a microcontroller could handle. 
    - There is no connection between the low voltage circuit operated by the microcontroller and the high power circuit. The relay protects each circuit from each other.

### STM32 Nucleo-64 development board & RIOT-OS
The core of the whole system is [STM NUCLEO-F401RE](https://www.st.com/en/evaluation-tools/nucleo-f401re.html) development board. It allows to build a prototype of the real system within an affordable all-in-one platform (ST-LINK debugger/programmer are included).

To develop the software needed to manage and process all the data of the infrastructure, the STM board relies on the **RIOT operating system**.
[RIOT](https://www.riot-os.org/)  is a free, open source operating system developed by a grassroots community gathering companies, academia, and hobbyists, distributed all around the world. RIOT supports most low-power IoT devices and microcontroller architectures (32-bit, 16-bit, 8-bit). RIOT aims to implement all relevant open standards supporting an Internet of Things that is connected, secure, durable & privacy-friendly.

## Collected data from Sensors
By the use of the above temperature sensor,an analog signal relative to the real room temperature is produced with respect to the **datasheet** specifications:
|   Parameter   |    Value    |
| ------------- | ------------- |
| Temperature coefficient type  | NTC  |
| Resistance @ 25°C  | 10kΩ  |
| **Tolerance**  | 	**±0.9%**  |
| Max power  | 75mW  |
| Max operative temperature  | +150°C |
| Min operative temperature  | -80°C |
This sensor is wired connected to the STM Nucleo board as in picture/circuit below. The board retrieves the analog signal every **10 minutes** and samples it using the internal 12-bit ADC (Analog-Digital Converter). Finally, the digital sample is converted in **°C** using the ``` adc_util_map() ``` function inside the ``` analog_util.h ``` offered by the RIOT OS.

By the use of the above brightness sensor,an analog signal relative to the real room light is produced with respect to the **datasheet** specifications:
|   Parameter   |    Value    |
| ------------- | ------------- |
| Power Dissipation @ 25°C | 50 mW |
| Max Light Resistance | 100 kΩ  |
| Min Light Resistance | 20 kΩ  |
| Dark Resistance  | 20 MΩ  |
| Max operative temperature  | +75°C |
| Max operative temperature  | -60°C |

This sensor is wired connected to the STM Nucleo board as in picture/circuit below. The board retrieves the analog signal every **1 hour** and samples it using the internal 12-bit ADC (Analog-Digital Converter). Finally, the digital sample is converted in **LUX** using the ``` adc_util_map() ``` function inside the ``` analog_util.h ``` offered by the RIOT OS.  

## Network architecture

## Sources
* https://www.agraria.org/polli/ricoveri.htm
* https://www.poultryledlights.com/
* https://en.wikipedia.org/wiki/International_System_of_Units
