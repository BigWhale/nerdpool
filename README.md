# NerdPool v0.01

Having a swimming pool in the back yard is a constant struggle. Here's where
a handful of electronics come in.


## The mission

- Automate water pump and filtration
- Measure water parameters
- Measure air temperature
- Give it an IP address - IoT **everything**


## Hardware

- Basics
 - Swimming pool
 - Filter
- Computers and micro controllers
 - ESP32 board
 - Raspberry Pi 2 or better (RPi 1 might also work?)
- Electronics
 - BME280 Temperature and humidty sensor
 - DS18B20 Waterproof sensor
 - Keyes 5V one channel relays


## Software

PlatformIO was used for building all the software that is running on an ESP
board. Apart from the standard Arduino library also these libraries
were used:

- Adafruit unified sensors (`pio lib install 31`)
- Adafruit BME280 Library (`pio lib install 166`)
- PubSubClient MQTT messaging library (`pio lib install 86`)
- OneWire (`pio lib install 1`)
- DallasTemperature library for DS18B20 sensor (`pio lib install 54`)

In my case RPi 3 is running [Home Assistant](https://www.home-assistant.io/hassio/) where all the data is sent to. 


## How it works

ESP32 is used to control all the relays, and read the sensors. After
connection to a WiFi it will subscribe to a few topics on the MQTT server
and it will use a few topics to publish data on it.

Home Assistant will display data collected and the state of relays.


## Compiling & Upload

- Install [Atom](https://atom.io) or [Visual Studio Code](https://code.visualstudio.com/)
- Install the `PlatformIO` plugin
- Start the IDE and open NerdPool project
- Open PlatformIO console and install all the prerequisites
- Hit build and cross your fingers

Upload should work with any ESP32 dev breakout board.


## Home Assistant Configuration

Switch example for configuration.yaml:

```
mqtt:
  broker: core-mosquitto
  discovery: true
switch:
  - platform: mqtt
    name: "Switch 1"
    state_topic: "sw_1"
    command_topic: "sw_1"
    payload_on: "1"
    payload_off: "0"
    retain: true
```

To simplyfy things, until you have a plethora of switches, it is the
easiest way to have one switch per MQTT topic. Switch names are hardcoded
as sw_1, sw_2, etc. Payload is 1 for on and 0 for off.

Sensor configuration:

```
  - platform: mqtt
    name: Pool Air Temperature
    state_topic: "air_temperature"
    unit_of_measurement: '°C'
  - platform: mqtt
    name: Pool Air Humidity
    state_topic: "air_humidity"
    unit_of_measurement: '%'
  - platform: mqtt
    name: Pool Air Pressure
    state_topic: "air_pressure"
    unit_of_measurement: 'mbar'
  - platform: mqtt
    name: Pool Water Temperature
    state_topic: "water_temperature"
    unit_of_measurement: '°C'
```
Again, topics hardcoded which greatly simplifies publishing, values are
just dumped out and HA will pick them up.


## Circuit board & electronics

Design is almost finished. Circuit board will include connectors for all
relay switches, five LED connectors (currently not completely utilized) and
four additional GPIO ports that will be configurable.

Currently I am using a relay board that has 4 relays on it. That should
be enough for most configurations with multiple pumps and lights. The
'problem' with this relay board are inverted signal pins. When you pull
the pin HIGH, the relay will disengage. When you pull pin LOW, the relay
will engage. That's why you have to set relay pins to high before you
set their mode to output.

Support for Neopixel lightstrip was dropped, because my lightstrip is
broken. ESP32 should be enough to control 5m strip without any trouble.
When I fix the lightstrip I'll add it back.

Circuit board is based on ESP32-WROOM-32 that comes with shielded chip and
a PCB antenna.

A lot of values are hardcoded in the source, I considered making them
configurable but for now you are stuck with changing the source if you
want to change the pin assignemnts.


## Bugs

Oh, loads ... Where do I start? ;>


## Future releases

It's a hobby pet project, I will probably tinker with it before the pool
season. Then forget about it. Right now the only reason this exists are the
[Webcamp SI](https://2017.webcamp.si) guys and girls that pushed me to rush
everything. :)

