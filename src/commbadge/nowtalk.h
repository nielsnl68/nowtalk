
#ifndef NOWTALK_COMMON
#define NOWTALK_COMMON
#include "esp_now.h"
#include "variables.h"

#include "display.h"

void add_peer(const uint8_t* mac) {
    // Register peer
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(&peerInfo.peer_addr, mac, 6);
        peerInfo.channel = config.channel;
        // Add peer        
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            ShowMessage("Failed to add peer", '!');
            return;
        }
    }
}

void serial_mac(const uint8_t* mac_addr) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x ",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
}


byte nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;  // Not a valid hexadecimal character
}

void hexCharacterStringToBytes(byte* byteArray, const char* hexString)
{
    bool oddLength = strlen(hexString) & 1;

    byte currentByte = 0;
    byte byteIndex = 0;

    for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
    {
        bool oddCharIndex = charIndex & 1;

        if (oddLength)
        {
            // If the length is odd
            if (oddCharIndex)
            {
                // odd characters go in high nibble
                currentByte = nibble(hexString[charIndex]) << 4;
            }
            else
            {
                // Even characters go into low nibble
                currentByte |= nibble(hexString[charIndex]);
                byteArray[byteIndex++] = currentByte;
                currentByte = 0;
            }
        }
        else
        {
            // If the length is even
            if (!oddCharIndex)
            {
                // Odd characters go into the high nibble
                currentByte = nibble(hexString[charIndex]) << 4;
            }
            else
            {
                // Odd characters go into low nibble
                currentByte |= nibble(hexString[charIndex]);
                byteArray[byteIndex++] = currentByte;
                currentByte = 0;
            }
        }
    }
}

void debug(char code, const uint8_t* mac, uint8_t action, const char* info)
{
    char msg[256];
    snprintf(msg, 255, "%c %02x%02x%02x%02x%02x%02x %02x %s",
        code, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], action, info);
    Serial.println(msg);
}

void ShowByteArray(String test) {
    byte buffer[test.length()];
    test.getBytes(buffer, test.length());
    for (int i = 0; i < test.length(); i++) Serial.print(buffer[i], HEX);
    Serial.println();
}

esp_err_t  send_message(const uint8_t* mac, byte action, String data) {
    char  myData[200];
    byte n = sprintf(myData, "%c%s", (char)action, data.c_str());
    myData[n] = 0;
    debug('<', mac, action, data.c_str());
    esp_err_t  result = esp_now_send(mac, (const uint8_t*)&myData, n);
    return result;
}

esp_err_t broadcast(byte action, const String& message)
{
    uint8_t broadcastAddress[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    add_peer(broadcastAddress);
    esp_err_t result = send_message(broadcastAddress, action, message);
    esp_now_del_peer(broadcastAddress);
    return result;
}

String IpAddress2String(const IPAddress& ipAddress)
{
    return String(ipAddress[0]) + String(".") + \
        String(ipAddress[1]) + String(".") + \
        String(ipAddress[2]) + String(".") + \
        String(ipAddress[3]);
}

void OnPing(AlarmID_t ID);

void Reboot() {
    if (config.TFTActive) {
        ledcWrite(TFT_BL, 0);
        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
    }
    ESP.restart();
}

void ClearBadge() {
    ShowMessage("Okey,\n Clearing badge and\nreboot.", '!');
    delay(3000);
    loadConfiguration(true);
    Reboot();
}

void checkReaction() {
    if (config.heartbeat > 3) {
        esp_now_del_peer(currentSwitchboard);
        currentSwitchboard[0] = 0x00;
    }
    uint32_t i = config.timerPing * ((currentSwitchboard[0] == 0) ? 5 : 1);
    //Serial.printf("checkReaction %d %ld \n", i, config.timerPing);
     myEvents.timerOnce(i, OnPing);
}

void OnClearBadge(Button2& b) {
    ClearBadge();
}

void OnRegisterBadge(Button2& b)
{
    if (config.registrationMode)
    {
        broadcast(NOWTALK_CLIENT_NEWPEER, "");
        // config.registrationMode = true;
        ShowMessage("Badge code:\n" + badgeID());
    }
    else if (currentSwitchboard[0] != 0)
    {
        send_message(currentSwitchboard, NOWTALK_CLIENT_START_CALL, "");
    }
    else
    {
        broadcast(NOWTALK_CLIENT_PING, "");
    }
}
void OnLongPress(Button2& btn) {
    GoSleep();
}

void OnNoReaction(AlarmID_t ID) {
    checkReaction();
    if (config.wakeup) GoSleep();
}

void OnGoSleep(AlarmID_t ID) {
    GoSleep();
}

void OnPing(AlarmID_t ID) {
    if (currentSwitchboard[0] == 0)
    {
        Serial.println("*  Search SwitchBoard");
        broadcast(0x01, "");
    }
    else
    {
        send_message(currentSwitchboard, 0x01, "");
    }
    config.timeoutID = myEvents.timerOnce(500, OnNoReaction);
}


#endif
