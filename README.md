# NerdPool v0.01

Having a swimming pool in the back yard is a constant struggle. Here's where
Arduino and handful of electronics come in.


## The mission

- Automate water pump and filtration
- Measure water parameters
- Measure air temperature
- Take care of lights
- Give it an IP address - IoT **everything**


## Hardware

- Basics
 - Swimming pool
 - Filter
 - Light bulb
- Computers and micro controllers
 - Arduino Nano board
 - ESP8266 ESP-12E board
 - Raspberry Pi 2 or better (RPi 1 might also work?)
- Electronics
 - DHT22 Temperature and humidty sensor
 - DS18B20 Waterproof sensor
 - Keyes 5V one channel relays
 - WS2812B 5050 RGB LED strip
 - a handful of resistors, capacitors, diodes, ...
- A decent power supply to power that LED strip


## Software

PlatformIO was used for building all the software that is running on Nano and
on ESP board. Apart from the standard Arduino library also these libraries
were used:

- Adafruit Neopixel library (`pio lib install 28`)
- Adafruit unified sensors (`pio lib install 18`)
- PubSubClient MQTT messaging library (`pio lib install 86`)
- DallasTemperature library for DS18B20 sensor (`pio lib install 54`)

Raspberry Pi 2 running [Home Assistant](https://home-assistant.io/) where all
the data is sent to.


## How it works

Arduino Nano is used to control all the relays, switches and read the sensors.
It will listen on I2C for incoming commands from ESP-12E board and report back
the sensor readings.

ESP-12E connects to the network and talks with RPi and Home Assistant. Commands
received on ESP are sent over MQTT.

Finally, Home Assistant will display data collected and the state of relays.

A few physical hard switches will be installed by the pool, so that you will be
able to control the lights without getting out of the pool.


## Compiling & Upload

- Install [Atom](https://atom.io)
- Install the `PlatformIO` plugin
- Start atom and open NerdPool project
- Open PlatformIO console and install all the prerequisites
- Hit build and cross your fingers

Upload is a bit trickier. Right now PlatformIO ini file assumes that both of
the boards are connected to the computer via USB. Edit the `platformio.ini`
and change the env values for each board. Then simply hitting the upload button
should work.

If you can't have two boards connected at the same time, then comment out one
of the env sections for each board. You will have to run compile & upload
for each board separately.


## Home Assistant Configuration

TBD sometime in the future. :/


## Bugs

Oh, loads ... Where do I start? ;>


## Future releases

It's a hobby pet project, I will probably tinker with it before the pool
season. Then forget about it. Right now the only reason this exists are the
[Webcamp SI](https://2017.webcamp.si) guys and girls that pushed me to rush
everything. :)
