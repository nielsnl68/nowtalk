#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define VERSION 2.25
#define FORMAT_SPIFFS_IF_FAILED true

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t channel = 0;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    if (sendStatus != ESP_NOW_SEND_SUCCESS)
    {
        Serial.write(0x02);
        Serial.write(mac, 6);
        Serial.write(0x01);
        Serial.write(0x77);
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *buf, int count)
{
    if (count > 250)
        count = 250;
    Serial.write(0x02);
    Serial.write(mac, 6);
    Serial.write(count);
    Serial.write(buf, count);
}

void add_peer(const uint8_t *mac)
{
    // Register peer
    if (!esp_now_is_peer_exist(mac))
    {
        esp_now_peer_info_t peerInfo = {};
        memcpy(&peerInfo.peer_addr, mac, 6);
        peerInfo.channel = channel;
        // Add peer
        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            Serial.write(0x04);
            Serial.write(mac, 6);
            Serial.write(0x00);
            return;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.print(F(" NowTalk Server Version:"));
    Serial.println(VERSION, 3);
    Serial.println("Booting");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.print("MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.print(F("WiFi Channel: "));
    channel = WiFi.channel();
    Serial.println(channel);

    // Init ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    add_peer(broadcastAddress);

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Running");
}

unsigned long interval = 1000;
void performUpdate(Stream &updateSource, size_t updateSize);

void serialEvent()
{
    unsigned long previousMillis = millis();
    while (Serial.available())
    {
        // get the new byte:
        byte inChar = Serial.read();
        if (inChar == '*')
        {
            String x = Serial.readStringUntil((char)0x00);
            Serial.write('#');
            Serial.write(x.c_str());
        }
        else if (inChar == 0x02)
        {
            byte mac[6];
            byte len;
            previousMillis = millis();
            while (Serial.available() < 7)
            {
                if (millis() - previousMillis > interval)
                {
                    return;
                }
            }
            Serial.readBytes(mac, 6);
            len = Serial.read();
            byte data[256];
            previousMillis = millis();
            while (Serial.available() < len)
            {
                if (millis() - previousMillis > interval)
                {
                    return;
                }
            }
            Serial.readBytes(data, len);
            add_peer(mac);
            esp_now_send(mac, data, len);
        }
        else if (inChar == 0x03)
        {
            byte mac[6];
            previousMillis = millis();
            while (Serial.available() < 6)
            {
                if (millis() - previousMillis > interval)
                {
                    return;
                }
            }
            Serial.readBytes(mac, 6);
            esp_now_del_peer(mac);
        }
        else if (inChar == (byte)'W') // 0x27
        {
            String ssid = Serial.readStringUntil('~');
            String password = Serial.readStringUntil('~');
            Serial.println("! Sidd..." + ssid);
            Serial.println("! pw..." + password);
            // WFusionNet~Lum3nS0fT~
            WiFi.begin(ssid.c_str(), password.c_str());
            while (WiFi.waitForConnectResult() != WL_CONNECTED)
            {
                Serial.println("! Connection Failed! Rebooting...");
                delay(5000);
                ESP.restart();
            }
            ArduinoOTA
                .onStart([]() {
                    String type;
                    if (ArduinoOTA.getCommand() == U_FLASH)
                        type = "sketch";
                    else // U_SPIFFS
                        type = "filesystem";

                    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                    Serial.println("* Start updating " + type);
                })
                .onEnd([]() {
                    Serial.println("* End");
                })
                .onProgress([](unsigned int progress, unsigned int total) {
                    Serial.printf("* Progress: %u%%", (progress / (total / 100)));
                })
                .onError([](ota_error_t error) {
                    Serial.printf("! Error[%u]: ", error);
                    if (error == OTA_AUTH_ERROR)
                        Serial.println(" Auth Failed");
                    else if (error == OTA_BEGIN_ERROR)
                        Serial.println(" Begin Failed");
                    else if (error == OTA_CONNECT_ERROR)
                        Serial.println(" Connect Failed");
                    else if (error == OTA_RECEIVE_ERROR)
                        Serial.println(" Receive Failed");
                    else if (error == OTA_END_ERROR)
                        Serial.println(" End Failed");
                });
            ArduinoOTA.setHostname("nowTalkBridge");

            ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

            ArduinoOTA.begin();
            Serial.print("* IP address: ");
            Serial.println(WiFi.localIP());
            Serial.println("@" + ArduinoOTA.getHostname());
        }
    }
}
// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize)
{
    if (Update.begin(updateSize))
    {
        size_t written = Update.writeStream(updateSource);
        if (written == updateSize)
        {
            Serial.println("* Written : " + String(written) + " successfully");
        }
        else
        {
            Serial.println("! Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        }
        if (Update.end())
        {
            Serial.println("* Update is  done!");
            if (Update.isFinished())
            {
                Serial.println("* Update successfully completed. Rebooting.");
            }
            else
            {
                Serial.println("!  Update not finished? Something went wrong!");
            }
        }
        else
        {
            Serial.println("! Error Occurred. Error #: " + String(Update.getError()));
        }
    }
    else
    {
        Serial.println("! Not enough space to begin Update process");
    }
}

void rebootEspWithReason(String reason)
{
    Serial.println(reason);
    delay(1000);
    ESP.restart();
}

void loop()
{
    ArduinoOTA.handle();
    serialEvent();
}