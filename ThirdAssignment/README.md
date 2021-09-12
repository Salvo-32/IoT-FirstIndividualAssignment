# IoT Third individual assignment
The goal of this assignment is the deployment of a long-range low-power wide area network, made up of multiple MCUs ([ST B-L072Z-LRWAN1](https://www.st.com/en/evaluation-tools/b-l072z-lrwan1.html)), by using [LoRaWAN](https://lora-alliance.org/about-lorawan/) and [TheThingsNetwork](https://www.thethingsnetwork.org/) technologies. Simultaneously, like in previous assignments, performance of the whole system are evaluated. 
This time resulting system incorporates both the [first](./../FirstAssignment) and [second](./../SecondAssignment) individual assignment functionalities, this means newer system is able to receive sensor values from the:
1. **Local** STM32 Nucleo F401RE, 
2. **Remote** [802.15.4](https://en.wikipedia.org/wiki/IEEE_802.15.4) + [6LoWPAN](https://en.wikipedia.org/wiki/6LoWPAN) wireless mesh network at [FIT Iot-LAB](www.iot-lab.info)
3. **Remote** [LoRaWAN](https://lora-alliance.org/about-lorawan/) network at [FIT Iot-LAB](www.iot-lab.info)

Focus of the third assignment is the implementation of [IoT Edge Analytics](https://www.sisense.com/glossary/edge-analytics/)
- For further information, please refer to http://ichatz.me/Site/InternetOfThings2021-Assignment3

## Premise
The long-range low-power wide area network is implemented inside the [Saclay deployment](https://www.iot-lab.info/docs/deployment/saclay/) from FIT IoT-LAB for the following reasons:
1. As state in the [first assigment](https://github.com/Salvo-32/IoT-IndividualAssignments/tree/main/FirstAssignment#sensors), a temputerature sensor and a light sensor are needed. Since [ST B-L072Z-LRWAN1](https://www.st.com/en/evaluation-tools/b-l072z-lrwan1.html) board does not provide any kind of sensor (LoRa & SigFox communication module only), it needs a further expansion board (shield) on top it. The only site providing such a composed configuration is Saclay site, in fact here [ST B-L072Z-LRWAN1](https://www.iot-lab.info/docs/boards/st-b-l072z-lrwan1/) boards (st-lrwan1-1 to st-lrwan1-25) are equipped with the [ST X-NUCLEO-IKS01A2](https://www.st.com/en/ecosystems/x-nucleo-iks01a2.html) shield. This gives access to external sensors to the node, in particular to a temperature sensor HTS221.
- Despite use of the expansion board above, [ST B-L072Z-LRWAN1](https://www.st.com/en/evaluation-tools/b-l072z-lrwan1.html) board is unable to sense light from the environment, therefore a light sensor is simulated inside its firmware (look at [main.c](./Firmware/Endpoint/main.c))
 
The following points address the new structure/diagram of the IoT system and its performance:

## How is the long-range low-power wide area network going to affect the IoT platform?
1. Deployment of multiple sensors: Benefits
2. Limitations of LoRaWAN network, a narrow-band wireless network
3. Data aggregation operations
4. Comparison of data quality (min, max, avg) at Cloud level vs. Edge level

## What are the connected components, the protocold to connect them and the overall IoT architecture?
### Network diagram (Physical devices and Protocols)
The following diagram depicts all the physical devices employed in this project and relative network protocols used to interconnect each other
![PhysicalNetworkDiagram](./Picture/PhysicalNetworkDiagram.jpg)

**NOTE** First individual assignment's network components are neither depicted again in the diagram above nor these are taken into account in the description below. (Look at the appropriate [document](/FirstAssignment/README.md/####-Network-diagram-(Physical-devices-and-Protocols)))

From LEFT to RIGHT:
1. Aavailable [ST B-L072Z-LRWAN1](https://www.iot-lab.info/docs/boards/st-b-l072z-lrwan1/) boards (st-lrwan1-1 to st-lrwan1-25) on Saclay site make use of CM act as follow:
   1. ```m3-1.saclay``` acts as Border router namely it makes use of both 802.15.4 network technologies and 6LowPAN to interact with endpoints/simple nodes. Moreover like in the first assignment it uses an attached device/computer (in this case a mini-computer called Gateway (GW)) through ```ethos``` + ```UHCP``` to reach the IP network by FIT IoT-LAB. In fact as reported at [FIT IoT-LAB - Testbed Design](https://www.iot-lab.info/docs/getting-started/design/) each experimentation board called Open Node (ON) is integrated with a mini-computer called Gateway (GW), which in turn provide tools to evaluate BOARD performance and give access to the FIT IoT-LAB network.
   2. ```m3-2.saclay``` - ```m3-11.saclay```board as end-points or single nodes. 
      1. They sense temperature and light as in the previous assignment with same sampling rate, but using the integrated sensors ***LPS331AP*** and ***ISL29020*** Both the [IoT-LAB M3 web-page](https://www.iot-lab.info/docs/boards/iot-lab-m3/) and this [Official Jupyter Notebook](https://github.com/iot-lab/iot-lab-training/tree/master/riot/basics/sensors)) by FIT Iot-LAB provide detailed information about these two sensors and how to use them in [RIOT-OS](www.riot-os.org)
      2. Publish these values using MQTTS messages using 802.15.4 network interface within the 6LowPAN network.
      3. Hop-by-hop (node-by-node) MQTTS messages eventually reach first final destination: the Border router
2. [IoT-LAB A8-M3](https://www.iot-lab.info/docs/boards/iot-lab-a8-m3/), which behaves as the Ubuntu Laptop from the first assignment
   1. It is an MQTTS broker (RSMB) that receives insecure messages from the wireless mesh network endpoints, in turn bridged (transparent bridge) to the Eclipse Mosquitto MQTT Broker
   2. It is an MQTT broker (Eclipse Mosquitto) that receive insecure messages from RSMB and forward them using TLS toward Amazon MQTT broker
3. Amazon AWS MQTT broker, same as previous assignment, which exchange MQTT(TLS) messages with the above-mentioned [IoT-LAB A8-M3](https://www.iot-lab.info/docs/boards/iot-lab-a8-m3/)
4. Frontend devices and Web-dashboard that exchange requests and responses using HTTPS, same as previous assignment

 
In order to clearly understand how the physical devices and software components work together look at paragraph Overall high-level architecture diagram of the whole system

### Overall high-level architecture diagram of Software components
This section shows inter-dependencies among the different software components. While the same paragraph of the first assignment provides a detailed description of each single software, here only new components are described 
![OverallArchitecture](./Picture/OverallArchitecture.jpg)
1. A new RIOT firmware ```Henhouse_4.elf``` provided [here main.c](./Firmware/SimpleNode/iotlab-m3/main.c) for [IoT-LAB M3](https://www.iot-lab.info/docs/boards/iot-lab-m3/) boards which allow them to act as end-points. Several lines of code are comment ones which describe the behavior of the whole firmware, please look at it before you go on. 
   * Conversely, the local Nucleo F401RE board of the previous assignment still run the same firmware, sensing local environment and publishing MQTT messages just like before. As it is stated before, this board is still present in this assignment although this document does not take into account it.
   * **NOTE** In this assignment I prefer having two different firmwares for FIT IoT-LAB M3 boards and one for NUCLEO F401RE board because:
      * They implement two different physical pin configurations for sensors and actuators, in particular components from the first assignment make use of analog input interface and GPIO, conversely boards of the second assignment make use of drivers to connect to the desired sensors
      * They rely on different network parameters (different local mqtts broker port and address), moreover different networks technology and topology
2. The example firmware ```RIOT gnrc_border_router``` as it is described at [Official RIOT-OS GitHub repository](https://github.com/RIOT-OS/RIOT/tree/master/examples/gnrc_border_router), runs on top of the above-mentioned ```m3-1.saclay``` Border router
   * One change with respect to the official tutorial concerns how to build the offcial source code: ```make ETHOS_BAUDRATE=500000 BOARD=iotlab-m3 ```. That value of baudrate comes from the intrinsic UART baudrate of M3 with which the board reaches the IP Network by IoT-LAB.

3. Yocto OS + Eclipse RSMB + Eclipse Mosquitto run on top of [IoT-LAB A8-M3](https://www.iot-lab.info/docs/boards/iot-lab-a8-m3/) board, like in the previous asignment
   * **NOTE** Yocto OS replace Ubuntu 20.04 of the first assignment, but behaves just like before. The [Yocto Project](https://www.iot-lab.info/docs/os/yocto/) is an open-source project which allows the creation of embedded Linux distributions. The project was announced by the Linux Foundation in 2010 and launched in March, 2011, in collaboration with 22 organizations. 
4. AWS Webservices
   1. AWS IoT Core adds a new Thing ```FITIoT-Lab```, which grants access to AWS IoT Core webservices using a new dedicated set of keys and certificates. 
      * To allow the local nucleo board and m3 boards access AWS IoT core at the same time, two different set of keys and certificates are rquired (One for each Thing)
      * Although current repository provides keys and certificates to access my Amazon AWS account at [Firmware/MqttBrokerBridge](./Firmware/MqttBrokerBridge) folder, these are currently not allowed from my personal account. If you want to try the whole system functioning, please contact me at ***Salvo.f96@gmail.com*** ![IotCoreThings](./Picture/IotCoreThings.png)
      
      * IoT Rules (Temperature and Light rule) are the same as before
   4. AWS DynamoDB the same as before
   5. AWS S3 hosts a new version of web-dashboard ```index2.html```, more additional details about its source code are provided below ![S3Bucket](./Picture/S3Bucket.png)
5.  Web-dashboard is a newer version which works with HTML5 and JS
   * It relies on [AWS JS SDK](https://docs.aws.amazon.com/AWSJavaScriptSDK/latest/) only (more efficient than previous version with Lambda)! 
   * Using this SDK the dashboard make use of Amazon Cognito (as in the previous assignment) in order to access/authenticate AWS Webservices 
   * Using this SDK the dashboard interacts with DynamoDB, to retrieve temperature and light values
   * Using this SDK the dashboard to send MQTT message to Amazon AWS broker to interact with specifid actuaturs and devices
   * All the aggreagted computation (avg, min, max) are performed on the client side using JS functions
   * Further information about the source code are available at [index2.html](./Frontend/index2.html) 
   * The following screenshot shows the current version of the Dashboard ![index2](./Picture/index2.png)  

## How do you measure the performance of the system
Each of the following points measure the performance of the whole IoT system in term of Energy efficiency (Power/Current/Voltage monitoring) and Wireless channel analysis, as the number of wireless node increase and consequently their physical locations vary. 
1. Power monitoring follows the official [Consumption monitoring guidelines](https://www.iot-lab.info/docs/tools/consumption-monitoring/) by FIT IoT-LAB, , please look at it to set up monitoring activity ![A8PowerMonitoring1](./Evaluation/Picture/A8PowerMonitoring1.png)
3. Wireless channel analysis follows the official [Radio monitoring guidelines](https://www.iot-lab.info/docs/tools/radio-monitoring/) by FIT IoT-LAB, please look at it to set up monitoring activity ![M3Monitoring1](./Evaluation/Picture/M3Monitoring1.png)
4. Here there is the list used to set up profiles above  (they take place at fava@saclay):
   1. ```iotlab-profile adda8 -n A8PowerMonitoring1 --power dc -voltage -current -power -period 8244 -avg 4```
   2. ```iotlab-profile addm3 -n M3Monitoring1 --power dc -voltage -current -power -period 8244 -avg 4 -rssi -channels 22 26 -num 1 -rperiod 1000```
5. Power monitoring concerns both A8 node and M3 nodes, while Wireless channel analysis concerns M3 boards only since they are the only one using 802.15.4 wireless technology
6. For each experiment, wireless channel employed is the **26** one for every m3 board (Multi-hop wireless network) and it is GRAPHICALLY evaluated only at Border router because all the wireless devices make use of the same channel.. Textual results are available at [Evaluation folder](./Evaluation) of this repository, and they include channel analysis for endpoints.
7.Next sections carry out graphical outcomes about the monitoring activities, for each node. 

### Experiment 272564 - 5 wireless nodes 
The IoT-LAB experiment 272564 takes place in Saclay site, involving:
* **5** endpoint nodes ```m3-2.saclay```, ```m3-4.saclay``` **-** ```m3-7.saclay``` 
* 1 border router ```m3-1.saclay```
* 1 MQTT bridge ```a8-2.saclay```

To show the following result, the following shell command are used:
1. ```iotlab-experiment submit -n Henhouse -d 120 -l saclay,a8,2 -l saclay,m3,1+2+4-7```
2. ```iotlab-node --profile A8PowerMonitoring1 -l saclay,a8,2```
3. ```iotlab-node --profile M3Monitoring1 -l saclay,m3,1+2+4-7```
4. ```plot_oml_consum --input ~/.iot-lab/272563/consumption/a8_2.oml --power --label "A8-2 - Power consumption analysis - Exp ID 272563"``` ![272563_a8-2_PowerMonitoring](./Evaluation/Picture/272563_a8-2_PowerMonitoring.png)
5. ```plot_oml_consum --input ~/.iot-lab/272563/consumption/m3_1.oml --power --label "Border router (m3-1) - Power consumption analysis - Exp ID 272563"``` ![272563_m3-1_PowerMonitoring](./Evaluation/Picture/272563_m3-1_PowerMonitoring.png)
6. ```plot_oml_radio --input ~/.iot-lab/272563/radio/m3_1.oml --label "Border router (m3-1) - Wireless channel analysis - Exp ID 272563" --all``` ![272563_m3-1_ChannelMonitoring](./Evaluation/Picture/272563_m3-1_ChannelMonitoring.png)
7. ```plot_oml_consum --input ~/.iot-lab/272563/consumption/m3_2.oml --power --label "Endpoint (m3-2) - Power consumption analysis - Exp ID 272563"``` ![272563_m3-2_PowerMonitoring](./Evaluation/Picture/272563_m3-2_PowerMonitoring.png)
8. ```plot_oml_consum --input ~/.iot-lab/272563/consumption/m3_4.oml --power --label "Endpoint (m3-4) - Power consumption analysis - Exp ID 272563"``` ![272563_m3-4_PowerMonitoring](./Evaluation/Picture/272563_m3-4_PowerMonitoring.png)
9. ```plot_oml_consum --input ~/.iot-lab/272563/consumption/m3_5.oml --power --label "Endpoint (m3-5) - Power consumption analysis - Exp ID 272563"```![272563_m3-5_PowerMonitoring](./Evaluation/Picture/272563_m3-5_PowerMonitoring.png)
10. ```plot_oml_consum --input ~/.iot-lab/272563/consumption/m3_6.oml --power --label "Endpoint (m3-6) - Power consumption analysis - Exp ID 272563"```![272563_m3-6_PowerMonitoring](./Evaluation/Picture/272563_m3-6_PowerMonitoring.png)
11. ```plot_oml_consum --input ~/.iot-lab/272563/consumption/m3_7.oml --power --label "Endpoint (m3-7) - Power consumption analysis - Exp ID 272563"```![272563_m3-7_PowerMonitoring](./Evaluation/Picture/272563_m3-7_PowerMonitoring.png)

### 10 wireless nodes
The IoT-LAB experiment 272564 takes place in Saclay site, involving:
* **10** endpoint nodes ```m3-2.saclay```, ```m3-4.saclay``` **-** ```m3-12.saclay``` 
* 1 border router ```m3-1.saclay```
* 1 MQTT bridge ```a8-2.saclay```

To show the following result the following shell command are used:
1. ```iotlab-experiment submit -n Henhouse -d 120 -l saclay,a8,2 -l saclay,m3,1+2+4-12```
2. ```iotlab-node --profile A8PowerMonitoring1 -l saclay,a8,2```
3. ```iotlab-node --profile M3Monitoring1 -l saclay,m3,1+2+4-12```
4. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/a8_2.oml --power --label "A8-2 - Power consumption analysis - Exp ID 272564"``` ![272564_a8-2_PowerMonitoring](./Evaluation/Picture/272564_a8-2_PowerMonitoring.png)
5. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_1.oml --power --label "Border router (m3-1) - Power consumption analysis - Exp ID 272564"``` ![272564_m3-1_PowerMonitoring](./Evaluation/Picture/272564_m3-1_PowerMonitoring.png)
6. ```plot_oml_radio --input ~/.iot-lab/272564/radio/m3_1.oml --label "Border router (m3-1) - Wireless channel analysis - Exp ID 272564" --all``` ![272564_m3-1_ChannelMonitoring](./Evaluation/Picture/272564_m3-1_ChannelMonitoring.png)
7. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_2.oml --power --label "Endpoint (m3-2) - Power consumption analysis - Exp ID 272564"``` ![272564_m3-2_PowerMonitoring](./Evaluation/Picture/272564_m3-2_PowerMonitoring.png)
8. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_4.oml --power --label "Endpoint (m3-4) - Power consumption analysis - Exp ID 272564"``` ![272564_m3-4_PowerMonitoring](./Evaluation/Picture/272564_m3-4_PowerMonitoring.png)
9. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_5.oml --power --label "Endpoint (m3-5) - Power consumption analysis - Exp ID 272564"```![272564_m3-5_PowerMonitoring](./Evaluation/Picture/272564_m3-5_PowerMonitoring.png)
10. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_6.oml --power --label "Endpoint (m3-6) - Power consumption analysis - Exp ID 272564"```![272564_m3-6_PowerMonitoring](./Evaluation/Picture/272564_m3-6_PowerMonitoring.png)
11. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_7.oml --power --label "Endpoint (m3-7) - Power consumption analysis - Exp ID 272564"```![272564_m3-7_PowerMonitoring](./Evaluation/Picture/272564_m3-7_PowerMonitoring.png)
12. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_8.oml --power --label "Endpoint (m3-8) - Power consumption analysis - Exp ID 272564"```![272564_m3-8_PowerMonitoring](./Evaluation/Picture/272564_m3-8_PowerMonitoring.png)
13. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_9.oml --power --label "Endpoint (m3-9) - Power consumption analysis - Exp ID 272564"```![272564_m3-9_PowerMonitoring](./Evaluation/Picture/272564_m3-9_PowerMonitoring.png)
14. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_10.oml --power --label "Endpoint (m3-10) - Power consumption analysis - Exp ID 272564"```![272564_m3-10_PowerMonitoring](./Evaluation/Picture/272564_m3-10_PowerMonitoring.png)
15. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_11.oml --power --label "Endpoint (m3-11) - Power consumption analysis - Exp ID 272564"```![272564_m3-11_PowerMonitoring](./Evaluation/Picture/272564_m3-11_PowerMonitoring.png)
16. ```plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_12.oml --power --label "Endpoint (m3-12) - Power consumption analysis - Exp ID 272564"```![272564_m3-12_PowerMonitoring](./Evaluation/Picture/272564_m3-12_PowerMonitoring.png)

### Conclusion
1. As you can infer from the graphs above as the number of wireless nodes increases ```m3-1.saclay``` border router's power consumption increases. In particular, its first power consumption graph (m3-1.saclay) shows few peaks around 0.12 W with an average value of 0.11 W, conversely the second one shows much more peaks around 0.14 W and an average values of 0.12 W.
2. Since during both experiments there were not running experiments on Saclay site, border router's wireless channel analysis shows an RSSI near -91 dBm without noise. This means transceiver were able to exchange data without external noise in that frequency band (channel 26). Nevertheless depending on the radio environment perturbations, you could measure worse RSSI values with relative noise, that will damage wireless links.
3. As stated in the [Disadvantages of a multi-hop wireless network](./README.md#disadvantages-of-a-multi-hop-wireless-network) above, deploying multiple IoT-LAB M3 boards, surely implies a bigger amount of data available to be handled. It is absolutely clear looking at both the graphs of MQTTS/MQTT broker ```a8-2.saclay```, they show an harder power consumption activity that grows as the number of MQTTS message grows (second experiment)

