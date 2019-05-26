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
  gTargetState = 0;
  gTargetDataReady = 0;
  gWifiCommand = 0;
  gWifiReady = false;
  pinMode (LED, OUTPUT);
  // put your setup code here, to run once:
  wifiSetup();

  while (!gWifiReady) {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }
  
  targetSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  targetLoop();
}
