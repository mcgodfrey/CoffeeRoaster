/*
 * PID controller
 * Eventually needs to be wrapped up into a class
 * set the setpoints and tune the pid parameters and limits.
 * Then call PIDCompute to calcualte the output.
 * Doesn't need to be called at a regular interval - it keeps track of the interval between calls.
 */

#ifndef MYPID_H
#define MYPID_H

double PIDCompute(float input);
void PIDSetpoint(double new_setpoint);
double PIDGetSetpoint(void);
void PIDSetTunings(double Kp, double Ki, double Kd);
void PIDSetOutputLimits(double min_in, double max_in);
void PIDreset();

#endif  // MYPID_H
