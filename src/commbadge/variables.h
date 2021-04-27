#ifndef NOWTALK_VARBS
#define NOWTALK_VARBS

#include "FS.h"
#include "SPIFFS.h"

#define VERSION 32.230

struct config_t
{
  unsigned long lastTime = 0;
  unsigned long timerDelay = 30000; // send readings timer
  char ssid[32] = "";
  char password[64] = "";
  unsigned long eventInterval = 5000;

  boolean allowGuests = true;
  boolean isMaster = false;
  char masterIP[32] = "<None>";
  char userName[64] = "<None>";
  uint8_t masterAddress[6] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  byte channel = 0;
  char hostname[64] = "";
  int port;
  const char *configFile = "/config.json"; // <- SD library uses 8.3 filenames
};

config_t config;

typedef struct
{
  uint8_t mac[6];
  uint8_t buf[256];
  size_t count;
} nowtalk_t;

typedef struct
{
  uint8_t status;
  uint8_t mac[6];
  char name[64];
  char orginIP[32];
  byte prev;
  byte next;
} nowtalklist_t;

const byte _maxUserCount = 24;

typedef struct
{
  byte first_element = 0;
  byte last_element = 0;
  byte current = 0xff;
  byte previous = 0xff;
  byte count = 0;
  const byte maxCount = _maxUserCount;
} nowtalklist_stats_t;
nowtalklist_stats_t stats;

nowtalklist_t users[_maxUserCount];

#define QUEUE_SIZE 16
#define FORMAT_SPIFFS_IF_FAILED true

static volatile int read_idx = 0;
static volatile int write_idx = 0;
static nowtalk_t circbuf[QUEUE_SIZE];

String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete

// REPLACE WITH RECEIVER MAC Address
///
// Structure example to send data
// Must match the receiver structure

// Replace with your network credentials (STATION)

/// Request message codes :

#define ESPTALK_CLIENT_PING 0x01
#define ESPTALK_SERVER_PONG 0x02
#define ESPTALK_SERVER_REQUEST_DETAILS 0x03
#define ESPTALK_CLIENT_DETAILS 0x04
#define ESPTALK_CLIENT_NEWPEER 0x05

#define ESPTALK_SERVER_REJECT 0x08
#define ESPTALK_SERVER_ACCEPT 0x09

#define ESPTALK_SERVER_NEW_NAME 0x0d
#define ESPTALK_SERVER_NEW_IP 0x0e
#define ESPTALK_CLIENT_ACK 0x0f

#define ESPTALK_CLIENT_START_CALL 0x30
#define ESPTALK_SERVER_SEND_PEER 0x31
#define ESPTALK_SERVER_PEER_GONE 0x32
#define ESPTALK_SERVER_OVER_WEB 0x33

#define ESPTALK_CLIENT_REQUEST 0x37
#define ESPTALK_CLIENT_RECEIVE 0x38
#define ESPTALK_CLIENT_CLOSED 0x39

#define ESPTALK_CLIENT_STREAM 0x3df

#define ESPTALK_SERVER_UPGRADE 0xe0
#define ESPTALK_SERVER_SENDFILE 0x31
#define ESPTALK_CLIENT_RECEIVED 0x32

#define ESPTALK_CLIENT_HELPSOS 0xff

#define ESPTALK_FINDPEER_NOTFOUND 0x00
#define ESPTALK_FINDPEER_BLOCKED 0x01
#define ESPTALK_FINDPEER_GUEST 0x02
#define ESPTALK_FINDPEER_FOUND 0x03

#define ESPTALK_PEER_MEMBER 0x10
#define ESPTALK_PEER_FRIEND 0x20
#define ESPTALK_PEER_BLOCKED 0x40

#define ESPTALK_STATUS_ALIVE 0x01
#define ESPTALK_STATUS_EXTERN 0x02
#define ESPTALK_STATUS_NEW 0x04
#define ESPTALK_STATUS_GONE 0x00

#include <ArduinoJson.h>

// load configuration from a file
void loadConfiguration(bool clear = false)
{
  if (clear)
    SPIFFS.remove(config.configFile);

  // Open file for reading
  File file = SPIFFS.open(config.configFile);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<524> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));
  file.close();

  // Copy values from the JsonDocument to the Config
  config.allowGuests = doc["userName"] | true;
  config.isMaster = doc["userName"] | false;

  config.channel = doc["userName"] | 0;
  config.port = doc["port"] | 2731;
  strlcpy(config.hostname, doc["hostname"] | "example.com", sizeof(config.hostname)); // <- destination's capacity

  config.timerDelay = doc["timerDelay"] | 30000;      // send readings timer
  config.eventInterval = doc["eventInterval"] | 5000; //EVENT_INTERVAL_MS

  strlcpy(config.ssid, doc["ssid"] | "", sizeof(config.ssid));             // <- destination's capacity
  strlcpy(config.password, doc["password"] | "", sizeof(config.password)); // <- destination's capacity

  strlcpy(config.masterIP, doc["masterIP"] | "", sizeof(config.masterIP)); // <- destination's capacity
  strlcpy(config.userName, doc["userName"] | "", sizeof(config.userName)); // <- destination's capacity
}

// Saves the configuration to a file
void saveConfiguration()
{
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(config.configFile);

  File file = SPIFFS.open(config.configFile, FILE_WRITE);
  if (!file)
  {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<524> doc;

  // Copy values from the JsonDocument to the Config
  doc["allowGuests"] = config.allowGuests;
  doc["isMaster"] = config.isMaster;

  doc["userName"] = config.channel;
  doc["port"] = config.port;
  doc["hostname"] = config.hostname;

  doc["timerDelay"] = config.timerDelay;       // send readings timer
  doc["eventInterval"] = config.eventInterval; //EVENT_INTERVAL_MS

  doc["ssid"] = config.ssid;
  doc["password"] = config.password;

  doc["masterIP"] = config.masterIP;
  doc["userName"] = config.userName;

  if (serializeJson(doc, file) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}

#endif
