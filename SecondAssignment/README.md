# IoT Second individual assignment
The goal of this assignment is to deploy a wireless sensor network, which consists of multiple MCUs, by using 802.15.4 mesh network technologies and 6LoWPAN  protocols, and meanwhile to evaluate the performance of the whole system. This time the system incorporates also the [first individual assignement](./../FirstAssignment) system, this means newer system is able to receive sensor values both from the **local** STM32 Nucleo F401RE and the **remote** wireless sensor network from FIT Iot-LAB

- For further information, please refer to http://ichatz.me/Site/InternetOfThings2021-Assignment2

## Premise
The whole wireless sensor network is implemented inside the [Saclay deployment](https://www.iot-lab.info/docs/deployment/saclay/) from FIT IoT-LAB for the following reasons:
1. Light sensors are above the floor. Despite [Grenoble site](https://www.iot-lab.info/docs/deployment/grenoble/) offers the biggest number of M3 nodes, i.e. 380 nodes, all sensors are located under an access floor, so completely in the dark. On this IoT-LAB site, the light sensors won't return usable values (always 0) for this experiment. Conversely, on Saclay site, the boards are located above the floor, therefore they provide useful LUX values.
2. Shorter waits to perform experiments. This individual assignment requires mainly [IoT-LAB M3](https://www.iot-lab.info/docs/boards/iot-lab-m3/) nodes and  Saclay site is quite often **free** with respect to the number of users which make use of this kind of board.
 
The following points address the new structure/diagram of the IoT system and its performance:

## How is the deployment of ***multiple sensors*** going to affect the IoT platform
Deploying multiple [IoT-LAB M3](https://www.iot-lab.info/docs/boards/iot-lab-m3/) boards, each one providing the **same** sensors (temperature and light) as the ones of the first individual assignment, sureley implies a bigger amount of data available. This aspect leads several advantages and disadvantages.
### Advantages of use multiple overlapping sensors
1. Temperature and light are [scalar physical quantities](https://en.wikipedia.org/wiki/Scalar_(physics)) therefore they change according to environmental position, namely with respect to the point in which the sensor performs measurement. Several differetn sensors spread all over the building surface on Saclay site provide the best average temperature for the entire building. In this Data quality
2. Fault tolerance
3. Energy efficiency 
### Disadvantages of employ multi-hop wireless network
1. sdfds
2. fgsfdg
3. efwesfe

## What are the connected components, the protocols to connect them and the overall IoT architecture
### Network diagram (Physical devices and Protocols)

### Overall high-level architecture diagram of Software components
![OverallArchitecture](./Picture/OverallArchitecture.jpg)

## How do you measure the performance of the system
Each of the following points measure the performance of the whole IoT system in term of Network performance and Energy efficiency as the number of wireless node increase and their physical locations vary. 
### 5 wireless nodes as in series network topology 
#### Network performance

#### Energy consumption

### 5 wireless nodes as mesh network topology 
#### Network performance

#### Energy consumption

### 10 wireless nodes as in-series network topology 
#### Network performance

#### Energy consumption

### 10 wireless nodes as mesh network topology
#### Network performance

#### Energy consumption

### Conclusion


