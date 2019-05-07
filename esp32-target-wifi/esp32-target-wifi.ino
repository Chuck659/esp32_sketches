#include <Arduino.h>
#include "esp32Wifi.h"
#include "esp32Target.h"
#define CREATE
#include "esp32Globals.h"

const char *version = "esp32 - version 1 2019-05-02";

void setup() {
  Serial.begin(115200);
  Serial.println(millis());
  Serial.println();
  Serial.print("Version: ");
  Serial.println(version);

  // put your setup code here, to run once:
  targetSetup();
  wifiSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  targetLoop();
}
