#include <Arduino.h>
#include "esp32Globals.h"

#include <WebServer.h>
#include <mDNS.h>
#include <WiFi.h>

TaskHandle_t wifiTaskHandle;

#define HOME
// Enables debug print outs
#define DEBUG 1

//
// WiFi SSID / password
////
#ifdef HOME
const char *ssid = "goofmeisters";
const char *password = "mineshaftgap";
#else
const char *ssid = "TXTdev";
const char *password = "Shoot999";
#endif

// Commands between Wifi and Target

// Status counters - used to debug connection status
int webStatusCount = 0;
int webHitCount = 0;
int webCount = 0;
// Last value of hitData received.
String hitData;

// Create the Web Server listening on port 80 (http)
WebServer server(80);

// Forward defines
static void sendCommandWithoutData(unsigned char cmd, String cmdName);
void getLocalStatus();
String recvSerial(unsigned char *cmd);
void handlePeerData();

// HTTP route handlers (see setup for mapping from URL to function
//
// Root route - http://<address>/
void handleRoot()
{
  server.send(200, "text/plain", "hello from esp32!");
}

String getStatus()
{
  switch (gTargetState)
  {
  case STATUS_IDLE:
    return "ready";
    break;
  case STATUS_RUNNING:
    return "running";
    break;
  case STATUS_RUN_COMPLETE:
    return "complete";
    break;
  default:
    return "unknown";
  }
}

// Status route - http://<address>/status
void handleStatus()
{
  webStatusCount++;
  String resp = "{ \"status\": \"";
  switch (gTargetState)
  {
  case STATUS_IDLE:
    resp += "ready\"}";
    break;
  case STATUS_RUNNING:
    resp += "running\"}";
    break;
  case STATUS_RUN_COMPLETE:
    resp += "complete\"}";
    break;
  default:
    resp += "unknown\"}";
  }
  server.send(200, "application/json", resp);
}

// Start (run) route - http://<address>/start
void handleStart()
{
  webCount++;
  hitData = "";
  server.send(200, "application/json", "{}");
  // Send RUN command to arduino
  Serial.println("Run command");
  sendCommandWithoutData(RUNCMD, "run");
}

// functionX route - http://<address>/functionX
void handleFunction1()
{
  webCount++;
  server.send(200, "application/json", "{}");
  sendCommandWithoutData(F1CMD, "Function 1");
}
void handleFunction2()
{
  webCount++;
  server.send(200, "application/json", "{}");
  sendCommandWithoutData(F2CMD, "Function 2");
}
void handleFunction3()
{
  webCount++;
  server.send(200, "application/json", "{}");
  sendCommandWithoutData(F3CMD, "Function 3");
}
void handleFunction4()
{
  webCount++;
  server.send(200, "application/json", "{}");
  sendCommandWithoutData(F4CMD, "Function 4");
}
void handleFunction5()
{
  webCount++;
  server.send(200, "application/json", "{}");
  sendCommandWithoutData(F5CMD, "Function 5");
}
void handleFunction6()
{
  webCount++;
  server.send(200, "application/json", "{}");
  sendCommandWithoutData(F6CMD, "Function 6");
}
void handleFunction7()
{
  webCount++;
  server.send(200, "application/json", "{}");
  sendCommandWithoutData(F7CMD, "Function 7");
}

// Get hit data route - http://<address>/hitData
void handleGetHitData()
{
  //  Serial.println(String("getHitData: ") + hitData);
  webHitCount++;
  String json = "{\"status\": \"";
  json += getStatus();
  json += "\", \"data\":[";
  if (hitData.length() > 0)
  {
    json += "\"";
    for (int i = 0; i < hitData.length(); i++)
    {
      if (hitData[i] == '\n')
      {
        json += "\",\"";
      }
      if (hitData[i] >= ' ')
      {
        json += hitData[i];
      }
    }
    json += "\"";
  }
  json += String("]}");
  if (hitData.length() > 0)
    Serial.println(json);
  hitData = "";

  server.send(200, "application/json", json);
}

// Default route for all other routes
void handleNotFound()
{
  server.send(404, "application/json", "{\"message\": \"Not Found\"}");
}

void wifiSetupInternal() {


  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  // commented out to allow WiFi to pick address
  //  WiFi.config(ip, gateway, netmask);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    yield();
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the web (port 80) server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this IP to connect: ");
  Serial.println(WiFi.localIP());

  // Start DNS (not sure if this is needed)
  // if (MDNS.begin("esp8266"))
  // {
  //   Serial1.println("MDNS responder started");
  // }

  // Setup up the URL routing to handler functions above
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/start", handleStart);
  server.on("/hitData", handleGetHitData);
  server.on("/function1", handleFunction1);
  server.on("/function2", handleFunction2);
  server.on("/function3", handleFunction3);
  server.on("/function4", handleFunction4);
  server.on("/function5", handleFunction5);
  server.on("/function6", handleFunction6);
  server.on("/function7", handleFunction7);

  server.onNotFound(handleNotFound);

  delay(100);

  Serial.println("");

  Serial.println("Wifi setup complete");
}

// Simple menu for various commands
void getJobMenu()
{
  Serial.println("GETJOB():  enter the item number to run");
  Serial.println("ITEM    function   Description");
  Serial.println("0. display menu");
  Serial.println("1. get local status");
  Serial.println("2. run exercise");
  Serial.println("3. get hit data");
  Serial.println("4. function 1");
  Serial.println("5. function 2");
  Serial.println("7. function 3");
  Serial.println("8. function 4");
  Serial.println("9. function 5");
  Serial.println("10. function 6");
  Serial.println("11. function 7");
  Serial.println("12. reset arduino");
  Serial.println("99. EXIT");
}

// Get action to perform - if no action just monitor interfaces
void getJob()
{
  int jobNumber;
  getJobMenu();
  bool done = false;
  while (!done)
  {
    while (!Serial.available())
    {
      // Poll interfaces while waiting for user input
      server.handleClient();
      handlePeerData();
    }
    delay(50);
    while (Serial.available())
    {
      jobNumber = Serial.parseInt();
      if (Serial.read() != '\n')
      {
        Serial.println("going to " + String(jobNumber));
      }
    }
  continue;
    switch (jobNumber)
    {
    case 0:
      getJobMenu();
      gTargetState = ((gTargetState + 1) % 3) + 1;
      Serial.println(String("Running on ") + String(xPortGetCoreID()));
      break;
    case 1:
      getLocalStatus();
      break;
    case 2:
      sendCommandWithoutData(RUNCMD, "run");
      break;
    case 3:
      sendCommandWithoutData(HITDATA, "get hit data");
      break;
    case 4:
      sendCommandWithoutData(F1CMD, "function 1");
      break;
    case 5:
      sendCommandWithoutData(F2CMD, "function 2");
      break;
    case 6:
      sendCommandWithoutData(F3CMD, "function 3");
      break;
    case 7:
      sendCommandWithoutData(F4CMD, "function 4");
      break;
    case 8:
      sendCommandWithoutData(F5CMD, "function 5");
      break;
    case 9:
      sendCommandWithoutData(F6CMD, "function 6");
      break;
    case 10:
      sendCommandWithoutData(F7CMD, "function 7");
      break;
    case 11:
      // resetSlave();
      break;
    case 99:
      done = true;
      break;
    } // end of switch
  }
  Serial.println("DONE in getJob().");
}

void wifiLoop()
{
  getJob();
}

void wifiTask(void*) {
  wifiSetupInternal();
  wifiLoop();
}

void wifiSetup()
{
  xTaskCreatePinnedToCore(
      wifiTask, /* Function to implement the task */
      "Wifi Task", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &wifiTaskHandle,  /* Task handle. */
      0); /* Core where the task should run */
}

// Dump some local data for debugging
void getLocalStatus()
{
  Serial.println("===================================================");
  Serial.println(String("Peer status is ") + String(gTargetState));
  Serial.println(String("Web counts are ") + String(webCount) + String(" ") + String(webStatusCount) + " " + String(webHitCount));
  Serial.println(String("Local hit data is ") + hitData);
  Serial.println("===================================================");
}

// Send a simple command with no data (cmdName is for debug output)
static void sendCommandWithoutData(unsigned char cmd, String cmdName)
{
  if (gWifiCommand == 0) {
    gWifiCommand = cmd;
  }
}

void handlePeerData() {
  if (gTargetDataReady > 0) {
    String data = String(gDataBuffer);
    gTargetDataReady = 0;
    hitData = data;
  }
}
