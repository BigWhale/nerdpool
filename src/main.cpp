#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#include "main.hpp"

/* Sensors stuff */
Adafruit_BME280 airSensor; // Use I2C by default
#define SEALEVELPRESSURE_HPA (1013.25)
#define BME_ADDRESS 0x76  // This is usually 0x76 or 0x77 it depends on the sensor
#define BME_SDA 21  // Not really needed, just for the reference
#define BME_SCL 22  // Not really needed, just for the reference

#define ONEWIRE_PIN 27
OneWire oneWire(ONEWIRE_PIN);
DallasTemperature waterSensor(&oneWire);
DeviceAddress waterSensorAddress;

/* LEDS */
#define LED_BUILTIN 0
#define LED_RED     18
#define LED_AMBER   5
#define LED_GREEN   17
#define LED_BLUE    16
#define LED_YELLOW  4

int leds[5] = { LED_RED, LED_AMBER, LED_GREEN, LED_BLUE, LED_YELLOW};  

/* Relays */
#define RELAY_1 32
#define RELAY_2 33
#define RELAY_3 25
#define RELAY_4 26

int relay[4] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4};

/* Wireless and MQTT */
char ssid[] = "YOUR SSID";
char pass[] = "YOUR WPA";
int status = WL_IDLE_STATUS;
char mqttServer[] = "192.168.1.1";

WiFiClient espClient;
PubSubClient client(mqttServer, 1883, callback, espClient);

// Timekeeping
long mqttPing = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial) {} // Wait

  // Pin setup
  pinMode(LED_BUILTIN, OUTPUT);

  // Set default LED states
  digitalWrite(LED_BUILTIN, HIGH);  // Builtin LED needs to be set to LOW to turn it off
  for (int i = 0; i < 5; i++) {
    digitalWrite(leds[i], LOW);
    pinMode(leds[i], OUTPUT);
  }

  // Initialize relays
  for (int i = 0; i < 4; i++) {
    digitalWrite(relay[i], HIGH);
    pinMode(relay[i], OUTPUT);
  }

  flash_leds();

  // Set WIFI mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  // Connect to WiFi
  while ( status != WL_CONNECTED) { 
    digitalWrite(LED_AMBER, HIGH);
    digitalWrite(LED_RED, LOW);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  digitalWrite(LED_AMBER, LOW);

  // Star Wire protocol
  Wire.begin();
  while(!airSensor.begin(BME_ADDRESS)) {
    Serial.println("Could not find BME280I2C sensor!");
    delay(1000);
  }
  
  waterSensor.begin();
  Serial.print("Found ");
  Serial.print(waterSensor.getDeviceCount(), DEC);
  Serial.println(" sensor devices.");

  if (!waterSensor.getAddress(waterSensorAddress, 0)) {
    Serial.println("Unable to find address for water sensor."); 
  }
}

float airTemp, waterTemp, airHumidity, airPressure;

void loop() {    
  long now;
  char tmpStr[8];

  if (!client.connected()) {
    digitalWrite(LED_YELLOW, HIGH);
    reconnect();
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }
  client.loop();
  
  now = millis();

  if ( now - mqttPing > 5000 ) {
    mqttPing = now;
    airTemp = airSensor.readTemperature();
    airPressure = airSensor.readPressure() / 100.0F;
    airHumidity = airSensor.readHumidity();
    waterSensor.requestTemperatures();
    waterTemp = waterSensor.getTempC(waterSensorAddress);
   
    dtostrf(airTemp, 1, 2, tmpStr);
    Serial.print("Publishing: ");
    Serial.println(tmpStr);
    client.publish("air_temperature", tmpStr);
   
    dtostrf(waterTemp, 1, 2, tmpStr);
    Serial.print("Publishing: ");
    Serial.println(tmpStr);
    client.publish("water_temperature", tmpStr);
   
    dtostrf(airHumidity, 1, 2, tmpStr);
    Serial.print("Publishing: ");
    Serial.println(tmpStr);
    client.publish("air_humidity", tmpStr);
   
    dtostrf(airPressure, 1, 2, tmpStr);
    Serial.print("Publishing: ");
    Serial.println(tmpStr);
    client.publish("air_pressure", tmpStr);
   
    printAirValues();
    printWaterValues();
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String ssw;
  String sval;
  int val;
  int sw;

  // Rudimentary validation. Switch topics must be named as sw_1, sw_2, ....
  if ((strncmp(topic, "sw_", 3) == 0 && length == 1) ) {

    ssw = (char)topic[3];
    sw = ssw.toInt();

    // toInt() will return 0 if it fails conversion. Switches start at 1.
    if (sw == 0) {
      return;
    }

    sval = (char)message[0];
    val = sval.toInt();
    if (val > 0) {
      Serial.print("Turning on: ");
      Serial.println(sw - 1);
      digitalWrite(relay[sw - 1], LOW);
    } else {
      Serial.print("Turning off: ");
      Serial.println(sw - 1);
      digitalWrite(relay[sw - 1], HIGH);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (client.connect("NerdPool")) {
      // One switch, one topic. This way HA will retain all
      // the states if ESP32 is disconnected.
      // These also need to be configured in HA.
      client.subscribe("sw_1");
      client.subscribe("sw_2");
      client.subscribe("sw_3");
      client.subscribe("sw_4");
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
    } else {
      Serial.print("ERR= ");
      Serial.print(client.state());
      Serial.println(" -- retrying.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void flash_leds() {
  for (int i = 0; i < 5; i++) { digitalWrite(leds[i], HIGH); delay(150); }
  delay(150);
  for (int i = 4; i > 0; i--) { digitalWrite(leds[i], LOW); delay(150); }
  delay(150);
}

void printAirValues() {
    Serial.print("Temperature = ");
    Serial.print(airSensor.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(airSensor.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(airSensor.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(airSensor.readHumidity());
    Serial.println(" %");
}

void printWaterValues()
{
  waterSensor.requestTemperatures();
  float tempC = waterSensor.getTempC(waterSensorAddress);
  Serial.print("Water temperature = ");
  Serial.print(tempC);
  Serial.println(" *C");
}
