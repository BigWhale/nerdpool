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
  #ifdef COMM_BOARD
      pinMode(COMM_LED, OUTPUT);
      pinMode(MQTT_LED, OUTPUT);
      digitalWrite(COMM_LED, LOW);
      digitalWrite(MQTT_LED, LOW);
  #endif
    Serial.begin(9600);
    delay(50);
    commRelay = new Comms();
}

long airTime = 0;
long waterTime = 0;
long humTime = 0;
long pressTime = 0;
long altTime = 0;

void loop() {
    commRelay->readCommand();
#ifdef COMM_BOARD
    long nowTime = millis();
    if (!commRelay->isConnected()) {
        commRelay->reconnect();
    }
    commRelay->loop();

    if (nowTime - airTime > 1000 * 60) {
        airTime = nowTime;
        commRelay->getAirTemperature();
    }

    if (nowTime - pressTime > 1000 * 61) {
        pressTime = nowTime;
        commRelay->getAirPressure();
    }

    if (nowTime - waterTime > 1000 * 62) {
        waterTime = nowTime;
        commRelay->getWaterTemperature();
    }

    if (nowTime - humTime > 1000 * 63) {
        humTime = nowTime;
        commRelay->getAirHumidity();
    }

    if (nowTime - altTime > 1000 * 64) {
        altTime = nowTime;
        commRelay->getAltitude();
    }

#endif
}

#ifdef COMM_BOARD
void mqttCallback(char *topic, byte *payload, unsigned int length) {
    char c_on[] = "1";
    char c_off[] = "0";
    if (!strncmp(topic, filter_set_topic, strlen(filter_set_topic))) {
        if ((char)payload[0] == '1') {
            Serial.print("@N2#");
            commRelay->mqttPublish(filter_topic, c_on);
        } else {
            Serial.print("@F2#");
            commRelay->mqttPublish(filter_topic, c_off);
        }
    }

    if (!strncmp(topic, light_set_topic, strlen(light_set_topic))) {
        if ((char)payload[0] == '1') {
            Serial.print("@N1#");
            delay(100);
            Serial.print("@LW#");
            commRelay->mqttPublish(light_topic, c_on);
        } else {
            Serial.print("@F1#");
            delay(100);
            Serial.print("@LX#");
            commRelay->mqttPublish(light_topic, c_off);
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

    if (!strncmp(topic, light_mode_topic, strlen(light_mode_topic))) {
      char tmp[150];
      memcpy(tmp, payload, length);
      tmp[length] = '\0';

      if (strcmp(tmp, "White") == 0) {
        Serial.print("@M1#");
      } else if (strcmp(tmp, "Rainbow") == 0) {
        Serial.print("@M2#");
      } else if (strcmp(tmp, "Theater") == 0) {
        Serial.print("@M3#");
      } else if (strcmp(tmp, "Scanner") == 0) {
        Serial.print("@M4#");
      } else if (strcmp(tmp, "Fade") == 0) {
        Serial.print("@M5#");
      }
    }
}
#endif

#ifdef CONTROL_BOARD
void CycleComplete() {
  commRelay->PatternComplete();
}
#endif
