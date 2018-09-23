#include "mypid.h"
#include <Arduino.h>


PID::PID(double Kp, double Ki, double Kd){
  _derivative_of_input = true;
  _mode = 0;
  _setpoint = 0;
  setTunings(Kp, Ki, Kd);
  reset();
  _outputMin = 0;
  _outputMax = 100;
}


double PID::compute(double input){
  //How long since we last calculated
  unsigned long now = millis();
  unsigned long timeChange = (double)((now - _lastTime)/1000.0);

  // Calculate the error term
  double error = _setpoint - input;

  //special case for the first datapoint
  if(_lastTime == 0){
    double output = 0;
    _lastErr = error;
    _lastInput = input;
    _lastTime = now;
    return(output);
  }

  //Compute all the other error variables
  _iTerm += _Ki*(error*timeChange);
  double dInput = (input - _lastInput) / timeChange;
  double dErr = (error - _lastErr) / timeChange;

  //Compute PID Output
  double output = 0;
  if(_derivative_of_input){
    output = _Kp*error + _iTerm - _Kd*dInput;
  }else{
    output = _Kp*error + _iTerm + _Kd*dErr;
  }

  //Check if we are inside the limits
  //http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-reset-windup/ - see comment from Will
  if (output > _outputMax) {
    _iTerm -= output - _outputMax; //back off the integral term
    output = _outputMax;
  } else if (output < _outputMin) {
    _iTerm += _outputMin - output;
    output = _outputMin;
  }
  
  //Remember some variables for next time
  _lastErr = error;
  _lastInput = input;
  _lastTime = now;

  Serial.print("error="); Serial.println(error); 
  Serial.print("kp="); Serial.println(_Kp);
  Serial.print("Iterm=");Serial.println(_iTerm);
  Serial.print("dInput=");Serial.print(dInput);
  Serial.print("dErr=");Serial.print(dErr);
  Serial.print("output="); Serial.println(output); 
  Serial.println("");

  return(output);
}


void PID::setTunings(double Kp, double Ki, double Kd){
  setP(Kp);
  setI(Ki);
  setD(Kd);
}

void PID::setP(double Kp){
  if(Kp < 0){
    _Kp = 0;
  }else{
    _Kp = Kp;
  }
}

void PID::setI(double Ki){
  if(Ki <= 0){
    // If we set ki to zero, then reset the integral term to zero.
    // This could cause a step change in the output though.
    _iTerm = 0;
    _Ki = 0;
  }else{
    _Ki = Ki;
  }
}

void PID::setD(double Kd){
  if(Kd < 0){
    _Kd = 0;
  }else{
    _Kd = Kd;
  }
}

double PID::getP(){
  return(_Kp);
}
double PID::getI(){
  return(_Ki);
}
double PID::getD(){
  return(_Kd);
}

void PID::setSetpoint(double setpoint){
  _setpoint = setpoint;
}

double PID::getSetpoint(void){
  return(_setpoint);
}

void PID::setOutputLimits(double outputMin, double outputMax){
  if(outputMin > outputMax){
    return;
  }
  _outputMax = outputMax;
  _outputMin = outputMin;
}

void PID::reset(){
  _iTerm = 0;
  _lastErr = 0;
  _lastInput = 0;
  _lastTime = 0;
}

