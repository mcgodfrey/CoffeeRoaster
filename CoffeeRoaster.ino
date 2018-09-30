/*
 * Coffee Roaster Controller
 * Temperature control of a coffee roaster for automated control and ramp profiles
 * Measures temperature with a thermocouple and outputs gate drive signal to a triac to contorl temperature
 * Has a web interface for setting the temperature/settings/ramp profiles
 */
#include "setup.h"
#include "controller.h"
#include "webServer.h"

void setup(void){
  Serial.begin(115200);
  Serial.println("Coffee Roaster setup!");

  SPIFFSSetup();
  controller.loadConfig();
  setupWIFI();
  webserverSetup();
}


void loop(void){
  controller.process();
  //controller.triac.process();
  server.handleClient();
  webSocket.loop();
}

