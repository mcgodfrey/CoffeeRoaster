#include "setup.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

// NTP server requirements
WiFiUDP UDP;
IPAddress timeServerIP;          // time.nist.gov NTP server address
const char* NTPServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
unsigned long lastNTPResponse = 0;
uint32_t NTPtime = 0;


/* helper functions */
static uint32_t getTime(void);
static void sendNTPpacket(IPAddress& address); 
static void startUDP(void);
static String formatBytes(size_t bytes);


/*
 * Initialise the filesystem.
 * Must be called once at the beginning.
 * Prints FS contents to serial
 */
void SPIFFSSetup(){
  SPIFFS.begin();
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {    
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  Serial.printf("\n");
}


/* 
 * Connect to the wifi.
 * Uses wifiManager library which sets up in AP mode to allow you to connect and configure wifi
 * From then on it remembers connection settings and automatically connects to known networks.
 * 
 * Also set up MDNS server (which I haven't been able to get working from my windows machine yet. Or android.)
 */
void setupWIFI(){
  Serial.println("Starting WIFI");

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect("CoffeeRoaster");

  Serial.println("");
  Serial.print("Connected to "); Serial.println(WiFi.SSID());
  Serial.print("IP address: ");  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }else{
    Serial.println("MDNS responder failed to start");
  }
}


/*
 * Setup for NTP to get the current time
 * Needs a UDP server, and then sends a packet to request the time.
 * Main code then needs to call NTPGetTime() to get the time.
 */
void setupNTP(){
  startUDP();

  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);
  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);
  delay(500);
}


/*
 * Start a UDP server (needed for NTP)
 */
void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

/*
 * Get NTP time once at the start and then just rely on millis from there.
 * We could get ntp time every hour or something if we really want to.
 */
uint32_t NTPGetTime(){
  uint32_t actualTime;
  unsigned long currentMillis = millis();

  // If we haven't got a response from the NTP server yet...
  if(NTPtime == 0){
    uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
    if (time) {                                  // If a new timestamp has been received
      NTPtime = time;
      Serial.print("NTP response:\t");
      Serial.println(NTPtime);
      lastNTPResponse = currentMillis;
    } else {                                    // If we didn't receive an NTP response yet, send another request
      sendNTPpacket(timeServerIP);
      delay(500);
    }
  }

  if(NTPtime > 0){
    actualTime = NTPtime + (currentMillis - lastNTPResponse)/1000;
  }else{
    actualTime = 0;
  }
  return(actualTime);
}

uint32_t getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}


String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}
