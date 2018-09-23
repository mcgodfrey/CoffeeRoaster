#include <FS.h>
#include <ArduinoJson.h>
#include "controller.h"
#include "webServer.h"


#define DEFAULT_P 0.4
#define DEFAULT_I 0.03
#define DEFAULT_D 0.02

Controller controller(TRIAC_PIN, FAN_PIN);

String config_filename = String("config.json");


Controller::Controller(uint8_t triac_pin, uint8_t fan_pin) :
    thermocouple(SS),
    triac(triac_pin),
    fan(fan_pin),
    myPID(DEFAULT_P, DEFAULT_I, DEFAULT_D) {

  state = OFF;
  programMode = SIMPLE;
  ramp_rate = 0;

  // initialise my objects and put into a safe state
  triac.disable();
  fan.off();
  myPID.setOutputLimits(0, 100);
  myPID.setSetpoint(0);
  thermocouple.begin();
}


void Controller::process(uint32_t actualTime){
  if(controller.programMode == SIMPLE){
    double temperature = getTemperature();
    webserverPushData("temperature", temperature);
    if(controller.state == OFF){
      controller.triac.disable();
      controller.fan.off();
    }else if(controller.state == HOLD){
      controller.fan.on();
      double output = myPID.compute(temperature);
     webserverPushData("duty_cycle", output);
      triac.duty_cycle = output;
      triac.enable();
      webserverPushDatapoint(actualTime, myPID.getSetpoint(), output, temperature);
    }else{
      //Error: unexpected state
      String error_msg = String("Error - unexpected state: ") + String(controller.state);
      Serial.println(error_msg);
      webserverLog(error_msg);
      controller = Controller(TRIAC_PIN, FAN_PIN);
    }
  }else{
    //error reset the controller
    String error_msg = String("Error - unknown programMode: ") + String(controller.programMode);
    Serial.println(error_msg);
    webserverLog(error_msg);
    controller = Controller(TRIAC_PIN, FAN_PIN);
  }
  return;
}

double Controller::getTemperature(){
  return(thermocouple.readCelsius());
}

void Controller::setSetpoint(double setpoint){
  myPID.setSetpoint(setpoint);
}
double Controller::getSetpoint(void){
  return(myPID.getSetpoint());
}


void Controller::start(){
  if(state != HOLD){
    myPID.reset();
    state = HOLD;
  }
}

void Controller::stop(){
  if(state != OFF){
    state = OFF;
  }
}

void Controller::restart(){
  stop();
  start();
}


// Set/Get PID parameters
void Controller::setP(double p){
  myPID.setP(p);
}
void Controller::setI(double i){
  myPID.setI(i);
}
void Controller::setD(double d){
  myPID.setD(d);
}

double Controller::getP(){
  return(myPID.getP());
}
double Controller::getI(){
  return(myPID.getI());
}
double Controller::getD(){
  return(myPID.getD());
}

bool Controller::loadConfig(){
  Serial.println("Loading config");
  if(!SPIFFS.exists("/"+config_filename)){
    Serial.println("  Config file doesn't exist");
    return(true);
  }

  File configFile = SPIFFS.open("/"+config_filename, "r");
  if (!configFile) {
    Serial.println("  Failed to open config file");
    return false;
  }

  // Allocate a buffer to store contents of the file and read it in
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();

  // parse it
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  if(json.containsKey("p")){
    setP(json["p"]);
  }
  if(json.containsKey("i")){
    setI(json["i"]);
  }
  if(json.containsKey("d")){
    setD(json["d"]);
  }
  return true;
}


bool Controller::saveConfig(){
  Serial.println("Saving Config");

  Serial.println("  Creating JSON object");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["p"] = myPID.getP();
  json["i"] = myPID.getI();
  json["d"] = myPID.getD();

  if(SPIFFS.exists("/"+config_filename)){
    Serial.println("   Config file already exists. Deleting old one first");
    SPIFFS.remove("/"+config_filename);
  }

  Serial.println("   Opening the config file");
  File configFile = SPIFFS.open("/"+config_filename, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  Serial.println("   Writing the config file");
  json.printTo(Serial);
  Serial.println("");
  if(json.printTo(configFile) == 0){
    Serial.println("    Failed to write to file");
  }

  Serial.println("   Closing the config file");
  configFile.close();

  Serial.println("   Done");
  return true;
}
