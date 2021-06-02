#ifndef NOWTALK_DISPLAY
#define NOWTALK_DISPLAY

//#include "Pangodream_18650_CL.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include "esp_adc_cal.h"
#include <Wire.h>
#include <WiFi.h>
#include "variables.h"
#include "nowTalkLogo.h"
#include "battery.h"
#include "nowtalk.h"

void ShowMessage(const String msg, char SerialPrefix = ' ');
void ShowMessage(const char* msg, char SerialPrefix = ' ');
void initTFT();

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34


#define TFT_BL          4  // Display backlight control pin

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
// Setting PWM properties, do not change this!

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;


//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}


void GoSleep();

void showVoltage(bool init = false)
{
    static uint64_t timeStamp = 0;
    if (config.TFTActive) {
        if (init) {
            tft.fillScreen(TFT_BLACK);
            tft.setTextDatum(TL_DATUM);
            tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK); // Orange
           // tft.fillRect(0, 0, tft.width(), 20, TFT_BLACK);
            int x = tft.drawString("Name:", 2, 2, 2);
            tft.setTextColor(TFT_WHITE, TFT_BLACK); // Orange
            tft.drawLine(0, 20, tft.width(), 20, TFT_LIGHTGREY);
            tft.setCursor(x + 6, 2, 2);
            tft.print(config.userName); //badgeID()
        }
    }

    if (millis() - timeStamp > 1000 || init) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = String(battery_voltage, 1) + "V";
        if (config.TFTActive) {
            tft.setTextDatum(TR_DATUM);
            tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK); // Orange
            tft.drawString(voltage, tft.width() - 2, 2, 2);
        }
        else {
            voltage = "Battery: " + voltage;
            if (battery_voltage < 3.2) ShowMessage(voltage, '!');
        }
        if (battery_voltage < 2.4) GoSleep();
    }
}

void drawString(const char* msg) {
    int len = strlen(msg), i = 0, w;
    int rows = 1, maxWidth = 0;
    uint8_t pos[5] = { 0,0,0,0,0 };
    char* pch;
    char buf[255];
    strcpy(buf, msg);
    tft.setTextSize(1);
    pch = strchr(msg, '\n');
    while (pch != NULL) {
        i = pch - msg;
        pos[rows] = i + 1;
        memset(buf, 0, 255);
        strncpy(buf, msg + pos[rows - 1], (i - pos[rows - 1]));
        rows++;
        w = tft.textWidth(buf);
        if (w > maxWidth)maxWidth = w;
        pch = strchr(pch + 1, '\n');
    }
    memset(buf, 0, 255);
    strncpy(buf, msg + pos[rows - 1], (len - pos[rows - 1]));
    pos[rows] = len;
    i = tft.textWidth(buf);
    if (i > maxWidth)maxWidth = i;
    int heigth = rows * (tft.fontHeight() + 1);
    i = 1;
    while (heigth * i <= (tft.height() - 20) && maxWidth * i <= tft.width()) { i++; }
    if (i > 1) i--;
    int h = ((tft.height() - 20) / 2) - ((heigth * i) / 2);
    w = tft.width() / 2;
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(i);
    //  Serial.printf("%d (%d %d) %d,%d\n\n", i, heigth * i, maxWidth * i, (tft.height() - 20), tft.width());

    for (int z = 0; z < rows; z++) {
        memset(buf, 0, 255);
        strncpy(buf, msg + pos[z], (pos[z + 1] - pos[z]));
        tft.drawString(buf, w, (20 + h) + (z * (tft.fontHeight() + 1)));
    }
}


void ShowMessage(char SerialPrefix, const char* format, ...)
{
    char loc_buf[64];
    char* temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if (len < 0) {
        va_end(arg);
        ShowMessage("ShowMessage Error ", 'E');
        return;
    };
    if (len >= sizeof(loc_buf)) {
        temp = (char*)malloc(len + 1);
        if (temp == NULL) {
            va_end(arg);
            ShowMessage("ShowMessage Allocation Error ",'E');
            return;
        }
        len = vsnprintf(temp, len + 1, format, arg);
    }
    va_end(arg);
    ShowMessage(temp, SerialPrefix);
    if (temp != loc_buf) {
        free(temp);
    }
}

void ShowMessage(const String  msg, char SerialPrefix) {
    ShowMessage(msg.c_str(), SerialPrefix);
}

void ShowMessage(const char* msg, char SerialPrefix) {
    if (!config.TFTActive) {
        initTFT();
    }
    else {
        showVoltage(true);
    }
    if (config.TFTActive) {
        tft.setTextDatum(MC_DATUM);

        tft.setTextColor(TFT_WHITE, TFT_BLACK); // Orange
        tft.setTextFont(1);
        drawString(msg);
        tft.setTextSize(1);
    }
    char buf[255];
    strcpy(buf, msg);
    char* pch;
    pch = strchr(buf, '\n');
    while (pch != NULL) {
        pch[0] = ' ';
        pch = strchr(pch + 1, '\n');
    }
    Serial.print(SerialPrefix);
    Serial.print(' ');
    Serial.println(buf);
}


void initTFT() {

    /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
    */
    pinMode(TFT_BL, INPUT);
    delay(10);
    config.TFTActive = !digitalRead(TFT_BL);



    if (config.TFTActive) {
        ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
        ledcAttachPin(TFT_BL, pwmLedChannelTFT);
        ledcWrite(pwmLedChannelTFT, config.ledBacklight);
        tft.init();
        tft.setRotation(1);
        if (!config.wakeup) {
            tft.fillScreen(TFT_BLACK);
            tft.setSwapBytes(true);
            tft.pushImage(0, 0, 240, 135, nowTalk);
            delay(2000);
        }
    }
    showVoltage(true);
}


void GoSleep() {
    uint64_t nt = myEvents.getNextTrigger();
    if (nt > 0) {
        uint64_t time = (nt - myEvents.timeNow());
        if (time > 1000) {
            esp_sleep_enable_timer_wakeup((time-1000) * 1000);
        } else {
            if (digitalRead(GPIO_NUM_0) == 0) {
                ShowMessage("Release the button.\n Timer prevent sleep mode.", '!');
                while (digitalRead(GPIO_NUM_0) == 0) {}
            }
            return;

        }
    }

    if (digitalRead(GPIO_NUM_0) == 0) {
        ShowMessage("Release the button.", '!');
        while (digitalRead(GPIO_NUM_0) == 0) {}
        ShowMessage("Press left key to wake up", '!');
        delay(3000);
    }
    wakeup = true;
    saveConfiguration();

    if (config.TFTActive) {
        ledcWrite(TFT_BL, 0);
        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
    }
    //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
    //esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
}

#endif
/*
In file included from src\commbadge\TimeEvents.cpp:21:0:
src\commbadge\TimeEvents.h:70:26: error: typedef 'OnTick_t' is initialized (use decltype instead)
 typedef void (*OnTick_t)(AlarmID_t ID);  // alarm callback function typedef
                          ^
src\commbadge\TimeEvents.h:70:26: error: 'AlarmID_t' was not declared in this scope
src\commbadge\TimeEvents.h:75:5: error: 'OnTick_t' does not name a type
     OnTick_t onTickHandler;
     ^
src\commbadge\TimeEvents.h:121:38: error: 'OnTick_t' has not been declared
     AlarmID_t create(uint32_t value, OnTick_t onTickHandler, uint8_t isOneShot,
dtAlarmPeriod_t alarmType);
                                      ^
src\commbadge\TimeEvents.h:150:43: error: 'OnTick_t' has not been declared
     AlarmID_t triggerOnce(uint32_t value, OnTick_t onTickHandler) {
                                           ^
src\commbadge\TimeEvents.h:156:41: error: 'OnTick_t' has not been declared
     AlarmID_t alarmOnce(uint32_t value, OnTick_t onTickHandler) {
                                         ^
src\commbadge\TimeEvents.h:160:64: error: 'OnTick_t' has not been declared
     AlarmID_t alarmOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                ^
src\commbadge\TimeEvents.h:165:91: error: 'OnTick_t' has not been declared
     AlarmID_t alarmOnce(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {

          ^
src\commbadge\TimeEvents.h:172:43: error: 'OnTick_t' has not been declared
     AlarmID_t alarmRepeat(uint32_t value, OnTick_t onTickHandler) {
                                           ^
src\commbadge\TimeEvents.h:176:66: error: 'OnTick_t' has not been declared
     AlarmID_t alarmRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                  ^
src\commbadge\TimeEvents.h:181:93: error: 'OnTick_t' has not been declared
     AlarmID_t alarmRepeat(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {

            ^
src\commbadge\TimeEvents.h:188:41: error: 'OnTick_t' has not been declared
     AlarmID_t timerOnce(uint32_t value, OnTick_t onTickHandler) {
                                         ^
src\commbadge\TimeEvents.h:192:64: error: 'OnTick_t' has not been declared
     AlarmID_t timerOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                ^
src\commbadge\TimeEvents.h:197:43: error: 'OnTick_t' has not been declared
     AlarmID_t timerRepeat(uint32_t value, OnTick_t onTickHandler) {
                                           ^
src\commbadge\TimeEvents.h:202:66: error: 'OnTick_t' has not been declared
     AlarmID_t timerRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                  ^
src\commbadge\TimeEvents.cpp: In member function 'void TimeEventsClass::enable(AlarmID_t)':
src\commbadge\TimeEvents.cpp:47:112: error: 'struct alarm_t' has no member named
'onTickHandler'
         if ((!(dtUseAbsoluteValue(m_alarms[ID].Mode.alarmType) && (m_alarms[ID].value == 0))) && (m_alarms[ID].onTickHandler != NULL)) {

                               ^
src\commbadge\TimeEvents.cpp: In member function 'void TimeEventsClass::free(AlarmID_t)':
src\commbadge\TimeEvents.cpp:104:22: error: 'struct alarm_t' has no member named
'onTickHandler'
         m_alarms[ID].onTickHandler = NULL;
                      ^
src\commbadge\TimeEvents.cpp: In member function 'void TimeEventsClass::loop()':
src\commbadge\TimeEvents.cpp:160:17: error: 'OnTick_t' was not declared in this scope
                 OnTick_t TickHandler = m_alarms[servicedAlarmId].onTickHandler;
                 ^
src\commbadge\TimeEvents.cpp:167:21: error: 'TickHandler' was not declared in this scope
                 if (TickHandler != NULL) {
                     ^
src\commbadge\TimeEvents.cpp: At global scope:
src\commbadge\TimeEvents.cpp:194:51: error: 'OnTick_t' has not been declared
 AlarmID_t TimeEventsClass::create(uint32_t value, OnTick_t onTickHandler, uint8_t isOneShot, dtAlarmPeriod_t alarmType)
                                                   ^
src\commbadge\TimeEvents.cpp: In member function 'AlarmID_t TimeEventsClass::create(uint32_t, int, uint8_t, dtAlarmPeriod_t)':
src\commbadge\TimeEvents.cpp:202:30: error: 'struct alarm_t' has no member named
'onTickHandler'
                 m_alarms[id].onTickHandler = onTickHandler;
                              ^
Compiling .pio\build\CommBadge\liba9d\FS\FS.cpp.o
Compiling .pio\build\CommBadge\liba9d\FS\vfs_api.cpp.o
*** [.pio\build\CommBadge\src\commbadge\TimeEvents.cpp.o] Error 1
In file included from src\commbadge\variables.h:12:0,
                 from src\commbadge\main.cpp:20:
src\commbadge\TimeEvents.h:70:26: error: typedef 'OnTick_t' is initialized (use decltype instead)
 typedef void (*OnTick_t)(AlarmID_t ID);  // alarm callback function typedef
                          ^
src\commbadge\TimeEvents.h:70:26: error: 'AlarmID_t' was not declared in this scope
src\commbadge\TimeEvents.h:75:5: error: 'OnTick_t' does not name a type
     OnTick_t onTickHandler;
     ^
src\commbadge\TimeEvents.h:121:38: error: 'OnTick_t' has not been declared
     AlarmID_t create(uint32_t value, OnTick_t onTickHandler, uint8_t isOneShot,
dtAlarmPeriod_t alarmType);
                                      ^
src\commbadge\TimeEvents.h:150:43: error: 'OnTick_t' has not been declared
     AlarmID_t triggerOnce(uint32_t value, OnTick_t onTickHandler) {
                                           ^
src\commbadge\TimeEvents.h:156:41: error: 'OnTick_t' has not been declared
     AlarmID_t alarmOnce(uint32_t value, OnTick_t onTickHandler) {
                                         ^
src\commbadge\TimeEvents.h:160:64: error: 'OnTick_t' has not been declared
     AlarmID_t alarmOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                ^
src\commbadge\TimeEvents.h:165:91: error: 'OnTick_t' has not been declared
     AlarmID_t alarmOnce(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {

          ^
src\commbadge\TimeEvents.h:172:43: error: 'OnTick_t' has not been declared
     AlarmID_t alarmRepeat(uint32_t value, OnTick_t onTickHandler) {
                                           ^
src\commbadge\TimeEvents.h:176:66: error: 'OnTick_t' has not been declared
     AlarmID_t alarmRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                  ^
src\commbadge\TimeEvents.h:181:93: error: 'OnTick_t' has not been declared
     AlarmID_t alarmRepeat(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {

            ^
src\commbadge\TimeEvents.h:188:41: error: 'OnTick_t' has not been declared
     AlarmID_t timerOnce(uint32_t value, OnTick_t onTickHandler) {
                                         ^
src\commbadge\TimeEvents.h:192:64: error: 'OnTick_t' has not been declared
     AlarmID_t timerOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                ^
src\commbadge\TimeEvents.h:197:43: error: 'OnTick_t' has not been declared
     AlarmID_t timerRepeat(uint32_t value, OnTick_t onTickHandler) {
                                           ^
src\commbadge\TimeEvents.h:202:66: error: 'OnTick_t' has not been declared
     AlarmID_t timerRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
                                                                  ^
In file included from src\commbadge\main.cpp:20:0:
src\commbadge\variables.h: In function 'void loadConfiguration(bool)':
src\commbadge\variables.h:239:20: error: 'struct alarm_t' has no member named 'onTickHandler'
         alarms[id].onTickHandler = (OnTick_t)elem["trigger"].as<uint32_t>();
                    ^
src\commbadge\variables.h:239:37: error: 'OnTick_t' was not declared in this scope
         alarms[id].onTickHandler = (OnTick_t)elem["trigger"].as<uint32_t>();
                                     ^
src\commbadge\variables.h: In function 'void saveConfiguration()':
src\commbadge\variables.h:284:55: error: 'struct alarm_t' has no member named 'onTickHandler'
             data_0["trigger"] = (uint32_t) alarms[id].onTickHandler;
                                                       ^
src\commbadge\main.cpp: In function 'void OnPing(AlarmID_t)':
src\commbadge\main.cpp:167:59: error: invalid conversion from 'void (*)(AlarmID_t) {aka void (*)(unsigned char)}' to 'int' [-fpermissive]
    config.timeoutID = myEvents.timerOnce(150, OnNoReaction);
                                                           ^
In file included from src\commbadge\variables.h:12:0,
                 from src\commbadge\main.cpp:20:
src\commbadge\TimeEvents.h:188:15: note:   initializing argument 2 of 'AlarmID_t
TimeEventsClass::timerOnce(uint32_t, int)'
     AlarmID_t timerOnce(uint32_t value, OnTick_t onTickHandler) {
               ^
src\commbadge\main.cpp: In function 'void setup()':
src\commbadge\main.cpp:262:41: error: invalid conversion from 'void (*)(AlarmID_t) {aka void (*)(unsigned char)}' to 'int' [-fpermissive]
             myEvents.timerOnce(1, OnPing);
                                         ^
In file included from src\commbadge\variables.h:12:0,
                 from src\commbadge\main.cpp:20:
src\commbadge\TimeEvents.h:188:15: note:   initializing argument 2 of 'AlarmID_t
TimeEventsClass::timerOnce(uint32_t, int)'
     AlarmID_t timerOnce(uint32_t value, OnTick_t onTickHandler) {
               ^
src\commbadge\main.cpp:268:41: error: invalid conversion from 'void (*)(AlarmID_t) {aka void (*)(unsigned char)}' to 'int' [-fpermissive]
             myEvents.timerOnce(i, OnPing);
                                         ^
In file included from src\commbadge\variables.h:12:0,
                 from src\commbadge\main.cpp:20:
src\commbadge\TimeEvents.h:188:15: note:   initializing argument 2 of 'AlarmID_t
TimeEventsClass::timerOnce(uint32_t, int)'
     AlarmID_t timerOnce(uint32_t value, OnTick_t onTickHandler) {
               ^
src\commbadge\main.cpp:270:73: error: invalid conversion from 'void (*)(AlarmID_t) {aka void (*)(unsigned char)}' to 'int' [-fpermissive]
         config.sleepID = myEvents.timerOnce(config.timerSleep, onGoSleep);
                                                                         ^
In file included from src\commbadge\variables.h:12:0,
                 from src\commbadge\main.cpp:20:
src\commbadge\TimeEvents.h:188:15: note:   initializing argument 2 of 'AlarmID_t
TimeEventsClass::timerOnce(uint32_t, int)'
     AlarmID_t timerOnce(uint32_t value, OnTick_t onTickHandler) {
               ^
*** [.pio\build\CommBadge\src\commbadge\main.cpp.o] Error 1*/