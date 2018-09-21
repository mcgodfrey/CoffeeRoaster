/*
 * Triac output controller
 * Defines a TriacOutput object which is linked to a particular output pin
 * The output is a pwm signal with duty cycle given by the duty_cycle variable.
 * Create the object (with a pin number) and then set/update the duty_cycle attribute as necessary
 * Run enable/disable to enable/disable the output.
 * 
 * process() must be called periodically (ie. every main loop).
 * This object does not use interrupts so you need to call process periodically 
 * (ideally as fast as possible, at least every ~100ms or so)
 * or it will not output the correct duty cycle
 */
#ifndef OUTPUT_H
#define OUTPUT_H

#define TRIAC_PWM_PERIOD_MS 500

class TriacOutput {
 public:
  float duty_cycle; //duty cycle [0, 100]
  
  TriacOutput(int8_t triggerPin_in);

  void enable();
  void disable();
  void process();

 private:
  boolean enabled;
  int8_t triggerPin;
  unsigned long period;  // PWM period in ms
  unsigned long periodStartTime;  // Time when the current period started
};


#endif  //OUTPUT_H
