#include <stdlib.h>

#include "main.hpp"
#include "comms.hpp"
#include "Arduino.h"

#ifdef CONTROL_BOARD
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#endif

#ifdef COMM_BOARD
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#endif

Comms::Comms() {
#ifdef CONTROL_BOARD
    airSensor = new DHT_Unified(AIRPIN, AIRTYPE);
    oneWire = new OneWire(WATERPIN);
    waterSensor = new DallasTemperature(oneWire);
    lightStrip = new Adafruit_NeoPixel(LEDS, LIGHTPIN, NEO_GRB + NEO_KHZ800);

    airSensor->begin();
    waterSensor->begin();

    lightStrip->begin();
    lightStrip->setBrightness(190);
    lightStrip->show();

    relays[0] = RELAY0PIN;
    relays[1] = RELAY1PIN;
    relays[2] = RELAY2PIN;

    for (int i = 0; i < NUM_RELAYS; i++) {
        pinMode(relays[i], OUTPUT);
    }
    resetStates();
#endif
#ifdef COMM_BOARD
    setupWiFi();
#endif
    resetBuffer();
}

void Comms::readCommand() {
    while (Serial.available() > 0) {
        unsigned char inByte = Serial.read();

#ifdef CONTROL_BOARD
        if (!inCommand && inByte == SOC) {
            inCommand = true;
            continue;
        }

        if (inCommand && inByte == EOC) {
            buffer[bufCnt++] = '\0';
            inCommand = false;
            processCommand();
            break;
        }
#endif
#ifdef COMM_BOARD
        if (!inData && inByte == SOD) {
            inData = true;
            continue;
        }

        if (inData && inByte == EOD) {
            buffer[bufCnt++] = '\0';
            inData = false;
            processData();
            break;
        }
#endif
        if ((inCommand || inData) && (bufCnt < MAX_BUFFER) && isprint(inByte)) {
            buffer[bufCnt++] = inByte;
            buffer[bufCnt] = '\0';
        } else {
#ifdef CONTROL_BOARD
            if (inCommand) {
                inCommand = false;
                buffer[MAX_BUFFER] = '\0';
                processCommand();
                break;
            }
#endif
#ifdef COMM_BOARD
            if (inData) {
                inData = false;
                buffer[MAX_BUFFER] = '\0';
                processData();
                break;
            }
#endif
        }
    }
}

void Comms::resetBuffer() {
    buffer[0] = '\0';
    bufCnt = 0;
}

#ifdef CONTROL_BOARD
void Comms::resetStates() {
    for (int i = 0; i < NUM_RELAYS; i++) {
        digitalWrite(relays[i], LOW);
    }
}

void Comms::processCommand() {
    int relay;
    bool state;
    switch (buffer[0]) {
        // Relay commands
        case 'R':       // Report relay state
            relay = buffer[1] - '0' - 1;
            state = reportRelayState(relay);
            break;
        case 'T':       // Toggle relay state
            relay = buffer[1] - '0' - 1;
            state = toggleRelay(relay);
            break;
        case 'N':       // Close relay - Turn on
            relay = buffer[1] - '0' - 1;
            relayOn(relay);
            break;
        case 'F':       // Open relay - Turn off
            relay = buffer[1] - '0' - 1;
            relayOff(relay);
            break;

        // Sensor commands
        case 'A':       // Report air temperature
            reportAirTemperature();
            break;
        case 'W':       // Report water tmperature
            reportWaterTemperature();
            break;
        case 'H':      // Report air humidity
            reportAirHumidity();
            break;

        case 'X':
            if (buffer[1] == 'X') {
                resetStates();
            }
            break;
        // Lights
        case 'L':                           // Light mode control
            if (buffer[1] == 'W') {         // Turn on full white
                lightWhite();
            } else if (buffer[1] == 'B') {  // Set brigthness
                char tmpbuf[256] = "";
                int brightness = 0;
                memcpy(tmpbuf, buffer + 2, strlen(buffer) - 2);
                brightness = atoi(tmpbuf);
                lightStrip->setBrightness(brightness);
                lightStrip->show();
            } else if (buffer[1] == 'X') {  // Turn off all lights
                lightOff();
            }
            break;
    }
    resetBuffer();
}


void Comms::lightOff() {
    for(int i = 0; i < LEDS; i++) {
        lightStrip->setPixelColor(i, lightStrip->Color(0, 0, 0));
    }
    lightStrip->show();
}

void Comms::lightWhite() {
    for(int i = 0; i < LEDS; i++) {
        lightStrip->setPixelColor(i, lightStrip->Color(255, 255, 255));
    }
    lightStrip->show();
}

void Comms::reportAirTemperature() {
    sensors_event_t event;
    airSensor->temperature().getEvent(&event);
    if (!isnan(event.temperature)) {
        Serial.print("$A");
        Serial.print(event.temperature);
        Serial.print("#");
    }
}

void Comms::reportAirHumidity() {
    sensors_event_t event;
    airSensor->humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) {
        Serial.print("$H");
        Serial.print(event.relative_humidity);
        Serial.print("#");
    }
}

void Comms::reportWaterTemperature() {
    waterSensor->requestTemperatures();
    Serial.print("$W");
    Serial.print(waterSensor->getTempCByIndex(0));
    Serial.print("#");
}

int Comms::reportRelayState(byte relay) {
    return digitalRead(relays[relay]);
}

int Comms::toggleRelay(byte relay) {
    digitalWrite(relays[relay], !digitalRead(relays[relay]));
    return digitalRead(relays[relay]);
}

void Comms::relayOn(byte relay) {
    digitalWrite(relays[relay], HIGH);
}

void Comms::relayOff(byte relay) {
    digitalWrite(relays[relay], LOW);
}
#endif



/******************************************************************************
 *
 * Communications stuff
 *
 *****************************************************************************/


#ifdef COMM_BOARD
// Handlers for transmitter

void Comms::setupWiFi() {
    delay(250);
    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    mqttClient = new PubSubClient(espClient);
    mqttClient->setServer(mqtt_server, 1883);
    mqttClient->setCallback(mqttCallback);
}

bool Comms::isConnected() {
    return mqttClient->connected();
}

void Comms::reconnect() {
    while (!mqttClient->connected()) {
        if (mqttClient->connect("ESP8266Client", mqtt_user, mqtt_pass)) {
            mqttClient->subscribe(light_set_topic);
            mqttClient->publish(light_topic, "0", true);
            mqttClient->subscribe(filter_set_topic);
            mqttClient->publish(filter_topic, "0", true);
            mqttClient->subscribe(light_brightness_set_topic);
            digitalWrite(COMM_LED, HIGH);
        } else {
            delay(3000);
        }
    }
}

void Comms::loop() {
    mqttClient->loop();
}

void Comms::processData() {
    char tmp;
    tmp = buffer[0];
    memmove(buffer, buffer + 1, strlen(buffer + 1) + 1);
    switch (tmp) {
        case 'A':       // Read air temperature
            handleAirTemperature(buffer);
            break;
        case 'W':       // Read water temperature
            handleWaterTemperature(buffer);
            break;
        case 'H':       // Read air humidity
            handleAirHumidity(buffer);
            break;
    }
    resetBuffer();
}

void Comms::mqttPublish(char *topic, char *payload) {
    mqttClient->publish(topic, payload, true);
}

void Comms::getAirTemperature() {
    Serial.print("@A#");
}

void Comms::getAirHumidity() {
    Serial.print("@H#");
}

void Comms::getWaterTemperature() {
    Serial.print("@W#");
}

void Comms::handleAirTemperature(char *buf) {
    mqttClient->publish(atemp_topic, buf, true);
}

void Comms::handleAirHumidity(char *buf) {
    mqttClient->publish(ahum_topic, buf, true);
}

void Comms::handleWaterTemperature(char *buf) {
    mqttClient->publish(wtemp_topic, buf, true);
}

void Comms::setPoolFilter(bool state) {

}

void Comms::setPoolLight(bool state) {

}
#endif
