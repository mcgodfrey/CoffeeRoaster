/*
 * Main controller class
 * This does all the work of storing the state, as well as updating the outputs
 * Contains all the input/output objects (eg. thermocouple, triac, fan) which are controlled through this interface
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "triacOutput.h"
#include "fan.h"
#include "mypid.h"


#define MAX_PREHEAT_TIME 120
#define SAFE_TEMP 30

#define FAN_PIN D3
#define TRIAC_PIN D2
#define SAMPLE_INTERVAL 2000

enum ProgramMode {SIMPLE, PROGRAM};
enum State {OFF, PREHEATING, PREHEAT, RAMPING, HOLD, COOLING};

class Controller{
  public:
    ProgramMode programMode;
    State state;
    double ramp_rate;
    unsigned long _prevMillis;
    unsigned long _sampleInterval;
    unsigned long _actualTime;
    
    Adafruit_MAX31855 thermocouple;
    TriacOutput triac;
    Fan fan;
    PID myPID;

    Controller(uint8_t triac_pin, uint8_t fan_pin, unsigned long sampleInterval);
    
    void process();
    void start();
    void stop();
    void restart();

    double getTemperature();

    void setSetpoint(double setpoint);
    double getSetpoint();

    void setP(double p);
    void setI(double i);
    void setD(double d);

    double getP();
    double getI();
    double getD();
    
    bool loadConfig();
    bool saveConfig();
};

extern Controller controller;

#endif  // CONTROLLER_H
