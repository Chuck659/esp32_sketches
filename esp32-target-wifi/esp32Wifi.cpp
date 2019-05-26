#include <Arduino.h>
#include "esp32Globals.h"

#include <WebServer.h>
#include <mDNS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

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
int lastState = STATUS_UNKNOWN;

// Create the Web Server listening on port 80 (http)
//WebServer server(80);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient* theClient = nullptr;

// Forward defines
static void sendCommandWithoutData(unsigned char cmd, String cmdName);
void getLocalStatus();
String recvSerial(unsigned char *cmd);
void handlePeerData();
String getStatus();

void wsSendStatus(AsyncWebSocketClient * client) {
  String status = "{ \"type\": \"status\", \"payload\": \"";
  status += getStatus() + "\"}";
  client->text(status);
}

void wsSendData(AsyncWebSocketClient * client, String data) {
  if (data.length() > 0) {
    String msg = "{ \"type\": \"data\", \"payload\": \"";
    msg += data + "\"}";
    client->text(msg);
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
 
  if(type == WS_EVT_CONNECT) {
    Serial.println("Websocket client connection received");
    wsSendStatus(client);
    lastState = gTargetState;
    wsSendData(client, hitData);
    theClient = client;
    hitData = "";
    
  } else if(type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
    theClient = nullptr;
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        if (strcmp((char*)data, "ping") == 0) {
          client->text("pong");
        }
      } 
    }
  }
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
void handleStatus(AsyncWebServerRequest *request)
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
  request->send(200, "application/json", resp);
}

// Start (run) route - http://<address>/start
void handleStart(AsyncWebServerRequest *request)
{
  webCount++;
  hitData = "";
  request->send(200, "application/json", "{}");
  // Send RUN command to arduino
  Serial.println("Run command");
  sendCommandWithoutData(RUNCMD, "run");
}

// Get hit data route - http://<address>/hitData
void handleGetHitData(AsyncWebServerRequest *request)
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
//  hitData = "";

  request->send(200, "application/json", json);
}

void setupHandlers() {
    server.on("/", [](AsyncWebServerRequest* request) {
      request->send(200, "text/plain", "hello from esp32!");
  });
  
  server.on("/status", handleStatus);
  server.on("/start", handleStart);
  server.on("/hitData", handleGetHitData);
  
  server.on("/function1", [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{}");
    sendCommandWithoutData(F1CMD, "Function 1");
  });
  
  server.on("/function2", [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{}");
    sendCommandWithoutData(F2CMD, "Function 2");
  });
  
  server.on("/function3", [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{}");
    sendCommandWithoutData(F3CMD, "Function 3");
  });
  
  server.on("/function4", [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{}");
    sendCommandWithoutData(F4CMD, "Function 4");
  });
  
  server.on("/function5", [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{}");
    sendCommandWithoutData(F5CMD, "Function 5");
  });
  
  server.on("/function6", [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{}");
    sendCommandWithoutData(F6CMD, "Function 6");
  });
  
  server.on("/function7", [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{}");
    sendCommandWithoutData(F7CMD, "Function 7");
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"message\": \"Not Found\"}");
  });

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}

void wifiSetupInternal() {


  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("MAC: ");
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());

  // commented out to allow WiFi to pick address
  //  WiFi.config(ip, gateway, netmask);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    yield();
    Serial.println(WiFi.status());
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

  setupHandlers();

  delay(100);

  Serial.println("");

  Serial.println("Wifi setup complete");
  gWifiReady = true;
}

// Get action to perform - if no action just monitor interfaces
void wifiLoop()
{
  while (true)
  {
    if (theClient && lastState != gTargetState) {
      wsSendStatus(theClient);
      lastState = gTargetState;
    }
    handlePeerData();
    yield();
    delay(10);
  }
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
    if (theClient) {
      wsSendData(theClient, hitData);
      hitData = "";
    }
  }
}
