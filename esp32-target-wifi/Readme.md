# ESP32 Target Readme

## Files

- esp32Globals.h - header file that defines the global variables and symbols
- esp32Target.cpp - C++ code for the "target" core of the esp32 - application code
- esp32Target.h - header file that defines the target setup / loop functions
- esp32-target-wifi.ino - "main" file that contains the arduino setup / loop
  - Calls the Target Setup and Loop and the Wifi Setup
- esp32Wifi.cpp - C++ code for the WiFi core of the esp32
- esp32Wifi.h - header file that defines

## Global Variables

- int gTargetState;
  - Contains the current state of the application (target) code
- unsigned char gWifiCommand;
  - Contains the command (Run, Function 1, etc) from WiFi to application
- unsigned char gTargetDataReady;
  - Flag used to tell the WiFi code that data is ready to send to Range Master
- unsigned char gTargetDataLength;
  - Length of the data in the buffer
- const char\* gDataBuffer;
  - Data buffer (pointer) for the data going from application to WiFi

## Application (target states)

The global variable gTargetState is set to the current application state. These are defined in esp32Globals.h and are as follows:

- STATUS_IDLE - set on initialization and after the target run is complete and data transferred
- STATUS_RUNNING - set when receiving the Run command
- STATUS_RUN_COMPLETE - set when run is complete and data transfer is ready
