#ifndef comms_h
#define comms_h

#ifdef CONTROL_BOARD
    #include <DHT_U.h>
    #include <OneWire.h>
    #include <DallasTemperature.h>
    #include <Adafruit_NeoPixel.h>
#endif

#ifdef COMM_BOARD
    #include <ESP8266WiFi.h>
    #define wifi_ssid "Lubica.WiFi2"
    #define wifi_password "ZunkoCar"

    #include <PubSubClient.h>
    #define mqtt_server "10.0.0.240"
    #define mqtt_user "homeassistant"
    #define mqtt_pass "ZunkoCar"

    #define atemp_topic "home/pool/atemp"
    #define ahum_topic "home/pool/ahum"
    #define wtemp_topic "home/pool/wtemp"

    #define filter_topic "home/pool/filter"
    #define filter_set_topic "home/pool/filter/set"

    #define light_topic "home/pool/light"
    #define light_set_topic "home/pool/light/set"

    #define light_brightness_topic "home/pool/light/brightness"
    #define light_brightness_set_topic "home/pool/light/brightness/set"
#endif

// Hardware stuff
#define LIGHTPIN        6
#define LEDS            300

#define RELAY0PIN       7
#define RELAY1PIN       8
#define RELAY2PIN       9

#define AIRPIN          10
#define WATERPIN        11

#define AIRTYPE         DHT22

#define ON 1
#define OFF 0

// Software stuff

#define SOC '@'
#define EOC '#'

#define SOD '$'
#define EOD '#'


#define MAX_BUFFER 255
#define MAX_COUNT 254
#define NUM_RELAYS 3

#define COMM_LED 0

class Comms {
public:
    Comms();
    void readCommand();
    void sendCommand();
    void readResponse();
    void resetBuffer();
#ifdef CONTROL_BOARD
    void lightOff();
    void lightWhite();
#endif

#ifdef COMM_BOARD
    bool isConnected();
    void reconnect();
    void loop();

    void getAirTemperature();
    void getAirHumidity();
    void getWaterTemperature();

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
    DHT_Unified *airSensor;
    OneWire *oneWire;
    DallasTemperature *waterSensor;
    Adafruit_NeoPixel *lightStrip;

    int reportRelayState(byte relay);
    int toggleRelay(byte relay);
    void relayOn(byte relay);
    void relayOff(byte relay);

    void processCommand();
    void reportAirTemperature();
    void reportWaterTemperature();
    void reportAirHumidity();

    void resetStates();
#endif

#ifdef COMM_BOARD
    WiFiClient espClient;
    PubSubClient *mqttClient;

    void setupWiFi();
    void processData();
    void handleAirTemperature(char *buf);
    void handleAirHumidity(char *buf);
    void handleWaterTemperature(char *buf);

#endif
};

void mqttCallback(char *topic, byte *payload, unsigned int length);

#endif // comms.hpp
