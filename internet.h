/*
 * Sets up wifi connection and gets time from NTP server
 * 
 */
#ifndef INTERNET_H
#define INTERNET_H

#include <ESP8266WiFi.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

// Should look into this:
// https://github.com/tzapu/WiFiManager
void setupWIFI(void);
void setupNTP(void);
void startUDP(void);
uint32_t NTPGetTime(void);


#endif //INTERNET_H
