#ifndef NOWTALK_SWITCH
#define NOWTALK_SWITCH

#include <Arduino.h>
#include "variables.h"
#include <esp_now.h>
#include "nowtalk.h"
#include "display.h"


// #include "CRC.h"
 

const char *tail;

void handlePackage(const uint8_t mac[6], const uint8_t action, const char *info, size_t count)
{
    String test;
    String split = String(info);
    char msg[255];

    debug('>', mac,  action,  info);

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
        bool isNewMaster = (config.masterAddress[0] == 0);
        if (isNewMaster)
        {
            sprintf(msg, "Switchboard at %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            ShowMessage(msg,'*');
        }
        else
        {
            esp_now_del_peer(config.masterAddress);
        }
        memcpy(config.masterAddress, mac, 6);
        add_peer(mac);
    }
    break;

    case NOWTALK_SERVER_REQUEST_DETAILS:
    {
        add_peer(mac);
        if ((strcmp(config.masterIP, "<None>") != 0) && (strcmp(config.userName, "<None>") != 0)){
            send_message(mac, NOWTALK_CLIENT_DETAILS, String(config.masterIP) +"~" + String(config.userName));
        }
        esp_now_del_peer(mac);
    }
    break;

    case NOWTALK_SERVER_ACCEPT:
    {
        add_peer(mac);
        if (config.registrationMode)
        {
            String checkID = getValue(split,'~',0);
            String IP = getValue(split, '~', 1);
            String name = getValue(split, '~', 2);
            String network = getValue(split, '~', 3);
     //       uint16_t crc = getValue(split, '~', 4).toInt();
            test = "nowTalkSrv!" + checkID + "~" + IP + "~" + name + "~" + network;
            //            uint16_t checkCrc crc16((uint8_t *)test.c_str(), test.length, 0x1021, 0, 0, false, false);
            if (checkID == badgeID())
            {                                                                  //crc == checkCrc &&
                strlcpy(config.masterIP, IP.c_str(), sizeof(config.masterIP)); // <- destination's capacity
                strlcpy(config.userName,name.c_str(), sizeof(config.userName)); // <- destination's capacity
                strlcpy(config.hostname, network.c_str(), sizeof(config.hostname)); // <- destination's capacity
                memcpy(config.masterAddress, mac, 6);
                saveConfiguration();
                send_message(mac, NOWTALK_CLIENT_ACK, "");
                config.registrationMode = false;
                return;
            }else {
                send_message(mac, NOWTALK_CLIENT_NACK, "");
                ShowMessage("Wrong Badge Id!\nPress Enter Button.",'!');
            }
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
            sprintf(msg, "Update IP to: \n%s", config.masterIP);
            ShowMessage(msg, '*');
        } else {
            send_message(mac, NOWTALK_CLIENT_NACK, "");
        }
    }
    break;
    case NOWTALK_SERVER_NEW_NAME:
    {
        String OldName = getValue(split, '~', 0);
        String NewName = getValue(split, '~', 1);
        if (strcmp(config.userName, OldName.c_str()) == 0)
        {
            strlcpy(config.userName, NewName.c_str(), sizeof(config.userName)); // <- destination's capacity
            send_message(mac, NOWTALK_CLIENT_ACK, "");
            sprintf(msg, "Update Name to: \n%s", config.userName);
            ShowMessage(msg,'*');
        }  else {
            send_message(mac, NOWTALK_CLIENT_NACK, "");
        }
    }
    break;
    default:
        sprintf(msg, "Unknown msg from: \n%02x%02x%02x%02x%02x%02x\n [%02x] %s",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], action, info);
        ShowMessage(msg,'!');
    }
}

#endif