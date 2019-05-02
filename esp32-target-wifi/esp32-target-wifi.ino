#include <Arduino.h>
#include "esp32Wifi.h"

String aGlobalVariable;

void setup() {
  // put your setup code here, to run once:
  aGlobalVariable = "THIS IS A TEST";
  WifiSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  WifiLoop();
}
