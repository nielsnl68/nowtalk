#ifndef NOWTALK_WEB
#define NOWTALK_WEB

#include "ESPAsyncWebServer.h"
#include "nowtalk.h"
#include "variables.h"

AsyncWebServer server(1234);
AsyncEventSource events("/events");

unsigned long lastEventTime = millis();

// callback function that will be executed when data is received
void handleServerMsg(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    // Copies the sender mac address to a string
    /*
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  
  board["id"] = incomingReadings.id;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.hum;
  board["readingId"] = String(incomingReadings.readingId);
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());

  */
}

void webLoop(){
    if (config.isMaster)
    {
        if ((millis() - lastEventTime) > config.eventInterval)
        {
            events.send("ping", NULL, millis());
            lastEventTime = millis();
        }
    }
}

void webSetup() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        ////   request->send_P(200, "text/html", index_html);
    });

    events.onConnect([](AsyncEventSourceClient *client) {
        if (client->lastId())
        {
            Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
        }
        // send event with message "hello!", id current millis
        // and set reconnect delay to 1 second

        StaticJsonDocument<256> doc;
        doc["version"] = VERSION;
        doc["masterIP"] = config.masterIP;
        doc["externelIP"] = GetExternalIP();
        doc["localIP"] = IpAddress2String(WiFi.localIP());
        doc["gatewayIP"] = IpAddress2String(WiFi.gatewayIP());
        doc["mac"] = WiFi.macAddress();
        doc["channel"] = WiFi.channel();
        char jsonString[512];
        serializeJson(doc, jsonString);
        client->send(jsonString, "new_status", millis());
    });

    server.addHandler(&events);
    server.begin();
}
#endif