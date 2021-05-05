#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define VERSION 2.10


uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t channel = 0;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    if (sendStatus != ESP_NOW_SEND_SUCCESS)
    {
        Serial.write(0x02);
        Serial.write(mac, 6);
        Serial.write(0x01);
        Serial.write(0x77);
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *buf, int count){
    if (count >250)  count = 250;
    Serial.write(0x02);
    Serial.write(mac, 6);
    Serial.write(count);
    Serial.write(buf, count);
}

void add_peer(const uint8_t *mac)
{
    // Register peer
    if (!esp_now_is_peer_exist(mac))
    {
        esp_now_peer_info_t peerInfo = {};
        memcpy(&peerInfo.peer_addr, mac, 6);
        peerInfo.channel = channel;
        // Add peer
        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            Serial.write(0x04);
            Serial.write(mac, 6);
            Serial.write(0x00);
            return;
        }
    }
}

void   setup()
{
    Serial.begin(115200);
    Serial.print(F("NowTalk Server Version:"));
    Serial.println(VERSION, 3);
    Serial.println("Booting");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.print("MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.print(F("WiFi Channel: "));
    channel = WiFi.channel();
    Serial.println(channel);

    // Init ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    add_peer(broadcastAddress);
    
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Running");
}

unsigned long previousMillis = millis();
void loop() {
  if(millis() - previousMillis > 100000){
      byte mac[6] = {0x21,0x31,0x41,0x51,0x11,0x61};
      Serial.write(0x02); Serial.write(mac,6); Serial.write(0x01);Serial.write(0x77);
      previousMillis =millis();
  }
}

  unsigned long interval = 1000;

void serialEvent() {
  unsigned long previousMillis = millis();
  while (Serial.available()) {
    // get the new byte:
    byte inChar = Serial.read();
    if (inChar == 0x02) {
      byte mac[6];
      byte len;
      previousMillis = millis();
      while (Serial.available()<7) {
        if(millis() - previousMillis > interval){
          return;
        }
      }
      Serial.readBytes(mac,6);
      len = Serial.read();
      byte data [200];
      previousMillis = millis();
      while (Serial.available()<len) {
        if(millis() - previousMillis > interval){
          return;
        }
      }
      Serial.readBytes(data,len);
      add_peer(mac);
      esp_now_send(mac, data, len);
      add_peer(mac);
    }else if (inChar == 0x03) {
      byte mac[6];
      previousMillis = millis();
      while (Serial.available()<6) {
        if(millis() - previousMillis > interval){
          return;
        }
      }
      Serial.readBytes(mac,6);
      esp_now_del_peer(mac);
    }
  }
}
