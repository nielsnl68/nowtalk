
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

  esp_err_t  result =  esp_now_send(mac, (const uint8_t *) &myData, n+2);
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

 void debug( char * code, const uint8_t *mac, uint8_t action, char *info){
    char msg[256];
    snprintf(msg, 255, "%s %02x%02x%02x%02x%02x%02x 0x%02x %s", 
             code, mac[0], mac[1], mac[2],   mac[3], mac[4], mac[5],  action, info);
    message(msg);
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

#endif
