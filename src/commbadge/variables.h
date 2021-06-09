#pragma once

#ifndef NOWTALK_VARBS
#define NOWTALK_VARBS

#define ARDUINOJSON_USE_LONG_LONG 1
#include <Arduino.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include "TimeEvents.h"
#include "Version.h"

#ifndef VERSION
#define VERSION "0.0.0"
#endif


/// Request message codes :

#define NOWTALK_CLIENT_PING 0x01
#define NOWTALK_SERVER_PONG 0x02
#define NOWTALK_SERVER_REQUEST_DETAILS 0x03
#define NOWTALK_CLIENT_DETAILS 0x04
#define NOWTALK_CLIENT_NEWPEER 0x05

#define NOWTALK_SERVER_ACCEPT 0x07

#define NOWTALK_SERVER_NEW_NAME 0x0d
#define NOWTALK_SERVER_NEW_IP 0x0e

#define NOWTALK_CLIENT_ACK 0x10
#define NOWTALK_CLIENT_NACK 0x11

#define NOWTALK_CLIENT_START_CALL 0x30
#define NOWTALK_SERVER_SEND_PEER 0x31
#define NOWTALK_SERVER_PEER_GONE 0x32
#define NOWTALK_SERVER_OVER_WEB 0x33

#define NOWTALK_CLIENT_REQUEST 0x37
#define NOWTALK_CLIENT_RECEIVE 0x38
#define NOWTALK_CLIENT_CLOSED 0x39

#define NOWTALK_STREAM_START 0x3d
#define NOWTALK_STREAM_DATA 0x3e
#define NOWTALK_STREAM_END 0x3df


#define NOWTALK_CLIENT_HELPSOS 0xff



#define NOWTALK_STATUS_GONE 0x00
#define NOWTALK_STATUS_ALIVE 0x01
#define NOWTALK_STATUS_GUEST 0x02


#define NOWTALK_PEER_MEMBER 0x10
#define NOWTALK_PEER_FRIEND 0x20
#define NOWTALK_PEER_BLOCKED 0x80


struct config_t
{
    boolean wakeup = false;
    unsigned long lastTime = 0;
    unsigned long timerPing = 30000; // send readings timer
    unsigned long timerSleep  = 90000;
    bool registrationMode = false;
    char masterIP[40] = "<None>";
    char userName[64] = "<None>";
    uint8_t masterSwitchboard[6] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    byte channel = 0;
    char switchboard[32] = "";
    boolean TFTActive = false;
    int ledBacklight = 80; // Initial TFT backlight intensity on a scale of 0 to 255. Initial value is 80.
    int sprVolume = 40;
    int sleepID = -1;
    int PingID = -1;
    int timeoutID = -1;
    short heartbeat = 0;
    size_t updateSize = 0;
};
const char* configFile = "/config.json"; // <- SD library uses 8.3 filenames

config_t config;//

RTC_DATA_ATTR boolean wakeup = false;
RTC_DATA_ATTR uint8_t currentSwitchboard[6] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

alarm_t alarms[dtNBR_ALARMS];
TimeEventsClass myEvents = TimeEventsClass(alarms);

typedef struct
{
    uint8_t mac[6];
    uint8_t code;
    char buf[256];
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
nowtalk_t circbuf[QUEUE_SIZE] = {};

String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete

uint16_t vref = 1100;

String GetExternalIP()
{
    String ip = "";

    HTTPClient http;
    http.begin("http://api.ipify.org/?format=text");
    int statusCode = http.GET();
    if (statusCode == 200)
    {
        ip = http.getString();
    }
    http.end();
    return ip;
}


String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


String badgeID()
{
    static char baseChars[] = "0123456789AbCdEfGhIjKlMnOpQrStUvWxYz"; //aBcDeFgHiJkLmNoPqRsTuVwXyZ
    uint8_t base = sizeof(baseChars);
    String result = "";
    uint32_t chipId = 0xa5000000;
    uint8_t crc = 0;

    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    do {
        result = String(baseChars[chipId % base]) + result; // Add on the left
        crc += chipId % base;
        chipId /= base;
    } while (chipId != 0);
    return result + String(baseChars[crc % base]);
}

// load configuration from a file
void loadConfiguration(bool clear = false)
{
    if (clear)
        SPIFFS.remove(configFile);
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<524> doc;
    
    // memset(*config, 0, sizeof(config));
    // Open file for reading
    if (SPIFFS.exists(configFile)) {
        fs::File file = SPIFFS.open(configFile);
        deserializeJson(doc, file);
        file.close();
    }
   // serializeJsonPretty(doc, Serial);
    // Copy values from the JsonDocument to the Config
    config.channel = doc["channel"] | 0;
    //config.port = doc["port"] | 2731;
    strlcpy(config.switchboard, doc["switchboard"] | "", sizeof(config.switchboard)); // <- destination's capacity

    config.timerPing = doc["timerPing"] | 30000;      // send readings timer
    config.timerSleep = doc["timerSleep"] |  90000; //EVENT_INTERVAL_MS
    config.ledBacklight = doc["ledBacklight"] | 80; // Initial TFT backlight intensity on a scale of 0 to 255. Initial value is 80.
    config.sprVolume = doc["sprVolume"] | 40;
    config.heartbeat = doc["heartbeat"] | 0;
    

    strlcpy(config.masterIP, doc["masterIP"] | "<None>", sizeof(config.masterIP)); // <- destination's capacity
    strlcpy(config.userName, doc["userName"] | "<None>", sizeof(config.userName)); // <- destination's capacity
    if ((strcmp(config.masterIP, "<None>") == 0) || (strcmp(config.userName, "<None>") == 0))
    {
        config.registrationMode = true;
    }
    
    for (JsonObject elem : doc["alarms"].as<JsonArray>()) {
        int id = elem["id"];
        uint64_t nt = elem["nextTrigger"].as<uint64_t>();
        alarms[id].value = elem["time"];
        alarms[id].nextTrigger = nt;
        alarms[id].Mode.alarmType = elem["mode_alarmType"];
        alarms[id].Mode.isEnabled = elem["mode_isEnabled"]; 
        alarms[id].Mode.isOneShot = elem["mode_isOneShot"]; 
        alarms[id].onTickHandler = (OnTick_t)elem["trigger"].as<uint32_t>();
        Serial.printf("timeleft %lld\n", myEvents.timeNow() - nt);
        myEvents.updateNextTrigger(id);
    }
}

// Saves the configuration to a file
void saveConfiguration()
{
    // Delete existing file, otherwise the configuration is appended to the file
    SPIFFS.remove(configFile);

     fs::File file = SPIFFS.open(configFile, FILE_WRITE);
    if (!file)    {
        Serial.println(F("E Failed to create file"));
        return;
    }

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<524> doc;

    // Copy values from the JsonDocument to the Config
    doc["channel"] = config.channel;
    //doc["port"] = config.port;
    doc["switchboard"] = config.switchboard;

    doc["timerPing"] = config.timerPing;       // send readings timer
    doc["timerSleep"] = config.timerSleep; //EVENT_INTERVAL_MS

    doc["masterIP"] = config.masterIP;
    doc["userName"] = config.userName;
    doc["ledBacklight"] = config.ledBacklight ; // Initial TFT backlight intensity on a scale of 0 to 255. Initial value is 80.
    doc["sprVolume"] = config.sprVolume;

    JsonArray data = doc.createNestedArray("alarms");
    for (uint8_t id = 0; id < dtNBR_ALARMS; id++) {
        if (alarms[id].Mode.alarmType != dtNotAllocated) {
            JsonObject data_0 = data.createNestedObject();
            data_0["id"]         = id;
            data_0["time"]     = alarms[id].value;
            data_0["nextTrigger"] = alarms[id].nextTrigger;
            data_0["mode_alarmType"] = alarms[id].Mode.alarmType;
            data_0["mode_isEnabled"] = alarms[id].Mode.isEnabled;
            data_0["mode_isOneShot"] = alarms[id].Mode.isOneShot;
            data_0["trigger"] = (uint32_t) alarms[id].onTickHandler;
        }
    }
    
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
}

#endif