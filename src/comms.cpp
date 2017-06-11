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
#include <Wire.h>
#include <SPI.h>
#include "lights.hpp"
#endif

#ifdef COMM_BOARD
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#endif

Comms::Comms() {
#ifdef CONTROL_BOARD
    airSensor = new Adafruit_BME280;
    oneWire = new OneWire(WATERPIN);
    waterSensor = new DallasTemperature(oneWire);
    lightStrip = new NeoPatterns(LEDS, LIGHTPIN, NEO_GRB + NEO_KHZ800, NULL);

    airSensor->begin();
    waterSensor->begin();

    lightStrip->begin();
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
#ifdef CONTROL_BOARD
    lightStrip->Update();
#endif
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
    int light_mode;
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
        case 'P':       // Report air pressure
            reportAirPressure();
            break;
        case 'G':       // Report altitude
            reportAltitude();
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
                lightStrip->ActivePattern = NONE;
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

        // Light mode
        case 'M':
            light_mode = buffer[1] - '0' - 1;
            switch (light_mode) {
              case 0:
                lightStrip->ActivePattern = NONE;
                lightWhite();
                break;
              case 1:
                lightStrip->ActivePattern = RAINBOW_CYCLE;
                lightStrip->Index = 0;
                lightStrip->Interval = 5;
                lightStrip->TotalSteps = 255;
                lightStrip->Direction = FORWARD;
                break;
              case 2:
                lightStrip->ActivePattern = THEATER_CHASE;
                lightStrip->Index = 0;
                lightStrip->Interval = 100;
                lightStrip->TotalSteps = lightStrip->numPixels();
                lightStrip->Color1 = lightStrip->Color(255, 0, 0);
                lightStrip->Color2 = lightStrip->Color(0, 255, 0);
                lightStrip->Direction = FORWARD;
                break;
              case 3:
                lightStrip->ActivePattern = SCANNER;
                lightStrip->Index = 0;
                lightStrip->Interval = 10;
                lightStrip->TotalSteps = (lightStrip->numPixels() - 1) * 2;
                lightStrip->Color1 = lightStrip->Color(0, 0, 255);
                break;
              case 4:
                  lightStrip->ActivePattern = FADE;
                  lightStrip->Index = 0;
                  lightStrip->Interval = 5;
                  lightStrip->TotalSteps = 255;
                  lightStrip->Color1 = lightStrip->Color(0, 0, 255);
                  lightStrip->Color1 = lightStrip->Color(255, 0, 0);
                  break;
            }
            break;

    }
    resetBuffer();
}


void Comms::lightOff() {
    lightStrip->FullOff();
}

void Comms::lightWhite() {
  lightStrip->FullWhite();
}

void Comms::reportAirTemperature() {
    Serial.print("$A");
    Serial.print(airSensor->readTemperature());
    Serial.print("#");
}

void Comms::reportAirHumidity() {
  Serial.print("$H");
  Serial.print(airSensor->readHumidity());
  Serial.print("#");
}

void Comms::reportAirPressure() {
  Serial.print("$P");
  Serial.print(airSensor->readPressure() / 100.0F);
  Serial.print("#");
}

void Comms::reportAltitude() {
  Serial.print("$G");
  Serial.print(airSensor->readAltitude(SEALEVELPRESSURE_HPA));
  Serial.print("#");
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
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    delay(50);
    digitalWrite(COMM_LED, HIGH);
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
            mqttClient->subscribe(light_mode_topic);
            digitalWrite(MQTT_LED, HIGH);
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
        case 'P':       // Read air pressure
            handleAirPressure(buffer);
            break;
        case 'G':       // Read altitude
            handleAltitude(buffer);
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

void Comms::getAirPressure() {
    Serial.print("@P#");
}

void Comms::getWaterTemperature() {
    Serial.print("@W#");
}

void Comms::getAltitude() {
    Serial.print("@G#");
}

void Comms::handleAirTemperature(char *buf) {
    mqttClient->publish(atemp_topic, buf, true);
}

void Comms::handleAirHumidity(char *buf) {
    mqttClient->publish(ahum_topic, buf, true);
}

void Comms::handleAirPressure(char *buf) {
    mqttClient->publish(apres_topic, buf, true);
}

void Comms::handleWaterTemperature(char *buf) {
    mqttClient->publish(wtemp_topic, buf, true);
}

void Comms::handleAltitude(char *buf) {
    mqttClient->publish(alt_topic, buf, true);
}

void Comms::setPoolFilter(bool state) {

}

void Comms::setPoolLight(bool state) {

}
#endif
