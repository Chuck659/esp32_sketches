#include <Arduino.h>
#include "esp32Globals.h"

int loopCount = 0;
unsigned long runTimer = millis();
//String hitData;

// Forward defines
void monitorCmd();
void setpins();
void targetmove();
static void getJob();
void checkRunStatus();
void sendToPeer(unsigned char cmd, const char* buffer, int len);

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
char pgmname2[] = "LSW-Target-phone-run-verOct23";
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
  
  gTargetState = STATUS_IDLE;
//
// ======  target movement setup
//
//  setpins();
//  loopcount = 0;
//  runtime = 0;
//  upperlimit = 1;
//  lowerlimit = 1;
//  digitalWrite(greenRelay,HIGH);
//  delay(1000);
//  digitalWrite(greenRelay,LOW);
//  digitalWrite(yellowRelay,HIGH);
//  delay(1000);
//  digitalWrite(yellowRelay,LOW);
//  digitalWrite(redRelay,HIGH);
//  delay(1000);
//  digitalWrite(redRelay,LOW);
  Serial.println("DONE w/ setup()");
//  zrestvibes();
  // slideadjust();
//
//  ====  end target movement setup
//

  }  // end setup

void targetLoop() {
  loopCount++;
  if(loopCount == 1) Serial.println("In loop().");
  getJob();
}  // end of loop()

// =================== Application code ================
//

static void getJobMenu() {
  Serial.println("GETJOB():  enter the item number to run");
  Serial.println("ITEM    function   Description");
  Serial.println("0, refresh menu");
  Serial.println("1, get local status");
  Serial.println("2, start exercise");
  Serial.println("3, end of exercise");
  Serial.println("99. start getjob() again (EXIT)");
}

static void getLocalStatus() {
  Serial.println("========== Local gTargetState ===========");
  Serial.println("========== Local gTargetState ===========");
}

static void getJob(){
  int jobNumber;
  bool done = false;
  getJobMenu();

  while (!done) {
    while (!Serial.available()){ 
      monitorCmd();
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
      getLocalStatus();
    break;
    case 2:
      targetmove();
      break;
    case 3:
      if (gTargetState == STATUS_RUNNING) gTargetState = STATUS_RUN_COMPLETE;
      break;
    case 99:
      done = true;
    break;
    } // end of switch
  }
  Serial.println("DONE in getjob().");
} // end getjob()

void checkRunStatus() {
  unsigned long now = millis();
  if ((gTargetState == STATUS_RUNNING) && ((now - runTimer) > 5000)) {
    gTargetState = STATUS_RUN_COMPLETE;
//    hitData = "Random hit data from Arduino  ";
//    hitData += String(millis());
//    sendToPeer(HITDATA, hitData.c_str(), hitData.length());
    Serial.println("Run Complete");
  }
}
// =========== end of Slave application code  ===========================================
//

void monitorCmd() {
  if (gWifiCommand > 0) {
    switch (gWifiCommand) {
      case RUNCMD:
        Serial.println("Run command received");
        runTimer = millis();
        targetmove();
      break;
      case F1CMD:
        Serial.println("Function 1 command received");
      break;
      case F2CMD:
      Serial.println("Function 2 command received");
      break;
      case F3CMD:
        Serial.println("Function 3 command received");
        if (gTargetState == STATUS_RUNNING) gTargetState = STATUS_RUN_COMPLETE;
      break;
      case F4CMD:
        Serial.println("Function 4 command received");
      break;
      case F5CMD:
        Serial.println("Function 5 command received");
      break;
      case F6CMD:
        Serial.println("Function 6 command received");
      break;
      case F7CMD:
        Serial.println("Function 7 command received");
      break;
      default:
        Serial.println(String("Unknown command: ") + String(gWifiCommand));
    }
    gWifiCommand = 0;
  }
  delay(1);
}  // end monitorCmd()

//
// Send message to peer
//
void sendToPeer(unsigned char cmd, const char* buffer, int len) {
  if (!gTargetDataReady) {
    Serial.println(String("Send to peer: ") + buffer);
    gDataBuffer = buffer;
    gTargetDataLength = len;
    gTargetDataReady = true;
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


void motogolimit(){
  Serial.println("IN motogolimit.");
  gTargetState = STATUS_RUNNING;
  startuptime = millis();
  movetime = startuptime;
  uprecord = "Zu";
  uprecord += String(startuptime);
  zval = analogRead(analogPinz);
  Serial.println("DONE loop of moving up.");
  gTargetState = STATUS_RUN_COMPLETE;      
  Serial.println("DONE in motogolimit().");
  sendToPeer(HITDATA, uprecord.c_str(), uprecord.length() <= 80 ? uprecord.length() : 80);  // USE THIS
  delay(1000);  
} // end motogolimit()


void targetmove(){
tmerror1:
  motogolimit(); // goes up, then down
}
