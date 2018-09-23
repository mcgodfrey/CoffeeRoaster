/*
 * PID controller
 * Eventually needs to be wrapped up into a class
 * set the setpoints and tune the pid parameters and limits.
 * Then call PIDCompute to calcualte the output.
 * Doesn't need to be called at a regular interval - it keeps track of the interval between calls.
 * 
 * Save Iterm = ki*(error*timeChange) instead of just error. then if ki changes we don't have to modify the sum term
 * http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-tuning-changes/
 * 
 * Default to calculating derivative of input. Able to switch to derivative of error by setting _derivative_of_input to false
 * dInput should eliminate "derivative kick" - http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-derivative-kick/
 */

#ifndef MYPID_H
#define MYPID_H

class PID {
  
 public:

  PID(double Kp, double Ki, double Kd);
  void setTunings(double Kp, double Ki, double Kd);
  void setP(double Kp);
  void setI(double Ki);
  void setD(double Kd);
  double getP();
  double getI();
  double getD();
  void reset();
  void setSetpoint(double setpoint);
  double getSetpoint();
  double compute(double input);
  void setOutputLimits(double _outputMin, double _outputMax);

 private:
  //set variables
  double _Kp, _Ki, _Kd;
  double _outputMin, _outputMax;
  double _setpoint;
  unsigned char _mode;
  bool _derivative_of_input;

  // internal variables
  unsigned long _lastTime;
  double _iTerm;
  double _lastErr, _lastInput; 
};

#endif  // MYPID_H
