#include <stdlib.h>

#include "Arduino.h"
#include "comms.hpp"
#include "main.hpp"

#ifdef COMM_BOARD
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#endif

Comms *commRelay;

uint32_t delayMS;

void setup() {
    Serial.begin(9600);
    commRelay = new Comms();
#ifdef COMM_BOARD
    pinMode(COMM_LED, OUTPUT);
    digitalWrite(COMM_LED, LOW);
#endif
}

long airTime = 0;
long waterTime = 0;
long humTime = 0;

void loop() {
    long nowTime = millis();
    commRelay->readCommand();
#ifdef COMM_BOARD
    if (!commRelay->isConnected()) {
        commRelay->reconnect();
    }
    commRelay->loop();

    if (nowTime - airTime > 1000 * 60) {
        airTime = nowTime;
        commRelay->getAirTemperature();
    }

    if (nowTime - waterTime > 1000 * 60) {
        waterTime = nowTime;
        commRelay->getWaterTemperature();
    }

    if (nowTime - humTime > 1000 * 60) {
        humTime = nowTime;
        commRelay->getAirHumidity();
    }
#endif
}

#ifdef COMM_BOARD
void mqttCallback(char *topic, byte *payload, unsigned int length) {
    if (!strncmp(topic, filter_set_topic, strlen(filter_set_topic))) {
        if ((char)payload[0] == '1') {
            Serial.print("@N1#");
            commRelay->mqttPublish(filter_topic, "1");
        } else {
            Serial.print("@F1#");
            commRelay->mqttPublish(filter_topic, "0");
        }
    }

    if (!strncmp(topic, light_set_topic, strlen(light_set_topic))) {
        if ((char)payload[0] == '1') {
            Serial.print("@N2#");
            delay(50);
            Serial.print("@LW#");
            commRelay->mqttPublish(light_topic, "1");
        } else {
            Serial.print("@F2#");
            delay(50);
            Serial.print("@LX#");
            commRelay->mqttPublish(light_topic, "0");
        }
    }

    if (!strncmp(topic, light_brightness_set_topic, strlen(light_brightness_set_topic))) {
        Serial.print("@LB");
        char tmp[10];
        memcpy(tmp, payload, length);
        tmp[length] = '\0';
        Serial.print(tmp);
        Serial.print("#");
        commRelay->mqttPublish(light_brightness_topic, tmp);
    }
}
#endif
