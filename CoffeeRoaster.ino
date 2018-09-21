/*
 * Coffee Roaster Controller
 * Temperature control of a coffee roaster for automated control and ramp profiles
 * Measures temperature with a thermocouple and outputs gate drive signal to a triac to contorl temperature
 * Has a web interface for setting the temperature/settings/ramp profiles
 */

#include "internet.h"
#include "webServer.h"
#include <FS.h>
#include "controller.h"

//temperature sampling interval and PID update interval
unsigned long sampleInterval = 2000;

unsigned long prevMillis;
uint32_t actualTime;

void logTemperature(String filename, uint32_t timestamp, float setpoint, float output, float temperature);
double calc_setpoint(double temp_in, double ramp_rate_in);


void setup(void){
  Serial.begin(115200);

  setupWIFI();
  startUDP();
  setupNTP();
  SPIFFSSetup();
  webserverSetup();
  
  prevMillis = millis();
}


void loop(void){
  // Get the time
  unsigned long currentMillis = millis();
  actualTime = NTPGetTime();
  if(actualTime != 0){
    // Check temperature
    unsigned long elapsed_time = currentMillis - prevMillis;
    if(elapsed_time > sampleInterval){
      prevMillis = currentMillis;
      // measure temperature and update state
      controller.measureTemperature();
      controller.updateState();

      // Do the things
      if(controller.programMode == SIMPLE){
        // Simple temp controller
        if(controller.state == OFF){
          controller.triac.disable();
          controller.fan.off();
        }else if(controller.state == HOLD){
          controller.fan.on();
          float output = controller.computeOutput();
          controller.triac.duty_cycle = output;
          controller.triac.enable();
          logTemperature(controller.filename, actualTime, controller.setpoint, output, controller.temperature);
          webserverPushDatapoint(actualTime, controller.setpoint, output, controller.temperature);
        }else{
          //Error: unexpected state
          Serial.print("Error - unexpected state: ");Serial.println(controller.state);
          controller = Controller(TRIAC_PIN, FAN_PIN);
          controller.triac.disable();
          controller.fan.off();
        }
      }else if(controller.programMode == PROGRAM){
        // Execute a roast profile
        float setpoint;
        float output;
        switch(controller.state){
          case HOLD:
            controller.fan.on();
            setpoint = controller.hold_temp;
          case PREHEAT:
            controller.fan.on();
            setpoint = controller.preheat_temp;
          case PREHEATING:
            controller.fan.on();
            setpoint = calc_setpoint(controller.temperature, controller.ramp_rate);
          case RAMPING:
            controller.fan.on();
            setpoint = calc_setpoint(controller.temperature, controller.ramp_rate);
            controller.updateSetpoint(setpoint);
            output = controller.computeOutput();
            controller.triac.duty_cycle = output;
            logTemperature(controller.filename, actualTime, setpoint, output, controller.temperature);
            webserverPushDatapoint(actualTime, setpoint, output, controller.temperature);
            controller.triac.enable();
            break;
          case OFF:
            controller.fan.off();
            controller.updateSetpoint(0);
            controller.triac.disable();
            break;
          case COOLING:
            controller.fan.on();
            controller.updateSetpoint(0);
            controller.triac.disable();
            break;
        }

      }else{
        //error reset the controller
        Serial.print("Error - unknown programMode: "); Serial.println(controller.programMode);
        controller = Controller(TRIAC_PIN, FAN_PIN);
      }
    }
    controller.triac.process();
  }
  server.handleClient();
  webSocket.loop();
}


/* Calculate a new setpoin during a ramp. Ie. current temp + interval*ramp_rate */
double calc_setpoint(double temp_in, double ramp_rate_in){
  return(temp_in + ramp_rate_in/60.0*sampleInterval/1000.0);
}


void logTemperature(String filename, uint32_t timestamp, float setpoint, float output, float temperature){
  File tempLog;
  if(!SPIFFS.exists(filename)){
    tempLog = SPIFFS.open(filename, "w");
    tempLog.println("Time,Setpoint,Output,Temperature");
  }else{
    tempLog = SPIFFS.open("/"+filename, "a");
  }
  tempLog.print(timestamp);
  tempLog.print(',');
  tempLog.print(setpoint);
  tempLog.print(',');
  tempLog.print(output);
  tempLog.print(',');
  tempLog.println(temperature);
  tempLog.close();
}
