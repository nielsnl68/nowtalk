#ifndef NOWTALK_VARBS;
#define NOWTALK_VARBS;
typedef struct {
    uint8_t mac[6];
    uint8_t buf[256];
    size_t count;
} nowtalk_t;

#define QUEUE_SIZE  16
#define FORMAT_SPIFFS_IF_FAILED true

static volatile int read_idx = 0;
static volatile int write_idx = 0;
static nowtalk_t circbuf[QUEUE_SIZE];

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

// REPLACE WITH RECEIVER MAC Address
boolean isMaster = false;
String  MasterIP = "<None>";
uint8_t masterAddress[6] =   {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
byte channel  = 0;
///
int EEPROMaddr = 0;
// Structure example to send data
// Must match the receiver structure

typedef struct {
  byte action;
  char info[240];

} struct_message;

// Create a struct_message called myData
struct_message myData;

unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;  // send readings timer

// Replace with your network credentials (STATION)
const char* ssid = "FusionNet";
const char* password = "Lum3nS0fT";
const unsigned long EVENT_INTERVAL_MS = 5000;

DynamicJsonDocument users(ESP.getMaxAllocHeap());

AsyncWebServer server(1234);
AsyncEventSource events("/events");

#endif
