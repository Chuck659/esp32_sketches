
#ifdef ESP8266
#include <ESP8266WebServer.h> // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer
#include <ESP8266mDNS.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS
#include <ESP8266WiFi.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#else
#include <WebServer.h>
#include <WiFi.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#endif

const char* version="esp31_v1";

#define HOME
// Enables debug print outs
#define DEBUG 1

// Commented out to allow WiFi to select address
//IPAddress ip(192, 168, 0, 101);
//IPAddress gateway(192, 168, 0, 1);
//IPAddress netmask(255, 255, 255, 0);

//
// WiFi SSID / password
////
#ifdef HOME
const char *ssid = "ATTjAWscI2";
const char *password = "csx#v=e%uq3t";
#else
const char *ssid = "TXTdev";
const char *password = "Shoot999";
#endif

#define NULLCMD ' '
#define RUNCMD 'R'
#define HITDATA 'D'
#define HITEVENT 'H'
#define F1CMD '1'
#define F2CMD '2'
#define F3CMD '3'
#define F4CMD '4'
#define F5CMD '5'
#define F6CMD '6'
#define F7CMD '7'

unsigned char cmd = NULLCMD;
unsigned char event = NULLCMD;
unsigned char buffer[80] = {0};
unsigned char targetState = 0;

// State of the arduino
// 1 - ready, 2 - running, 3 - run complete
#define UNKNOWN_STATE 0
#define READY_STATE 1
#define RUNNING_STATE 2
#define RUN_COMPLETE_STATE 3

// Status counters - used to debug connection status
int webStatusCount = 0;
int webHitCount = 0;
int webCount = 0;

// Last value of target received.
String targetData;

// Create the Web Server listening on port 80 (http)
WebServer server(80);

//
// HTTP route handlers (see setup for mapping from URL to function
//
// Root route - http://<address>/
void handleRoot() {
  server.send(200, "text/plain", String("ESP32: ") + String(version));
}

String getStatus() {
  switch (targetState) {
    case READY_STATE:
      return "ready";
      break; 
    case RUNNING_STATE:
      return "running";
      break; 
    case RUN_COMPLETE_STATE:
      return "complete";
      break; 
    default: 
      return "unknown";
  }
}

// Status route - http://<address>/status
void handleStatus() {
  webStatusCount++;
  String resp = "{ \"status\": \"";
  switch (targetState) {
    case READY_STATE:
      resp += "ready\"}";
      break; 
    case RUNNING_STATE:
      resp += "running\"}";
      break; 
    case RUN_COMPLETE_STATE:
      resp += "complete\"}";
      break; 
    default: 
      resp += "unknown\"}";
  }
  server.send(200, "application/json", resp);
}

// Start (run) route - http://<address>/start
void handleStart() {
  webCount++;
  targetData = "";
  server.send(200, "application/json", "{}");
  // Send RUN command to arduino
  Serial.println("Run command");
  if (cmd == NULLCMD) cmd = RUNCMD;
}

// functionX route - http://<address>/functionX
void handleFunction() {
  webCount++;
  Serial.println(String("Function ") + server.uri());
  Serial.println(String("args ") + server.arg("f"));
  server.send(200, "application/json", "{}");
  if (server.args() > 0) {
    String fn = server.arg("f");
    if (fn.length() > 0 && fn[0] >= '1' && fn[0] <= '9') {
        if (cmd == NULLCMD) cmd = fn[0];
    }
  }
}

// Get hit data route - http://<address>/targetData
void handleGetTargetData() {
//  Serial.println(String("getHitData: ") + targetData);
  webHitCount++;
  String json = "{\"status\": \"";
  json += getStatus();
  json += "\", \"data\":[";
  if (targetData.length() > 0) {
    json += "\"";
    for (int i = 0; i < targetData.length(); i++) {
      if (targetData[i] == '\n') {
        json += "\",\"";
      }
      if (targetData[i] >= ' ') {
        json += targetData[i];     
      }
    }
    json += "\"";
  }
  json += String("]}");
  if (targetData.length() > 0) Serial.println(json);
  targetData = "";
  
  server.send(200, "application/json", json);
 
}

// Default route for all other routes
void handleNotFound() {
   server.send(404, "application/json", "{\"message\": \"Not Found\"}");
}

void setup()
{
  targetSetup();
  wifiSetup();
}

void wifiSetup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Version: ");
  Serial.println(version);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi .mode(WIFI_STA);
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

  // Setup up the URL routing to handler functions above
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/start", handleStart);
  server.on("/hitData", handleGetTargetData);
  server.on("/function", handleFunction);
  
  server.onNotFound(handleNotFound);

  Serial.println("");

  Serial.println("Setup complete");

  xTaskCreatePinnedToCore(
                    getWifiJob,   /* Function to implement the task */
                    "wifiTask", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    0);  
}

// Simple menu for various commands
void getWifiJobMenu() {
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
void getWifiJob(void*){
  Serial.println(String("Running on core ") + String(xPortGetCoreID()));
  int jobNumber;
  getWifiJobMenu();
  bool done = false;
  while (!done) {
    while (!Serial.available()){
      // Poll interfaces while waiting for user input
      server.handleClient();
    }
    vTaskDelay(10);
    while(Serial.available())
    {
      jobNumber = Serial.parseInt();
      if (Serial.read() != '\n') { Serial.println("going to "+String(jobNumber)); }
    } 
  
    switch (jobNumber) {
      case 0:
        getJobMenu();
        break;
      case 1:
        getWifiLocalStatus();
        break;
      case 2:
        if (cmd == NULLCMD) cmd = RUNCMD;
      break;
      case 3:
        
      break;
      case 4:
        if (cmd == NULLCMD) cmd = F1CMD;
        break;
      case 5:
        if (cmd == NULLCMD) cmd = F2CMD;
        break;
      case 6:
        if (cmd == NULLCMD) cmd = F3CMD;
        break;
      case 7:
        if (cmd == NULLCMD) cmd = F4CMD;
        break;
      case 8:
        if (cmd == NULLCMD) cmd = F5CMD;
        break;
      case 9:
        if (cmd == NULLCMD) cmd = F6CMD;
        break;
      case 10:
        if (cmd == NULLCMD) cmd = F7CMD;
        break;
      case 99:
        done = true;
        break;
    } // end of switch
  }
  Serial.println("DONE in getJob().");
}
/*
void loop()
{
  delay(1000);
  if (cmd != NULLCMD) {
    Serial.println(String("command received ") + String(cmd));
    cmd = NULLCMD;
  }
}
*/
// Dump some local data for debugging
void getWifiLocalStatus() {
  Serial.println("===================================================");
  Serial.println(String("Peer status is ") + String(targetState));
  Serial.println(String("Web counts are ") + String(webCount) + String(" ") + String(webStatusCount) + " " + String(webHitCount));
  Serial.println(String("Local hit data is ") + targetData);
  Serial.println("===================================================");
}

// Function to print a debug message + int value
void debugMsgInt(const char *msg, int value, bool filter)
{
  if (DEBUG && filter)
  {
    Serial.print(msg);
    Serial.println(value);
  }
}

// Function to print a debug message + string value
void debugMsgStr(const char *msg, String data, bool filter)
{
  if (DEBUG && filter)
  {
    Serial.println(msg + data);
  }
}


// Code for SPI interface to 8266
#include <SPI.h>

// Controls some debug output
#define DEBUG 1
// Set to 1 to indeicate no H/W is available
#define NOHW 1

//
// =========== SPI data =================  
// SPI send/recv states
#define SPI_STATE_RCVCMD 1
#define SPI_STATE_RCVLEN 2
#define SPI_STATE_RCVDATA 3
#define SPI_STATE_RCVCOMP 4
#define SPI_STATE_SENDCMD 5
#define SPI_STATE_SENDLEN 6
#define SPI_STATE_SENDDATA 7
#define SPI_STATE_WAIT 8

// SPI Variables
volatile char spi_state;
char spi_sendBuffer[256];
unsigned char spi_rcvBuffer[150];
volatile int spi_rcvIndex;
volatile int spi_length;
volatile int spi_lengthNdx;
volatile unsigned char spi_rcvCommand;

volatile unsigned char spi_sendCommand;
volatile unsigned char spi_lastSentCommand;
volatile char *spi_sendMsg;
volatile int spi_sendLength;
// These are set by application code to send a message
const char* api_sendMsg;
volatile int api_sendLength;
volatile unsigned char api_sendCommand;
// Status variable to debug whether we are getting polled
volatile int spi_pollCount = 0;

// Variable to prevent simulataneous sends
bool sendOnSpi = false;

#define RESETCMD 1
#define PINGCMD 2
#define POLLCMD 3
#define MSG 7
#define RUNCMD 20
#define HITDATA 21
#define F1CMD 22
#define F2CMD 23
#define F3CMD 24
#define F4CMD 25
#define F5CMD 26
#define F6CMD 27
#define F7CMD 28
#define ACK 0x7F
//
// ===========  End of SPI data =================
//

// Local Run Status
volatile char status;
#define STATUS_IDLE 1
#define STATUS_RUNNING 2
#define STATUS_RUN_COMPLETE 3

// char pgmname[] = "unoSpiSlaveNoAppCode";
char pgmname1[] = "Target-phone-run-ver0";
int loopCount = 0;
unsigned long runTimer = millis();
String hitData;

//
// ========== target control variables
//
// based on exerciseall-ver1
// 3.24.2018 1645hrs
// first use of device ver3 using: 220rpm, 3hall effect, 1 3-axis accel
// pins have been changed from ver1
// 5.18.18 2330hrs using motogolimit w/o time, just Upper & Mid Hall sensors also has 1st use of motohalt()
// 5.22/18 1900hrs making function to go into sketch "Slave-control-May22"
// will have motogotime() and motogolimit() at first, will then add some HIT control
// 5.28.18 1700hrs - no serial input for Mega.  Use Stephens GetJob(), each target deployment is go up, stay up for 3 seconds
// while displaying Green light and recording HITS time & max zvibes in array, once down send 3 records to SPI = up, hit, down.
// 6/14/18 - LIMIT SWITCHES NOT HALL SENSORS
// changed enough for it to compile, didn't try to run.  The 8266 & Tablet interface seemed fine.
// changing work effort to limit switch version of Exercise All.


const int greenRelay = 24;
const int redRelay = 28;
const int yellowRelay = 26; 
const int Lenable3and4 = 6;
const int LIN3 = 7;
const int LIN4 = 5;
// const int lswupper = 47;
// const int lswmid = 45;
// const int lswlower = 43;
const int lswupper = 36;
const int lswmid = 38;
const int lswlower = 37;
const int analogPinz = 7;
// int lswU, lswM, lswL;  once was "hallU" etc
int lswU, lswM, lswL;
int zval;
int zwork, zrest, zposthreshold, znegthreshold, oldhighzval, tm;
int maxuptime = 550;
int maxdowntime = 475;
int targetholdtime = 3000;
int hitdiff = 50;
char pgmname2[] = "LSW-Target-phone-run-ver022";
String zrecord, uprecord, hitrecord, dnrecord, hit5record, workrecord;
int i, j, loopcount, hitcount, hit, hittest, dir, lowerlimit, upperlimit, movecycles, moveerror;
unsigned long startmillis, runtime, movetime, startholdtime;
unsigned long hittime, prevhittime, hitinterval, startuptime, startdowntime; 
unsigned long uploopcount, downloopcount; 
unsigned long zhittime, zpintime;
int hit5grab[5];
unsigned long hit5time[5];
int hitvibelow, hitvibehigh;
unsigned long hit5endtime;
        

//
// ==========  end target control variables
//


void targetSetup() {
  Serial.begin(9600);
  Serial.println(pgmname1);
  Serial.println(pgmname2);
  Serial.println("SPI setup complete");
  status = STATUS_IDLE;
  // end of Slave setup
//
// ======  target movement setup
//
  setpins();
  loopcount = 0;
  runtime = 0;
  upperlimit = 1;
  lowerlimit = 1;
  digitalWrite(greenRelay,HIGH);
  delay(1000);
  digitalWrite(greenRelay,LOW);
  digitalWrite(yellowRelay,HIGH);
  delay(1000);
  digitalWrite(yellowRelay,LOW);
  digitalWrite(redRelay,HIGH);
  delay(1000);
  digitalWrite(redRelay,LOW);
  Serial.println("DONE w/ setup()");
  zrestvibes();
  // slideadjust();
//
//  ====  end target movement setup
//

  }  // end setup

void loop() {
  loopCount++;
  // getjob();
  if(loopCount == 1) Serial.println("In loop().");
  monitorSpi();
}  // end of loop()

// =================== Application code ================
//

void getJobMenu() {
  Serial.println("GETJOB():  enter the item number to run");
  Serial.println("ITEM    function   Description");
  Serial.println("0, refresh menu");
  Serial.println("1, get local status");
  Serial.println("2, start exercise");
  Serial.println("3, end of exercise");
  Serial.println("99. start getjob() again (EXIT)");
}

void getjob(){
  int jobNumber;
  bool done = false;
  getJobMenu();

  while (!done) {
    while (!Serial.available()){ 
      monitorSpi();
      checkRunStatus();
    }
    delay(50);
    
    while(Serial.available())
    {
      jobNumber = Serial.parseInt();
      if(Serial.read() != '\n'){Serial.println("going to " + String(jobNumber));}
    } 
    switch (jobNumber) {
    case 0:
      getJobMenu();
    break;
    case 1:
      getTargetLocalStatus();
    break;
    case 2:
      targetmove();
      break;
    case 3:
      if (status == STATUS_RUNNING) status = STATUS_RUN_COMPLETE;
      break;
    case 99:
      done = true;
    break;
    } // end of switch
  }
  Serial.println("DONE in getjob().");
} // end getjob()

void getTargetLocalStatus() {
  Serial.println("========== Local Status ===========");
  if (sendOnSpi) Serial.println("Send on SPI is true?");
  Serial.println(String("Status is ") + String((int)status));
  Serial.println(String("Poll Count is ") + String(spi_pollCount));
  Serial.println(String("Hit data is ") + hitData);
  Serial.println("========== Local Status ===========");
}

void checkRunStatus() {
  unsigned long now = millis();
  if ((status == STATUS_RUNNING) && ((now - runTimer) > 5000)) {
    status = STATUS_RUN_COMPLETE;
    hitData = "Random hit data from Arduino  ";
    hitData += String(millis());
    sendToSpiPeer(HITDATA, hitData.c_str(), hitData.length());
    Serial.println("Run Complete");
  }
}
// =========== end of Slave application code  ===========================================
//

//
// SPI interface code
//
//
// Application interface routines
//
//
//
//
// check for state of RCVCOMP - meaning command has been received for application
// 
void monitorSpi() {
  // State RCVCOMP means a command was received - process it
  // Serial.println("In monitorSpi().");
  if (spi_state == SPI_STATE_RCVCOMP)
  {
    unsigned char locCommand = spi_rcvCommand;
        
    spi_rcvBuffer[spi_rcvIndex] = 0;
    
//    debugMsgInt("Command: ", locCommand);
//    debugMsgInt("Length: ", spi_length);

    if (spi_length > 0)
    {
        debugMsgStr("Message data: ", (char*)spi_rcvBuffer);
    }

    // Simply reply with POLL message ? 
    spi_sendLength = 0;
    spi_sendCommand = POLLCMD;  
    spi_state = SPI_STATE_SENDCMD;

    switch (locCommand) {
      case RUNCMD:
      locCommand = 'x';
        Serial.println("Run command received");
        runTimer = millis();
        targetmove();
      break;
      case HITDATA:
      locCommand = 'x';
        Serial.println("Get hit data command received");
        sendToSpiPeer(HITDATA, hitData.c_str(), hitData.length());
      break;
      case F1CMD:
      locCommand = 'x';
        motogolowlimit();
        Serial.println("DONE F1 = motogolowlimit()");
      break;
      case F2CMD:
      locCommand = 'x';
      motogouplimit();
      Serial.println("DONE F2 = motogouplimit()");
      break;
      case F3CMD:
      locCommand = 'x';
        Serial.println("Function 3 command received");
      break;
      case F4CMD:
      locCommand = 'x';
        Serial.println("Function 4 command received");
      break;
      case F5CMD:
      locCommand = 'x';
        Serial.println("Function 5 command received");
      break;
      case F6CMD:
      locCommand = 'x';
        Serial.println("Function 6 command received");
      break;
      case F7CMD:
      locCommand = 'x';
        Serial.println("Function 7 command received");
      break;     
      case MSG:
      locCommand = 'x';
      break;
      case RESETCMD:
      locCommand = 'x';
      status = STATUS_IDLE;
      break;
      default:
      locCommand = 'x';
        Serial.println(String("Unknown command: ") + String(locCommand));
    }
  }
  delay(1);

}  // end monitorSpi()

//
// Send message to peer
//
void sendToSpiPeer(unsigned char cmd, const char* buffer, int len) {
  if (!sendOnSpi) {
    Serial.println(String("Send to peer: ") + buffer);
    api_sendMsg = buffer;
    api_sendLength = len;
    api_sendCommand = cmd;
    sendOnSpi = true;
  }
}

// Receive variable integer length
bool receiveLength(unsigned char c, volatile int *length, volatile int *lengthNdx)
{
  int ndx = (*lengthNdx)++;
  int l = *length;
  *length = ((c & 0x7f) << (7 * ndx)) + l;
  return c < 128;
}

// Reset state machine and receive variables
void resetSpiState()
{
  spi_state = SPI_STATE_RCVCMD;
  spi_rcvIndex = 0;
  spi_length = 0;
  spi_lengthNdx = 0;
}

void debugMsgInt(const char *msg, int value)
{
  if (DEBUG)
  {
    Serial.print(msg);
    Serial.println(value);
  }
}

void debugMsgStr(const char *msg, String value)
{
  if (DEBUG)
  {
    Serial.print(msg);
    Serial.println(value);
  }
}
//
//  ======  target move code below  ===================
//
void setpins(){
  pinMode(greenRelay, OUTPUT);
  pinMode(redRelay, OUTPUT);
  pinMode(yellowRelay, OUTPUT);
  digitalWrite(greenRelay,LOW);
  digitalWrite(yellowRelay,LOW);
  digitalWrite(redRelay,LOW);
  pinMode(Lenable3and4, OUTPUT);
  pinMode(LIN3, OUTPUT);
  pinMode(LIN4, OUTPUT);
  pinMode(lswlower, INPUT_PULLUP);
  pinMode(lswmid, INPUT_PULLUP);
  pinMode(lswupper, INPUT_PULLUP);
  digitalWrite(Lenable3and4, LOW);
  } // end setpins()

// below is for F4  06102018 2038hrs
/* void godown100(){
  Serial.println("IN godown100.");
  status = STATUS_RUNNING;
  workrecord = "F4godown100,";
  dir = 0;
  upperlimit = digitalRead(lswupper);
  middlelimit = digitalRead(lswmid);
  lowerlimit = digitalRead(lswlower);
  workrecord+="Hu,Hm,Hl,";
  workrecord+=String(upperlimit);
  workrecord+=',';
  workrecord+=String(middlelimit);
  workrecord+=',';
  workrecord+=String(lowerlimit);
  workrecord+=',';
  digitalWrite(LIN3, HIGH); // make it go down
  digitalWrite(LIN4, LOW);
  startdowntime = millis();
  digitalWrite(Lenable3and4,HIGH);  // motor starts
  startdowntime = millis();
  while((millis()-startdowntime) < 100){
   if(upperlimit != digitalRead(lswupper)) workrecord+="U,";
   if(middlelimit != digitalRead(lswmid)) workrecord+="M,";
   if(lowerlimit != digitalRead(lswlower)) workrecord+="L,"; 
  }// end while()
  digitalWrite(Lenable3and4,LOW);  // motor stops
  motohalt();
  workrecord+=String(startdowntime);
  workrecord+=',';
  
  // below ismsg to rangeMaster - keep at end of ()
  sendToSpiPeer(HITDATA, workrecord.c_str(), workrecord.length() <= 80 ? workrecord.length() : 80);  // USE THIS
} // end godown100  */


void targetmove(){
tmerror1:
movecycles = 1;
  lowerlimit = digitalRead(lswlower);
  upperlimit = digitalRead(lswupper);
  for(tm = 0; tm < movecycles; tm++){
  motogolimit(); // goes up, then down
    }
  digitalWrite(yellowRelay, HIGH);
  Serial.println("DONE with targettmove().");
  status = STATUS_IDLE;
  delay(1000);
  digitalWrite(yellowRelay, LOW);
} // end of targetmove()

// THIS motogolimit May18th bare up & down between middle & upper Hall sensors
// must start below upper Hall
void motogolimit(){
  Serial.println("IN motogolimit.");
  status = STATUS_RUNNING;
  // Serial.println(zrecord);
  // sendToSpiPeer(HITDATA, uprecord.c_str(), zrecord.length() <= 80 ? zrecord.length() : 80);  // USE THIS
  digitalWrite(Lenable3and4,LOW);
      moveerror = 0;
      dir = 1;
      digitalWrite(LIN3, LOW); //make it go up
      digitalWrite(LIN4, HIGH);
      digitalWrite(Lenable3and4,HIGH);  //motor starts here
      startuptime = millis();
      movetime = startuptime;
      uprecord = "Zu";
      uprecord += String(startuptime);
      uprecord+='/';
      // uprecord+="zrest=";
      uprecord+=String(zrest);
      uprecord+='.';
      uploopcount = 0;
      while(digitalRead(lswupper) != 0){
       zval = 0;
       hittest = 0;
       uploopcount++;
       zval = analogRead(analogPinz);
       // if(zval > zposthreshold || zval < znegthreshold) {hit = 1; hittime = millis();}
       if(zval > (zposthreshold + 50) || zval < (znegthreshold - 50)) {hit = 2; hittime = millis();}
       if(hit != 0){
        if((hittime - prevhittime) < 50) hit = 0;
        }
       // if(hit == 1){uprecord += "hit,";}
       if(hit == 2){uprecord += "H";}
       if (hit != 0) {hit = 0; prevhittime = hittime; uprecord += String(zval); uprecord+='/'; uprecord+=String(hittime-startuptime); }
       } // end of loop moving up
       digitalWrite(Lenable3and4, LOW); // motor stops here
       motohalt(); // halt it now!
       uprecord+='C';
       uprecord+=String(uploopcount);
       uprecord+='/';
       uprecord+=String(millis()-startuptime);
       uprecord+='.';
       Serial.println("DONE loop of moving up.");
       Serial.println("size of uprecord = "+String(uprecord.length()));
       Serial.println(uprecord);
       // holding up and recording hits
       startholdtime = millis();
       digitalWrite(greenRelay, HIGH);
       hitrecord = "H";
       hitrecord+= String(startholdtime-startuptime);
       hitrecord+=',';
       while( (millis() - startholdtime) < targetholdtime){
       zval = analogRead(analogPinz);
       hit5grab[0] = zval;
       hit5time[0] = millis();
       // if(zval > zposthreshold || zval < znegthreshold) {hit = 1; hittime = millis();}
       if(zval > (zposthreshold + 50) || zval < (znegthreshold - 50)) {
        hit = 2;
        hittime = millis();
        for (i = 1; i < 5; i++){
          hit5grab[i] = analogRead(analogPinz);
          hit5time[i] = millis();
          delayMicroseconds(100);
        }
        hitvibelow = zrest;
        hitvibehigh = zrest;
        for(i=0; i<5; i++){
          if (hit5grab[i] < hitvibelow) hitvibelow = hit5grab[i];
          if (hit5grab[i] > hitvibehigh) hitvibehigh = hit5grab[i];
          }
        hit5endtime = millis();
        }
       if(hit != 0){
        if((hittime - prevhittime) < 50) hit = 0;
        }
       // if(hit == 1){hitrecord += "hit,";}
       if(hit == 2){hitrecord += "HIT=";}
       if (hit != 0) {
        // for study
        hit5record = "H5,";
        hit5record+=String(hittime);
        hit5record+=','; 
        for(i=0; i < 5; i++){
          hit5record+=String(hit5grab[i]);
          hit5record+=',';
        }
        hit5record+=String(hit5endtime);
        Serial.println(hit5record);

        // end for study
        hit = 0;
        prevhittime = hittime;
        hitrecord+=String(hittime);
        hitrecord+=',';
        hitrecord += String(hitvibelow);
        hitrecord+=',';
        hitrecord+=String(hitvibehigh);
        hitrecord+=',';
        hitrecord+=String(hit5endtime);
        hitrecord+='!';}
       }// end tagetholdtime
       hitrecord+="EndD";
       hitrecord+=',';
       hitrecord+=String(millis());
       hitrecord+='.';
       digitalWrite(greenRelay, LOW);
       Serial.println("DONE holding up.");
       Serial.println("size of hitrecord = "+String(hitrecord.length()));
       Serial.println(hitrecord);
       //sendToSpiPeer(HITDATA, hitrecord.c_str(), zrecord.length() <= 80 ? zrecord.length() : 80);  // USE THIS
       // end holding up and recording hits
      // below is moving down
      dir = 0;
      digitalWrite(LIN3, HIGH); // make it go down
      digitalWrite(LIN4, LOW);
      digitalWrite(Lenable3and4,HIGH);  // motor starts
      startdowntime = millis();
      dnrecord = "Zdn";
      dnrecord += String(startdowntime);
      dnrecord+=',';
      downloopcount = 0;
      while(digitalRead(lswlower) != 0){
       downloopcount++;
       zval = analogRead(analogPinz);
       if(zval > (zposthreshold + 50) || zval < (znegthreshold - 50)) {hit = 2; hittime = millis();}
       if(hit != 0){
        if((hittime - prevhittime) < 50) hit = 0;
        }
       if(hit == 2){dnrecord += "HIT=";}
       if (hit != 0) {hit = 0; prevhittime = hittime; dnrecord += String(zval); dnrecord+=','; dnrecord+=String(hittime); dnrecord+=',';}
       } // end of loop moving down
       digitalWrite(Lenable3and4, LOW); // motor stops here
       motohalt(); // halt it now!
       digitalWrite(Lenable3and4, LOW); // motor stops here
       dnrecord+=String(downloopcount);
       dnrecord+=',';
       dnrecord+=String(millis());
       Serial.println("DONE loop of moving down.");
       Serial.println("Size of dnrecord = "+String(dnrecord.length()));
       Serial.println(dnrecord);
       //  sendToSpiPeer(HITDATA, dnrecord.c_str(), zrecord.length() <= 80 ? zrecord.length() : 80);  // USE THIS
       // end of reporting for moving down
status = STATUS_RUN_COMPLETE;      
Serial.println("DONE in motogolimit().");
sendToSpiPeer(HITDATA, uprecord.c_str(), uprecord.length() <= 80 ? uprecord.length() : 80);  // USE THIS
delay(1000);
sendToSpiPeer(HITDATA, hitrecord.c_str(), hitrecord.length() <= 80 ? hitrecord.length() : 80);  // USE THIS
delay(1000);
sendToSpiPeer(HITDATA, dnrecord.c_str(), dnrecord.length() <= 80 ? dnrecord.length() : 80);  // USE THIS
delay(1000);  
} // end motogolimit()


void motohalt(){
 digitalWrite(Lenable3and4, LOW);
  if(dir == 0){
    digitalWrite(LIN3, LOW); // make it go up
    digitalWrite(LIN4, HIGH);
    digitalWrite(Lenable3and4, HIGH);
    delay(20);
    digitalWrite(Lenable3and4, LOW);
    }
  if(dir == 1){
    digitalWrite(LIN3, HIGH); //make it go down
    digitalWrite(LIN4, LOW);
    digitalWrite(Lenable3and4, HIGH);
    delay(10);
    digitalWrite(Lenable3and4, LOW);
   }
  Serial.println("DONE motohalt()."); 
}// end motohalt()

void motogolowlimit(){
  startdowntime = millis();
  dir = 0;
    digitalWrite(Lenable3and4,LOW);
    Serial.println("IN motogolowlimit().");
    digitalWrite(LIN3, HIGH); //make it go down
    digitalWrite(LIN4, LOW);
    digitalWrite(greenRelay, HIGH);
    digitalWrite(Lenable3and4, HIGH); //start motor
    while(lowerlimit != 0) {
      lowerlimit = digitalRead(lswlower);
      }  
    digitalWrite(Lenable3and4, LOW); //stop motor
    motohalt();
    digitalWrite(greenRelay, LOW);
    Serial.println("DONE in motogolowlimit.");
}  // end motogolowlimit

void motogouplimit(){
  startuptime = millis();
  dir = 1;
    digitalWrite(Lenable3and4,LOW);
    Serial.println("IN motogouplimit().");
    digitalWrite(LIN3, LOW); //make it go up
    digitalWrite(LIN4, HIGH);
    digitalWrite(greenRelay, HIGH);
    digitalWrite(Lenable3and4, HIGH); //start motor
    upperlimit = 1;
    while(upperlimit != 0) {
      upperlimit = digitalRead(lswupper);
      }  
    digitalWrite(Lenable3and4, LOW); //stop motor
    motohalt();
    digitalWrite(greenRelay, LOW);
    Serial.println("DONE in motogouplimit");
}  // end motogouplimit

void zrestvibes(){
  digitalWrite(yellowRelay,HIGH);
  for(i = 0; i < 10; i++){
  zval = analogRead(analogPinz);
  zwork = zwork + zval;
  delay(100);
  }
  digitalWrite(yellowRelay,LOW);
  zrest = zwork/10;
  zposthreshold = zrest + hitdiff;
  znegthreshold = zrest - hitdiff;
  zval = 0; zwork = 0;
  Serial.println("DONE zrestvibes(), zrest, zposthreshold, znegthreshold = " + String(zrest) + " , " + String(zposthreshold)+" , "+String(znegthreshold));
  }// end zrestvibes()

//
//   ============  end of target move code as of May 22  ==========
//
/* Serial.println("IN targettmove() ENTER: direction to start(MUST be 1 or 9), number of move cycles:");
   while (!Serial.available()){ monitorSpi();}
   delay(50);
   while(Serial.available())
   {
  // dir = Serial.parseInt();
    movecycles = Serial.parseInt();
     if(Serial.read() != '\n'){Serial.println("Entered dir, cycles = "+String(dir)+" , "+String(movecycles)+".");}
    // Serial.println("Entered dir, cycles = "+String(dir)+" , "+String(movecycles)+".");
  }
  if(dir == 9) {Serial.println("LEAVING targetmove() w/ dir = 9."); status = STATUS_IDLE; return;}
  if(dir != 1){Serial.println("ERROR! slide MUST be at bottom to start this function!"); goto tmerror1;}
  */
  /*
 void motogotime(){
    digitalWrite(Lenable3and4,LOW);
    Serial.println("IN motogotime.  dir, runtime = "+String(dir)+","+String(runtime));
    if(dir == 0){
      digitalWrite(LIN3, HIGH); //make it go down
      digitalWrite(LIN4, LOW);
    }
    if(dir == 1){
      digitalWrite(LIN3, LOW); // make it go up
      digitalWrite(LIN4, HIGH);
    }
    digitalWrite(greenRelay, HIGH);
    digitalWrite(Lenable3and4, HIGH); //start motor
    delay(runtime);  
    digitalWrite(Lenable3and4, LOW); //stop motor
    motohalt();
    digitalWrite(greenRelay, LOW);
    Serial.println("DONE in motogotime.");
}  // end motogotime
*/
/*
void motorelax(){
  digitalWrite(Lenable3and4, LOW);
  runtime = 10;
  Serial.println("IN motorelax(). dir, runtime = "+String(dir)+","+String(runtime));
  if(dir == 0){ dir = 1; motogotime(); dir = 0;}
  if(dir == 1){dir = 0; motogotime(); dir = 1;}
  Serial.println("DONE motorelax.");
} // end motorelax()
*/
/*
void slideadjust(){
 adjusterror:
      Serial.println("IN slideadjust()");
      Serial.println("ENTER: milliseconds of runtime ,dir 0 = down, dir 1 = up");
      Serial.println("OR 9,9 to exit motogotime().");
      while (!Serial.available()){ monitorSpi();}
      delay(50);
      while(Serial.available())
      {
      runtime = Serial.parseInt();
      dir = Serial.parseInt();
      if(Serial.read() != '\n'){Serial.println("IN getjob() dir, runtime = " +String(dir)+","+String(runtime));}
      }
      if(runtime == 9 || dir == 9) goto adjustgood;
      if(dir < 0 || dir > 1) goto adjusterror;
      if(runtime < 10  || runtime > 800) goto adjusterror;
      motogotime();
      goto adjusterror;
adjustgood:
      Serial.println("DONE slideadjust()."); 
} // end slideadjust()
*/
