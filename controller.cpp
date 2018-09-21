#include "controller.h"
#include "mypid.h"
#include <FS.h>


Controller controller(TRIAC_PIN, FAN_PIN);

Controller::Controller(uint8_t triac_pin, uint8_t fan_pin) : thermocouple(SS), triac(triac_pin), fan(fan_pin) {

  state = OFF;
  programMode = SIMPLE;
  filename = String("temp.csv");
  preheat_temp = 0;
  ramp_rate = 0;
  hold_temp = 0;
  hold_time = 0;
  max_preheat_time = MAX_PREHEAT_TIME;
  temperature = 0;

  setpoint = 0;
  p = 10;
  i = 0;
  d = 0;

  _state_start_time = 0;

  PIDSetOutputLimits(0, 100);
  PIDSetTunings(p, i, d);
  PIDSetpoint(setpoint);

  thermocouple.begin();
}


void Controller::measureTemperature(){
  temperature = thermocouple.readCelsius();
}


void Controller::updateState(){
  if(programMode == PROGRAM){
    switch(state){
      case OFF:
        break;
      case PREHEATING:
        if(temperature > preheat_temp){
          state = PREHEAT;
          _state_start_time = millis();
        }
        break;
      case PREHEAT:
        if(millis() - _state_start_time > max_preheat_time*1000){
          // sat in preheat state for too long
          state = COOLING;
        }
        break;
      case RAMPING:
        if(temperature > hold_temp){
          state = HOLD;
          _state_start_time = millis();
        }
        break;
      case HOLD:
        if(millis() - _state_start_time > hold_time*1000){
          state = COOLING;
        }
        break;
      case COOLING:
        if(temperature < SAFE_TEMP){
          state = OFF;
        }
        break;
      default:
        state = OFF;
        break;
    }
  }
}

void Controller::setMode(String mode_in){
  if(mode_in == "SIMPLE"){
    if(programMode != SIMPLE){
      programMode = SIMPLE;
      state = OFF;
      filename = String("temp.csv");
    }
  }else if(mode_in == "PROGRAM"){
    if(programMode != PROGRAM){
      programMode = PROGRAM;
      state = OFF;
    }
  }
}

void Controller::updateSetpoint(float _setpoint){
  setpoint = _setpoint;
  PIDSetpoint(setpoint);
}


float Controller::computeOutput(){
  return(PIDCompute(temperature));
}


void Controller::start(){
  if(programMode == SIMPLE){
    if(state != HOLD){
      PIDreset();
      SPIFFS.remove("/"+filename);
      File tempLog = SPIFFS.open("/"+filename, "w");
      tempLog.println("Time,Setpoint,Output,Temperature");
      tempLog.close();
      state = HOLD;
    }
  }else if(programMode == PROGRAM){
    if(state != OFF){
      state = OFF;
    }
  }
}

void Controller::restart(){
  stop();
  start();
}

void Controller::stop(){
  if(programMode == SIMPLE){
    if(state != OFF){
      state = OFF;
    }
  }else if(programMode == PROGRAM){
    if((state != OFF) && (state != COOLING)){
      state = COOLING;
    }
  }
}

void Controller::set_p(float _p){
  p = _p;
  PIDSetTunings(p, i, d);
}

void Controller::set_i(float _i){
  i = _i;
  PIDSetTunings(p, i, d);
}


void Controller::set_d(float _d){
  d = _d;
  PIDSetTunings(p, i, d);
}

