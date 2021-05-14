
#ifndef NOWTALK_COMMON
#define NOWTALK_COMMON

#include "variables.h"
#include <HTTPClient.h>

void message(const char * message){
    Serial.println(message);
}

void message(const String msg) {
  message(msg.c_str());
}

void add_peer(const uint8_t * mac) {
  // Register peer
  if (!esp_now_is_peer_exist(mac)) {    
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, mac, 6);
    peerInfo.channel = config.channel;
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      message("* Failed to add peer");
      return;
    }
  }
}

void serial_mac(const uint8_t *mac_addr) {
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

void hexCharacterStringToBytes(byte *byteArray, const char *hexString)
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

void debug(char code, const uint8_t *mac, uint8_t action, const char *info)
{
    char msg[256];
    snprintf(msg, 255, "%c %02x%02x%02x%02x%02x%02x %02x %s",
             code, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], action, info);
    message(msg);
}

void ShowByteArray(String test) {
  byte buffer[test.length()] ;
  test.getBytes(buffer,test.length());
  for (int i = 0; i < test.length(); i++) Serial.print(buffer[i], HEX);
  Serial.println();
}

esp_err_t  send_message(const uint8_t * mac, byte action, String data) {
  char  myData[200];
  byte n = sprintf(myData, "%c%s", (char) action,  data.c_str());
  myData[n] = 0;
  debug('<', mac, action, data.c_str());
  esp_err_t  result =  esp_now_send(mac, (const uint8_t *) &myData, n);
  return result;
}

esp_err_t broadcast(byte action, const String &message)
{
  uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  add_peer(broadcastAddress);
  esp_err_t result = send_message(broadcastAddress, action , message);
  esp_now_del_peer(broadcastAddress);
  return result;
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

String GetExternalIP()
{
  String ip = "";

  HTTPClient http;
  http.begin("http://api.ipify.org/?format=text");
  int statusCode = http.GET();
  if (statusCode == 200)
  {
    ip = http.getString();
  }
  http.end();
  return ip;
}

String badgeID()
{
    static char baseChars[] = "0123456789AbCdEfGhIjKlMnOpQrStUvWxYz"; //aBcDeFgHiJkLmNoPqRsTuVwXyZ
    uint8_t base = sizeof(baseChars);
    String result = "";
    uint32_t chipId = 0xa5000000;
    uint8_t crc = 0;

    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    do {
        result = String(baseChars[chipId % base]) + result; // Add on the left
        crc += chipId % base;
        chipId /= base;
    } while (chipId != 0);
    return result + String(baseChars[crc % base]);
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

#endif
