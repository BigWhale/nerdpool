#ifndef main_hpp
#define main_hpp

#ifdef COMM_BOARD
    #include <ESP8266WiFi.h>
    #include <PubSubClient.h>
    void mqttCallback(char *topic, byte *payload, unsigned int length);
#endif

#endif
