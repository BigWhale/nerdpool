#ifndef comms_h
#define comms_h

#ifdef CONTROL_BOARD
    #include <OneWire.h>
    #include <DallasTemperature.h>
    #include <Adafruit_NeoPixel.h>
    #include <Adafruit_BME280.h>
    #include "lights.hpp"
#endif

#ifdef COMM_BOARD
    #include <ESP8266WiFi.h>
    #define wifi_ssid "Lubica.WiFi2"
    #define wifi_password "ZunkoCar"

    #include <PubSubClient.h>
    #define mqtt_server "10.0.0.240"
    #define mqtt_user "homeassistant"
    #define mqtt_pass "ZunkoCar"

    #define atemp_topic (char *)"home/pool/atemp"
    #define ahum_topic (char *)"home/pool/ahum"
    #define apres_topic (char *)"home/pool/apres"
    #define wtemp_topic (char *)"home/pool/wtemp"

    #define alt_topic (char *)"home/pool/alt"

    #define filter_topic (char *)"home/pool/filter"
    #define filter_set_topic (char *)"home/pool/filter/set"

    #define light_topic (char *)"home/pool/light"
    #define light_mode_topic (char *)"home/pool/light/mode"
    #define light_set_topic (char *)"home/pool/light/set"

    #define light_brightness_topic (char *)"home/pool/light/brightness"
    #define light_brightness_set_topic (char *)"home/pool/light/brightness/set"
#endif

// Hardware stuff
#define LIGHTPIN        6
#define LEDS            294

#define RELAY0PIN       7
#define RELAY1PIN       8
#define RELAY2PIN       9

#define WATERPIN        10

#define SEALEVELPRESSURE_HPA (1013.25)

#define ON 1
#define OFF 0

// Software stuff

#define SOC '@'
#define EOC '#'

#define SOD '$'
#define EOD '#'


#define MAX_BUFFER 30
#define NUM_RELAYS 3

#define COMM_LED D0
#define MQTT_LED D1

class Comms {
public:
    Comms();
    void readCommand();
    void sendCommand();
    void readResponse();
    void resetBuffer();
#ifdef CONTROL_BOARD
    void PatternComplete();
#endif

#ifdef COMM_BOARD
    bool isConnected();
    void reconnect();
    void loop();

    void getAirTemperature();
    void getAirHumidity();
    void getAirPressure();
    void getWaterTemperature();
    void getAltitude();

    void setPoolFilter(bool state);
    void setPoolLight(bool state);

    void mqttPublish(char *topic, char *payload);
#endif

private:
    bool inCommand;
    bool inData;
    byte bufCnt;
    char buffer[MAX_BUFFER];

#ifdef CONTROL_BOARD
    int relays[3];
    OneWire *oneWire;
    DallasTemperature *waterSensor;
    NeoPatterns *lightStrip;
    Adafruit_BME280 *airSensor;

    // int reportRelayState(byte relay);
    // int toggleRelay(byte relay);
    void relayOn(byte relay);
    void relayOff(byte relay);

    void processCommand();
    void reportAirTemperature();
    void reportAirHumidity();
    void reportAirPressure();
    void reportAltitude();
    void reportWaterTemperature();

    void setLightMode(int mode);
    void resetStates();
#endif

#ifdef COMM_BOARD
    WiFiClient espClient;
    PubSubClient *mqttClient;

    void setupWiFi();
    void processData();
    void handleAirTemperature(char *buf);
    void handleAirHumidity(char *buf);
    void handleAirPressure(char *buf);
    void handleWaterTemperature(char *buf);
    void handleAltitude(char *buf);

#endif
};

#ifdef COMM_BOARD
  void mqttCallback(char *topic, byte *payload, unsigned int length);
#endif

#ifdef CONTROL_BOARD
  void CycleComplete();
#endif

#endif // comms.hpp
