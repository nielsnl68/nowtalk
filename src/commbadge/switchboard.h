#ifndef NOWTALK_SWITCH
#define NOWTALK_SWITCH

#include <Arduino.h>
#include "variables.h"
#include <esp_now.h>
#include "nowtalk.h"

const char *tail;

void handlePackage(const uint8_t mac[6], const uint8_t *buf, size_t count)
{
    /*
    char info[256];
    String test;
    memcpy((void *)info, (void *)&buf[1], count - 1);
    uint8_t action = buf[0];
    String split = String(info);
    char msg[255];

    debug(">", mac, action, info);

    switch (action)
    {
    case ESPTALK_SERVER_PONG:
    {
        // turn off timeout
        bool isNewMaster = (config.masterAddress[0] == 0);
        if (isNewMaster)
        {
            sprintf(msg, "* New Master at %02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            message(msg);
        }
        else
        {
            esp_now_del_peer(config.masterAddress);
        }
        add_peer(mac);
        if ((strcmp(config.masterIP, "<None") == 0) && (strcmp(config.userName, "<None>") == 0))
        {
            send_message(mac, ESPTALK_CLIENT_NEWPEER, "");
        }
    }
    break;
    case ESPTALK_SERVER_REQUEST_DETAILS:
    {
        if ((strcmp(config.masterIP, "<None") == 0) && (strcmp(config.userName, "<None>") == 0))
        {
            send_message(mac, ESPTALK_CLIENT_NEWPEER, "");
        }
        else
            send_message(mac, ESPTALK_CLIENT_DETAILS, String(config.masterIP) + " " + String(config.userName));
    }
    break;

    case ESPTALK_SERVER_NEW_IP:
    {
        test = split.substring(0, split.indexOf(" "));
        if (strcmp(config.masterIP, split.c_str()) == 0)
        {
            strlcpy(config.masterIP, split.substring(split.indexOf(" ") + 1).c_str(), sizeof(config.masterIP)); // <- destination's capacity

            send_message(mac, ESPTALK_CLIENT_ACK, "IP");
            message("* Update IP to " + String(config.masterIP));
        }
    }
    break;
    case ESPTALK_SERVER_NEW_NAME:
    {
        test = split.substring(0, split.indexOf(" "));
        if (strcmp(config.userName, split.c_str()) == 0)
        {
            strlcpy(config.userName, split.substring(split.indexOf(" ") + 1).c_str(), sizeof(config.userName)); // <- destination's capacity
            send_message(mac, 0x06, "Name");
            message("* Update Name to " + String(config.userName));
        }
    }
    break;
    default:
    {
        char msg[254];
        sprintf(msg, "E Unknown message code receiced from %02x%02x%02x%02x%02x%02x [%02x] %s",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], action, info);
        message(msg);
    
    }
    */
}

#endif