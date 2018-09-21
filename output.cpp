/*
 * Burst mode triac controller
 */
#include <Arduino.h>
#include "output.h"


TriacOutput::TriacOutput(int8_t triggerPin_in){
  triggerPin = triggerPin_in;
  period = TRIAC_PWM_PERIOD_MS;
  periodStartTime = 0;
  duty_cycle = 0;
  enabled = 0;
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
}

void TriacOutput::enable(){
  if(!enabled){
    enabled = 1;
    periodStartTime = millis();
    digitalWrite(triggerPin, LOW);
  }
}

void TriacOutput::disable(){
  if(enabled){
    enabled = 0;
    digitalWrite(triggerPin, LOW);
  }
}

/*
 * This has to be called periodically (much faster than the period (~1s))
 */
void TriacOutput::process(){
  if(enabled){
    //Serial.println("Output enabled in TriacOutput::process()");
    unsigned long currentTime = millis();
    if(currentTime - periodStartTime >= period){
      periodStartTime = currentTime;
    }
    if(currentTime - periodStartTime < period*(duty_cycle/100.0)){
      digitalWrite(triggerPin, HIGH);
    }else{
      digitalWrite(triggerPin, LOW);
    }
  }else{
    //Serial.println("Output disabled in TriacOutput::process()");
    digitalWrite(triggerPin, LOW);
  }
}



