/*
 * Very simple class to control a fan.
 * Currently only on/off, eventually I might add a pwm output
 * Really just toggles a GPIO pin
 */

#ifndef FAN_H
#define FAN_H

#include "Arduino.h"

class Fan {
 public:
  int8_t fanPin; 
 
  Fan(int8_t pin);
  void on();
//  void on(float duty_cycle);
  void off();
};

#endif  // FAN_H
