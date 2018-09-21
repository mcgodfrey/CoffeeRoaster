#include "fan.h"

Fan::Fan(int8_t pin){
  fanPin = pin;
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, LOW);
}

void Fan::on(){
  digitalWrite(fanPin, HIGH);
}

/*not implemented yet. Just turn on 100%*/
//void Fan::on(float duty_cycle){
//  on();
//}

void Fan::off(){
  digitalWrite(fanPin, LOW);
}

