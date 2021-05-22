/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp8266-nodemcu-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <TFT_eSPI.h>

#include "variables.h"

 #include "nowtalk.h"
 #include "esp_adc_cal.h"
 #include "switchboard.h"
#include "Button2.h"

#define BUTTON_1 35
#define BUTTON_2 13
#define BUTTON_3  0

 TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
Button2 btn3(BUTTON_3);

// Callback when data is sent
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    if (sendStatus != ESP_NOW_SEND_SUCCESS)
    {
        /// do something;
    }
}

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *buf, int count)
{
    // copy to circular buffer
  
    nowtalk_t data;
    memcpy(data.mac, mac, 6);
    data.code = buf[0];
    memset(data.buf, 0, 255);
    if (count > 1)
    {
        memcpy(data.buf, buf + 1, count - 1);
    }
    data.count = count - 1;
    circbuf[write_idx] = data;
    write_idx = (write_idx + 1) % QUEUE_SIZE;
}

void handleCommand()
{
    inputString = inputString.substring(3);

    if (inputString.equals("clear"))
    {
        loadConfiguration(true);
        ESP.restart();
    }
    else if (inputString.equals("info"))
    {
        Serial.print(badgeID());
        Serial.print('~');
        if (config.masterAddress[0] == 0)
        {
            Serial.print("<none>");
        }
        else
        {
            serial_mac(config.masterAddress);
        }
        Serial.print('~');
        Serial.print(config.masterIP);
        Serial.print('~');
        Serial.print(config.userName);
        Serial.print('~');
        Serial.println(VERSION,3);
    }
    else
    {
        Serial.println("ERROR: >" + inputString + "<");
    }
}

void serialEvent()
{
    while (Serial.available())
    {
        // get the new byte:
        char inChar = (char)Serial.read();
        if (inChar == '\r')
            continue;
        if (inChar == '\n')
        {
            stringComplete = true;
        }
        else
        {
            inputString += inChar;
        }
    }
}
void ClearBadge(Button2& b) {    
    loadConfiguration(true);
    ESP.restart();
}

void RegisterBadge(Button2& b)
{
    if (config.registrationMode)
    {
        broadcast(ESPTALK_CLIENT_NEWPEER, "");
        // config.registrationMode = true;
        message("Badge ID:\n" + badgeID());
    }
    else if (config.masterAddress[0] != 0)
    {
        send_message(config.masterAddress, ESPTALK_CLIENT_START_CALL, "");
    }
    else
    {
        broadcast(ESPTALK_CLIENT_PING, "");
    }
}

void setup()
{
    // Init Serial Monitor
    Serial.begin(115200);

    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println("* SPIFFS Mount Failed");
        return;
    }
    loadConfiguration();

    inputString.reserve(300);

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_AP_STA);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());

    message("Application version: " + String(VERSION, 3));

    Serial.print(F("* MasterIP: "));
    Serial.println(config.masterIP);
    Serial.print(F("* Username: "));
    Serial.println(config.userName);
    Serial.print(F("* MAC Address: "));
    Serial.println(WiFi.macAddress());
    Serial.print(F("* WiFi Channel: "));
    Serial.println(WiFi.channel());

    if (esp_now_init() != 0)
    {
        Serial.println("* Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    config.lastTime = millis() - (config.timerDelay + 10);

    btn1.setClickHandler(RegisterBadge);
    btn2.setClickHandler(RegisterBadge);
     btn3.setTripleClickHandler(ClearBadge);

    if (config.registrationMode) {
        message("New Badge!\nPress Enter Button.");
    }
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}


void loop()
{
    button_loop();
    if (read_idx != write_idx)
    {
        nowtalk_t espnow = circbuf[read_idx];
        handlePackage(espnow.mac, espnow.code, espnow.buf, espnow.count);
        read_idx = (read_idx + 1) % QUEUE_SIZE;
    }
    if (!config.registrationMode)
    {
        if ((millis() - config.lastTime) > config.timerDelay + (config.masterAddress[0] == 0 ? 5 : 1))
        {
            if (config.masterAddress[0] == 0)
            {
                Serial.println("Search master");
                broadcast(0x01, "");
            }
            else
            {
                send_message(config.masterAddress, 0x01, "");
            }
            config.lastTime = millis();
        }
    }
    if (stringComplete)
    {

        if (inputString.startsWith("###"))
        {
            handleCommand();
        }
        // clear the string:
        inputString = "";
        stringComplete = false;
    }
    else
    {
        serialEvent();
    }
}