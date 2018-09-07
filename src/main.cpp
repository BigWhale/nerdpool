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

/* Relays */
#define RELAY_1 32
#define RELAY_2 33
#define RELAY_3 25
#define RELAY_4 26

int relay[4] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4};

/* Wireless and MQTT */
char ssid[] = "YOUR SSID";            // Change me!
char pass[] = "YOUR WPA KEY";         // Change me!
char mqttServer[] = "192.168.1.1";    // Probably change me!
int status = WL_IDLE_STATUS;

WiFiClient espClient;
PubSubClient client(mqttServer, 1883, callback, espClient);

// Timekeeping
long mqttPing = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial) {} // Wait

  // Pin setup
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_AMBER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);
  digitalWrite(RELAY_3, HIGH);
  digitalWrite(RELAY_4, HIGH);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);

  // Set default LED states
  digitalWrite(LED_BUILTIN, LOW);  // Builtin LED needs to be set to LOW

  digitalWrite(LED_RED, HIGH);
  delay(300);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_AMBER, HIGH);
  delay(300);
  digitalWrite(LED_AMBER, LOW);
  digitalWrite(LED_GREEN, HIGH);
  delay(300);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);
  delay(300);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  delay(300);
  digitalWrite(LED_YELLOW, LOW);

  // Set WIFI mode
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  // Connect to WiFi
  while ( status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);  // Make actual connection
    delay(10000);
  }
  // Turn off status LED
  digitalWrite(LED_BUILTIN, HIGH);

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
    reconnect();
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

  Serial.print("MSG: (");
  Serial.print(topic);
  Serial.print("): ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

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
    Serial.print("Attempting MQTT connection...");
    if (client.connect("NerdPool")) {
      Serial.println("connected");
      // One switch, one topic. This way HA will retain all
      // the states if ESP32 is disconnected.
      // These also need to be configured in HA.
      client.subscribe("sw_1");
      client.subscribe("sw_2");
      client.subscribe("sw_3");
      client.subscribe("sw_4");
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
  Serial.println("Cycling leds");
  int leds[5] = { LED_RED, LED_AMBER, LED_GREEN, LED_BLUE, LED_YELLOW};  

  for (int r = 5; r > 0; r-- ) {
    for (int i = 0; i < 5; i++) { digitalWrite(leds[i], HIGH); delay(100); }
    for (int i = 0; i < 5; i++) { digitalWrite(leds[i], LOW); delay(100); }
  }

  for (int r = 5; r > 0; r-- ) {
    for (int i = 0; i < 5; i++) {
      if (i == 0 ) {
        digitalWrite(leds[4], LOW);
      } else {
        digitalWrite(leds[i - 1], LOW);
      }
      digitalWrite(leds[i], HIGH);
      delay(100);
    }
    digitalWrite(leds[4], LOW);
  }
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
