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
#include "rom/crc.h"
#include <esp_wifi.h> 

#include <Button2.h>

#include "variables.h"

#include "nowtalk.h"
//#include "esp_adc_cal.h"
#include "switchboard.h"
#include"display.h"
extern "C" {
#include "crypto/base64.h"
}
#include"audio.h"

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
        ClearBadge();
    }
    else if (inputString.startsWith("init~"))
    {
        if (config.registrationMode)
        {
            String checkID = getValue(inputString, '~', 1);
            String IP = getValue(inputString, '~', 2);
            String name = getValue(inputString, '~', 3);
            String network = getValue(inputString, '~', 4);
            String mac = getValue(inputString, '~', 5);

            uint16_t crc = getValue(inputString, '~', 6).toInt();

            String test = "nowTalkSrv!" + checkID + "~" + IP + "~" + name + "~" + network + "~" + mac;

            uint16_t checkCrc = crc16_le(0, (uint8_t const*)test.c_str(), test.length());
            //            uint16_t checkCrc crc16((uint8_t *)test.c_str(), test.length, 0x1021, 0, 0, false, false);
            if ((crc == checkCrc) && (checkID == badgeID())) { //
                strlcpy(config.masterIP, IP.c_str(), sizeof(config.masterIP)); // <- destination's capacity
                strlcpy(config.userName, name.c_str(), sizeof(config.userName)); // <- destination's capacity
                strlcpy(config.switchboard, network.c_str(), sizeof(config.switchboard)); // <- destination's capacity

                size_t outputLength;
                unsigned char* decoded = base64_decode((const unsigned char*)mac.c_str(), mac.length(), &outputLength);
                memcpy(config.masterSwitchboard, decoded, 6);
                memcpy(currentSwitchboard, decoded, 6);
                free(decoded);
                saveConfiguration();
                Serial.print("# ACK");
                ShowMessage('*', ("Switchboard:\n%s\n\nCallname: %s"), config.switchboard, config.userName);
                config.registrationMode = false;
                return;
            }
            else {
                Serial.print("# NACK");
                ShowMessage(F("Wrong Badge Id!\nPress Enter Button."), '!');
            }
        }

    }
    else if (inputString.startsWith("info~"))
    {
        if (config.masterSwitchboard[0] != 0) {
            String mac = getValue(inputString, '~', 1);
            
            byte decoded[6];
            mac.replace(":", "");
            hexStringToBytes(mac.c_str(), decoded);
            boolean okay = memcmp(config.masterSwitchboard, (char*)decoded, 6) == 0;
            if (!okay) {
                Serial.println("# NACK");
                return;
            }
        }
        uint8_t mac[6];
        Serial.print("# ");
        serial_mac(WiFi.macAddress(mac));
        Serial.print('~');
        Serial.print(badgeID());
        Serial.print('~');
        Serial.print(config.masterIP);
        Serial.print('~');
        Serial.print(config.userName);
        Serial.print('~');
        Serial.print(VERSION);
        Serial.print('\n');
    }
    else
    {
        Serial.printf(("E ERROR: > %s<\n"), inputString.c_str());
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

            if (inputString.startsWith("###"))
            {
                handleCommand();
            }
            // clear the string:
            inputString = "";

        }
        else
        {
            inputString += inChar;
        }
    }
}


void setup()
{
    // Init Serial Monitor
    Serial.begin(115200);
    //   Serial.println(wakeup);

    config.wakeup = wakeup;

    loadConfiguration();

    inputString.reserve(300);

    // Set device as a Wi-Fi Station
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP_STA);
#ifdef CONFIG_ESPNOW_ENABLE_LONG_RANGE
    esp_wifi_get_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR);
#endif

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

    audio_setup();
    
    if (!config.wakeup) {
        initTFT();
        Serial.printf("* Model: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
        Serial.printf("* Version: %s\n", VERSION);
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
        ShowMessage(F("Error initializing ESP-NOW"), 'E');
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
            ShowMessage("New Badge!", '!');
        }
        else {
            ShowMessage("READY", '*');
            OnPing(-1);
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

    serialEvent();
}