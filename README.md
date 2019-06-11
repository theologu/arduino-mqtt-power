# arduino-mqtt-power
This code integrates an arduino nano with w5100 ethernet shield, a 30A/220V AC relay, a ACS712 20A current sensor, a flood sensor and a DHT22 temperature/humidity sensor all connected through MQTT to a Home Assistant server

This setup I use to monitor my basement environment and also control the water pump. Features:
- detects fault situation (pump is continuously running for more than 10 min)and sends it to HA
- calculates pump uptime (effective running time) in an configurable interval (in code is 2h) and sends it to HA
- cuts the power of the pump in fault conditions
- calculates power consumption and sends it to HA
- monitors temperature/humidity
- detects flood condition 

More automation and logic can be done on Home Assistant side.

![relay](relay.png?raw=true "Title")
![relay](acs712.png?raw=true "Title")


