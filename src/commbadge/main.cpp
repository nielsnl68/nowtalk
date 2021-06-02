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

#include <Button2.h>

#include "variables.h"

#include "nowtalk.h"
//#include "esp_adc_cal.h"
#include "switchboard.h"
#include"display.h"


#define BUTTON_1 35
#define BUTTON_2 13
#define BUTTON_3  0

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
Button2 btn3(BUTTON_3);


// Callback when data is sent
void OnDataSent(const uint8_t* mac, esp_now_send_status_t sendStatus)
{
    if (sendStatus != ESP_NOW_SEND_SUCCESS)
    {
        checkReaction();
    }
}

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* buf, int count)
{
    // copy to circular buffer
    int idx = write_idx;
    write_idx = (write_idx + 1) % QUEUE_SIZE;
    
    memcpy(circbuf[idx].mac, mac, 6);
    circbuf[idx].code = buf[0];
    memset(circbuf[idx].buf, 0, 255);
    if (count > 1) {
        memcpy(circbuf[idx].buf, buf + 1, count - 1);
    }
    circbuf[idx].count = count - 1;
}

void handleCommand()
{
    inputString = inputString.substring(3);

    if (inputString.equals("clear"))
    {
        ClearBadge( );
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
        Serial.println(VERSION, 3);
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

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
}

void setup()
{
    // Init Serial Monitor
    Serial.begin(115200);
    //   Serial.println(wakeup);
    config.wakeup = wakeup;
    //   print_wakeup_reason();

    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println("E SPIFFS Mount Failed");
        esp_restart();
        return;
    }

    loadConfiguration();


    inputString.reserve(300);

    // Set device as a Wi-Fi Station
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP_STA);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        vref = adc_chars.vref;
    }


    if (!config.wakeup) {
        initTFT();
        ShowMessage("Version: " + String(VERSION, 3), '*');
        Serial.printf("* Model: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
        Serial.print(F("* MasterIP: "));
        Serial.println(config.masterIP);
        Serial.print(F("* Username: "));
        Serial.println(config.userName);
        Serial.print(F("* MAC Address: "));
        Serial.println(WiFi.macAddress());
        Serial.print(F("* WiFi Channel: "));
        Serial.println(WiFi.channel());
        delay(2500);
    }

    if (esp_now_init() != 0) {
        ShowMessage("Error initializing ESP-NOW", 'E');
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    btn1.setClickHandler(OnRegisterBadge);
    btn2.setClickHandler(OnRegisterBadge);
    btn3.setClickHandler(OnRegisterBadge);

    btn3.setTripleClickHandler(OnClearBadge);
    btn3.setLongClickDetectedHandler(OnLongPress);
    btn3.setLongClickTime(1000);
    
    config.wakeup = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
    if (!config.wakeup) {
        myEvents.free(0);
        if (config.registrationMode) {
            ShowMessage("New Badge!\n\nPress Enter Button\nto register...", '!');

            myEvents.timerOnce(1, OnPing);
            myEvents.disable(0);
        }
        else {
            ShowMessage("READY", '*');
            uint32_t i = config.timerDelay * (config.masterAddress[0] == 0) ? 5 : 1;
            myEvents.timerOnce(i, OnPing);
        }
        config.sleepID = myEvents.timerOnce(config.timerSleep, OnGoSleep);
    }
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
    btn3.loop();
}


void loop()
{
    button_loop();
    myEvents.loop();
    if (read_idx != write_idx)
    {
        nowtalk_t espnow = circbuf[read_idx];
        handlePackage(espnow.mac, espnow.code, espnow.buf, espnow.count);
        read_idx = (read_idx + 1) % QUEUE_SIZE;
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