# Hands-on walkthrough
Before proceding, download the whole project repository from https://github.com/Salvo-32/IoT-IndividualAssignments/tree/main/SecondAssignment in your local computer

## Building of monitoring profiles and experiment
``` salvo@ubuntu: ssh fava@saclay.iot-lab.info```
```fava@saclay: iotlab-auth -u fava -p +++++++++++```
```fava@saclay: iotlab-profile adda8 -n A8PowerMonitoring1 --power dc -voltage -current -power -period 8244 -avg 4```
```fava@saclay: iotlab-profile addm3 -n M3Monitoring1 --power dc -voltage -current -power -period 8244 -avg 4 -rssi -channels 22 26 -num 1 -rperiod 1000```
```fava@saclay: iotlab-experiment submit -n Henhouse -d 120 -l saclay,a8,2 -l saclay,m3,1+2+4-12```
```fava@saclay: iotlab-experiment wait```
```fava@saclay: iotlab-node --profile A8PowerMonitoring1 -l saclay,a8,2```
```fava@saclay: iotlab-node --profile M3Monitoring1 -l saclay,m3,1+2+4-12```

## Implementation of Eclipse Mosquitto - A8 MQTTS Broker + MQTT Transparent bridge
```salvo@ubuntu: scp -r IoT/IndividualAssignment/02/mosquitto/ fava@saclay.iot-lab.info:/senslab/users/fava/shared```
```salvo@ubuntu: ssh fava@saclay.iot-lab.info```
```fava@saclay: scp -r /senslab/users/fava/shared/mosquitto root@node-a8-2:/etc```
```fava@saclay: ssh root@node-a8-2```
```root@node-a8-2: mosquitto -v -c /etc/mosquitto/conf.d/bridge.conf```

## Implementation of RSMB Broker
```scp IoT/IndividualAssignment/02/rsmb_mqtts_broker.conf fava@saclay.iot-lab.info:/senslab/users/fava/shared```
```salvo@ubuntu: ssh fava@saclay.iot-lab.info```
```fava@saclay: ssh root@node-a8-2```
```fava@saclay: broker_mqtts shared/rsmb_mqtts_broker.conf ```

## Deployment of M3 border router
```salvo@ubuntu: cd IoT/IndividualAssignment/02/Firmware/BorderRouter```
```salvo@ubuntu: make ETHOS_BAUDRATE=500000 DEFAULT_CHANNEL=26 BOARD=iotlab-m3```
```salvo@ubuntu: scp ./bin/iotlab-m3/gnrc_border_router.elf fava@saclay.iot-lab.info:/senslab/users/fava/shared/```
```salvo@ubuntu: ssh fava@saclay.iot-lab.info```
```fava@saclay: sudo ethos_uhcpd.py m3-1 tap0 2001:660:3207:04c1::1/64```
```fava@saclay: iotlab-auth -u fava -p 328uS4r@iOTLAB```
```fava@saclay: iotlab-node --flash /senslab/users/fava/shared/gnrc_border_router.elf -l saclay,m3,1```

## Deployment of M3 simpleNode (endpoints)
```salvo@ubuntu: scp /IoT/IndividualAssignment/02/Firmware/SimpleNode/iotlab-m3/5/bin/iotlab-m3/Henhouse_4.elf fava@saclay.iot-lab.info:/senslab/users/fava/shared```
```salvo@ubuntu: ssh -X fava@saclay.iot-lab.info```
```fava@saclay: iotlab-auth -u fava -p 328uS4r@iOTLAB```
```fava@saclay: iotlab-node --flash /senslab/users/fava/shared/Henhouse_4.elf -l saclay,m3,2+4-12```
```fava@saclay: nc m3-2 20000```

## Evaluation of performance - Power monitoring
```salvo@ubuntu: ssh -X fava@saclay.iot-lab.info```
```fava@saclay: less ~/.iot-lab/last/consumption/m3_1.oml```
```fava@saclay: plot_oml_consum --input ~/.iot-lab/272563/consumption/m3_1.oml --power --label "Border router (m3-1) - Power consumption analysis - Exp ID 272563"```
```fava@saclay: plot_oml_consum --input ~/.iot-lab/272564/consumption/m3_1.oml --power --label "Border router (m3-1) - Power consumption analysis - Exp ID 272564"```

## Evaluation of performance - Channel monitoring
```salvo@ubuntu: ssh -X fava@saclay.iot-lab.info # -X option enables X11 forwarding```
```fava@saclay: plot_oml_radio --input ~/.iot-lab/272563/radio/m3_1.oml --label "Border router (m3-1) - Wireless channel analysis - Exp ID 272563" --all```
```fava@saclay: plot_oml_radio --input ~/.iot-lab/272564/radio/m3_1.oml --label "Border router (m3-1) - Wireless channel analysis - Exp ID 272564" --plot```
