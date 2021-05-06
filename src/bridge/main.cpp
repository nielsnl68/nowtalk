#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <FS.h>
#include "SPIFFS.h"
#include <Update.h>

#define VERSION 2.15
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
    Serial.print(F("NowTalk Server Version:"));
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

    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println("* SPIFFS Mount Failed");
        return;
    }
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
        if (inChar =='*') {
            String x = Serial.readStringUntil((char) 0x00);
            Serial.write('#');
            Serial.write(x.c_str());
        } else  if (inChar == 0x02)
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
                byte data[200];
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
                add_peer(mac);
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
            else if (inChar == 0x27)
            {
                size_t size = 0;
                Serial.readBytes((uint8_t *)size, sizeof(size));

                fs::FS &fs = SPIFFS;
                if (!fs.exists("/update.bin") || fs.remove("/update.bin"))
                {
                    File file = fs.open("/update.bin", FILE_WRITE);
                    if (!file)
                    {
                        Serial.println("! ERROR: cant open file");
                        return;
                    }
                    Serial.printf("* READY: Start reading: %d bytes", size);
                    while (size > 0)
                    {
                        while (Serial.available())
                        {
                            if (!file.write(Serial.read()))
                            {
                                Serial.println("! ERROR cant write any data anymore.");
                            }
                            size--;
                        }
                    }
                    size_t updateSize = file.size();

                    if (updateSize > 0)
                    {
                        Serial.println("* Try to start update");
                        performUpdate(file, updateSize);

                        file.close();
                        delay(1000);
                        ESP.restart();
                    }
                    else
                    {
                        Serial.println("! ERROR: file is empty");
                    }
                }
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
    serialEvent();
}