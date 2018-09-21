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
#include "output.h"
#include "fan.h"


#define MAX_PREHEAT_TIME 120
#define SAFE_TEMP 30

#define FAN_PIN D3
#define TRIAC_PIN D2

enum ProgramMode {SIMPLE, PROGRAM};
enum State {OFF, PREHEATING, PREHEAT, RAMPING, HOLD, COOLING};

class Controller{
  public:
    ProgramMode programMode;
    State state;
    String filename;
    float preheat_temp;
    float ramp_rate;
    float hold_temp;
    float hold_time;
    float max_preheat_time;
    float temperature;
    float setpoint;
    float p;
    float i;
    float d;
    Adafruit_MAX31855 thermocouple;
    TriacOutput triac;
    Fan fan;

    Controller(uint8_t triac_pin, uint8_t fan_pin);
    void reset();
    void setMode(String mode_in);
    void updateSetpoint(float setpoint_in);

    void measureTemperature();
    void updateState();
    float computeOutput();

    void start();
    void stop();
    void restart();

    void set_p(float p);
    void set_i(float i);
    void set_d(float d);

  private:
    uint32_t _state_start_time;
};

extern Controller controller;

#endif  // CONTROLLER_H
