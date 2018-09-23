/*
 * Coffee Roaster Controller
 * Temperature control of a coffee roaster for automated control and ramp profiles
 * Measures temperature with a thermocouple and outputs gate drive signal to a triac to contorl temperature
 * Has a web interface for setting the temperature/settings/ramp profiles
 */

#include "setup.h"
#include "controller.h"
#include "webServer.h"

//temperature sampling interval and PID update interval
unsigned long sampleInterval = 2000;

unsigned long prevMillis;
uint32_t actualTime;

void setup(void){
  Serial.begin(115200);
  Serial.println("Coffee Roaster setup!");

  SPIFFSSetup();
  controller.loadConfig();
  setupWIFI();
  setupNTP();
  webserverSetup();
  
  prevMillis = millis();
}


void loop(void){
  // Get the time
  unsigned long currentMillis = millis();
  actualTime = NTPGetTime();
  if(actualTime != 0){
    unsigned long elapsed_time = currentMillis - prevMillis;
    if(elapsed_time > sampleInterval){
      prevMillis = currentMillis;
      controller.process(actualTime);
    }
  }
  controller.triac.process();
  server.handleClient();
  webSocket.loop();
}

