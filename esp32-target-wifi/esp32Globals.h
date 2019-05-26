// Define CREATE in main file
#ifdef CREATE
int gTargetState;
int gWifiCommand;
int gTargetDataReady;
int gTargetDataLength;
const char* gDataBuffer;
int gWifiReady;
#else
extern int gTargetState;
extern int gWifiCommand;
extern int gTargetDataReady;
extern int gTargetDataLength;
extern const char* gDataBuffer;
extern int gWifiReady;
#endif

#define STATUS_UNKNOWN -1
#define STATUS_IDLE 1
#define STATUS_RUNNING 2
#define STATUS_RUN_COMPLETE 3

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
#define ACK 0x40

#define LED 2
