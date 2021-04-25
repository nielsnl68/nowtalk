#ifndef NOWTALK_SWITCH
#define NOWTALK_SWITCH

#include <Arduino.h>
#include "variables.h"
#include <esp_now.h>
#include "nowtalk.h"
#include <sqlite3.h>

sqlite3_stmt *res;
const char *tail;

void update_peer_status(const uint8_t mac[6], byte status);
byte find_peer(const uint8_t mac[6], char *ip, char *Name);

void handlePackage(const uint8_t mac[6], const uint8_t *buf, size_t count)
{
    char info[206];
    String test;
    memcpy((void *)info, (void *)&buf[1], count - 1);
    uint8_t action = buf[0];
    String split = String(info);
    char msg[255];

    debug(">", mac, action, info);

    switch (action)
    {
    case ESPTALK_CLIENT_PING:
    { // client ping
        if (!config.isMaster)
            return;
        if (!esp_now_is_peer_exist(mac))
        {
            if (esp_now_peer_num().total_num >= ESP_NOW_MAX_TOTAL_PEER_NUM)
            {
                return; // no space left to add a new peer.
            }
            char *ip;
            char *name;
            byte state = find_peer(mac, ip, name);
            if (state == ESPTALK_FINDPEER_NOTFOUND && config.allowGuests)
            { // peer not in database and guest allowed
                add_peer(mac);
                send_message(mac, ESPTALK_SERVER_REQUEST_DETAILS, ""); // request peer information
                return;
            }
            else if (state == ESPTALK_FINDPEER_GUEST)
            { // peer is found in database
                add_peer(mac);
                send_message(mac, ESPTALK_SERVER_REQUEST_DETAILS, "");
                return;
            }
            else if (state == ESPTALK_FINDPEER_BLOCKED)
            { // user blocked no excess allowed
                update_peer_status(mac, ESPTALK_STATUS_GONE);
                return;
            }
        }
        update_peer_status(mac, ESPTALK_STATUS_ALIVE);
        send_message(mac, ESPTALK_SERVER_PONG, "");
    }
    break;
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
        if ((strcmp(config.masterIP, "<None")==0) && (strcmp(config.userName, "<None>")==0))
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
    case ESPTALK_CLIENT_NEWPEER:
    {
        char ip[32] = "";
        char name[64] = "";
        if (find_peer(mac, ip, name) == 1)
        {
            send_message(mac, ESPTALK_SERVER_NEW_IP, "<None> " + String(ip));
            send_message(mac, ESPTALK_SERVER_NEW_NAME, "<None> " + String(name));
        }
        send_message(mac, ESPTALK_SERVER_REJECT, "");
        // turn off timeout
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

    case ESPTALK_CLIENT_ACK:
    {
        // do  noting yet
    }
    break;

    default:
    {
        char msg[254];
        sprintf(msg, "E Unknown message code receiced from %02x%02x%02x%02x%02x%02x [%02x] %s",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], action, info);
        message(msg);
    }
    }
}

bool add_userinfo(byte status, const uint8_t *mac_addr, const char *externIP, const char *username)
{
    return false;
}

void createUserList() {}

void update_peer_status(const uint8_t *mac, byte status)
{
    if (stats.first_element != 0xff)
    {

        int i = stats.first_element;
        if (memcmp((const void *)users[i].mac, (const void *)mac, 6) == 0)
        {
            // if the arrays are equal execute the code here
            // ...
        }
    }
}

void Handle_Node(const uint8_t *mac, char *status, const char *externIP)
{
    // do something later
}

static int createlist(void *data, int argc, char **argv, char **azColName)
{
    if (stats.count >= stats.maxCount)
        return -1;
    if (stats.current == 0xff)
    {
        stats.current = 0;
    }
    else
    {
        stats.current = stats.current + 1;
    }
    if (stats.current >= stats.maxCount || users[stats.current].status != 0)
    {
        bool found = false;
        for (int i = 0; i <= stats.maxCount; i++)
        {
            if (users[i].status == 0x00)
            {
                stats.current = i;
                found = true;
                break;
            }
        }
        if (!found)
            return -1;
    }
    users[stats.current].status = int(argv[0]);
    hexCharacterStringToBytes(users[stats.current].mac, argv[1]);
    sprintf(users[stats.current].name, "%s", argv[2]);
    sprintf(users[stats.current].orginIP, "%s", argv[3]);
    users[stats.current].next = 0xff;
    // users[stats.current].prev = stats.previous;
    if (stats.previous != 0xff)
    {
        users[stats.previous].next = stats.current;
    }
    else
    {
        stats.first_element = stats.current;
    }
    stats.previous = stats.current;
    stats.last_element = stats.current;

    return 0;
}

byte find_peer(const uint8_t mac[6], char *ip, char *name)
{
    ip = "";
    name = "";
}

void setupDatabase()
{
    int rc;
    sqlite3 *db1;

    memset(users, '\0', sizeof(users));
    sqlite3_initialize();

    if (sqlite3_open("/spiffs/users.db", &db1))
        return;

    rc = sqlite3_exec(db1, "CREATE TABLE IF NOT EXISTS users (  status INTEGER,  mac TEXT NOT NULL UNIQUE,  externIP TEXT,   username TEXT NOT NULL, PRIMARY KEY(mac)", 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db1);
        db1 = 0;
        return;
    }
    rc = sqlite3_exec(db1, "INSERT INTO test1 VALUES (3, 'Hello, World from test1');", 0, 0, 0);

    rc = sqlite3_exec(db1, "LIST * FROM users WHERE status =  " + 0xf0, createlist, 0, 0);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db1);
        db1 = 0;
        return;
    }
    sqlite3_close(db1);
}

#endif