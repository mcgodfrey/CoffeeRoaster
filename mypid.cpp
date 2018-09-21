#include "mypid.h"
#include <Arduino.h>

#define CTRL_P 0
#define CTRL_PI 1
#define CTRL_PID 2

#define CONTROL_MODE 1

/*working variables*/
static unsigned long lastTime;
static double setpoint;
//Save Iterm = ki*(error*timeChange) instead of just error. then if ki changes we don't have to modify the sum term
//http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-tuning-changes/
static double Iterm;
static double lastErr, lastInput;
static double kp, ki, kd;
static double outputMin, outputMax;


//Default to calculating derivative of input. Able to switch to derivative of error by setting to false
//dInput should eliminate "derivative kick" - http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-derivative-kick/
bool derivative_of_input = true;

#if CONTROL_MODE == CTRL_P
double PIDCompute(float input){
  //Compute all the working error variables
  double error = setpoint - input;

  double output = kp*error;
  if (output > outputMax) {
    output = outputMax;
  } else if (output < outputMin) {
    output = outputMin;
  }
  return(output);
}


#elif CONTROL_MODE == CTRL_PI

double PIDCompute(float input){
  //How long since we last calculated
  unsigned long now = millis();
  unsigned long timeChange = (double)((now - lastTime)/1000.0);
  lastTime = now;

  //Compute all the working error variables
  double error = setpoint - input;
  Iterm += ki*(error*timeChange);

  double output = kp*error + Iterm;

  if (output > outputMax) {
    output = outputMax;
  } else if (output < outputMin) {
    output = outputMin;
  }

  Serial.print("Error=");Serial.print(error); 
  Serial.print(", Iterm=");Serial.print(Iterm);
  Serial.print(", output=");Serial.println(output);
  return(output);
}


#else

double PIDCompute(float input){
  //How long since we last calculated
  unsigned long now = millis();
  unsigned long timeChange = (double)((now - lastTime)/1000.0);

  //Compute all the working error variables
  double error = setpoint - input;
  Iterm += ki*(error*timeChange);
  double dInput = (input - lastInput) / timeChange;
  double dErr = (error - lastErr) / timeChange;

  //Compute PID Output
  double output;
  if(derivative_of_input){
    output = kp*error + Iterm - kd*dInput;
  }else{
    output = kp*error + Iterm + kd*dErr;
  }

  //Check if we are inside the limits
  //http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-reset-windup/ - see comment from Will
  if (output > outputMax) {
    Iterm -= output - outputMax; //back off the integral term
    output = outputMax;
  } else if (output < outputMin) {
    Iterm += outputMin - output;
    output = outputMin;
  }
  
  //Remember some variables for next time
  lastErr = error;
  lastInput = input;
  lastTime = now;

  Serial.print("kp="); Serial.print(kp);
  Serial.print(" error="); Serial.print(error); 
  Serial.print(" output="); Serial.print(output); 
  Serial.println("");

  return(output);
}

#endif


void PIDSetTunings(double Kp, double Ki, double Kd){
  kp = Kp;
  ki = Ki;
  kd = Kd;
  
  if(kp < 0){
    kp = 0;
  }
  
  if(ki < 0){
    ki = 0;
  }else if(ki == 0){
    // If we set ki to zero, then reset the integral term to zero.
    // This could cause a step change in the output though.
    Iterm = 0;
  }
  
  if(kd < 0){
    kd = 0;
  }}

void PIDSetpoint(double new_setpoint){
  setpoint = new_setpoint;
}

double PIDGetSetpoint(void){
  return(setpoint);
}

void PIDSetOutputLimits(double min_in, double max_in){
  if(min_in > max_in){
    return;
  }
  outputMin = min_in;
  outputMax = max_in;
}

void PIDreset(){
  Iterm = 0;
  lastErr = 0;
  lastInput = 0;
  lastTime = millis();
}

