#ifndef NOWTALK_SWITCH
#define NOWTALK_SWITCH

#include <Arduino.h>
#include "variables.h"
//#include <esp_now.h>
#include "nowtalk.h"
#include "display.h"
//#include "time.h"

// #include "CRC.h"
tm mytm;

tm createTM(const char* str)
{
    tm mytm;
    int Year, Month, Day, Hour, Minute, Second;
    sscanf(str, "%d-%d-%dT%d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);
    mytm.tm_year = Year;
    mytm.tm_mon = Month;
    mytm.tm_mday = Day;
    mytm.tm_hour = Hour;
    mytm.tm_min = Minute;
    mytm.tm_sec = Second;
    return mytm;
}

void handlePackage(const uint8_t mac[6], const uint8_t action, const char* info, size_t count)
{
    String test;
    String split = String(info);
   // char msg[255];

    debug('>', mac, action, info);

    switch (action)
    {
    case NOWTALK_CLIENT_PING:
    {
        // do noting. this is a call for a server. this is a client.
    }
    break;
    case NOWTALK_SERVER_PONG:
    {
        // turn off timeout
        bool isNewMaster = (currentSwitchboard[0] == 0);
        if (isNewMaster)
        {
            memcpy(currentSwitchboard, mac, 6);
            if (config.masterSwitchboard[0] == 0)
            {
                memcpy(config.masterSwitchboard, mac, 6);
                saveConfiguration();
            }
            add_peer(mac);

        }
        if (split.length() > 0) {
            String name = getValue(split, '~', 0);
            strlcpy(config.switchboard, name.c_str(), sizeof(config.switchboard)); // <- destination's capacity

            String date = getValue(split, '~', 1);
            if (!config.wakeup) {
                ShowMessage('*', "New switchboard:\n\n%s", config.switchboard);
            }
        }
        /*
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        tm xtm = createTM(date.c_str());
        setTime(makeTime(xtm))
        */

      //  Serial.println(config.timerPing);
        myEvents.free(config.timeoutID); config.timeoutID = -1;
        myEvents.timerOnce(config.timerPing, OnPing);
        config.heartbeat = 0;

        if (config.wakeup) {
            GoSleep();
        }


    }
    break;

    case NOWTALK_SERVER_REQUEST_DETAILS:
    {
        add_peer(mac);
        if ((strcmp(config.masterIP, "<None>") != 0) && (strcmp(config.userName, "<None>") != 0)) {
            send_message(mac, NOWTALK_CLIENT_DETAILS, String(config.masterIP) + "~" + String(config.userName)+"~"+badgeID());
        }  else {
            send_message(mac, NOWTALK_CLIENT_NACK, "");
            
        }
        esp_now_del_peer(mac);
    }
    break;

    case NOWTALK_SERVER_NEW_IP:
    {
        String OldIP = getValue(split, '~', 0);
        String NewIP = getValue(split, '~', 1);
        if (strcmp(config.masterIP, OldIP.c_str()) == 0)
        {
            strlcpy(config.masterIP, NewIP.c_str(), sizeof(config.masterIP)); // <- destination's capacity

            send_message(mac, NOWTALK_CLIENT_ACK, "");
            ShowMessage( '*',  "Update IP to: \n%s", config.masterIP);
        }
        else {
            send_message(mac, NOWTALK_CLIENT_NACK, "");
        }
    }
    break;
    case NOWTALK_SERVER_NEW_NAME:
    {
        String OldName = getValue(split, '~', 0);
        String NewName = getValue(split, '~', 1);
        if (strcmp(config.userName, OldName.c_str()) == 0){
            strlcpy(config.userName, NewName.c_str(), sizeof(config.userName)); // <- destination's capacity
            send_message(mac, NOWTALK_CLIENT_ACK, "");
            ShowMessage( '*', "Update Name to: \n%s", config.userName );
        } else {
            send_message(mac, NOWTALK_CLIENT_NACK, "");
        }
    }
    break;
    default:
        ShowMessage('!', "Unknown msg from: \n%02x%02x%02x%02x%02x%02x\n [%02x] %s",
                                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], action, info);
    }
}

#endif