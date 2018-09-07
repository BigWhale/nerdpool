#ifndef MAIN_HPP
#define MAIN_HPP
void flash_leds();
void printAirValues();
void printWaterValues();
void printCurrentNet();
void printWifiData();
void callback(char* topic, byte* message, unsigned int length);
void reconnect();
#endif
