#include "setup.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

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
 * Setup the wifi in AP mode
 */
void setupWIFI(){
  Serial.println("Starting WIFI");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);

  if(WiFi.softAP("CoffeeRoaster", "password")){
    Serial.println("Wifi AP set up with SSID = CoffeeRoaster, password = 'password'");
  }else{
    Serial.println("Error setting up wifi AP");
  }
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
