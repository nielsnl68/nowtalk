#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
//#include "Version.h"

#ifndef VERSION
#define VERSION "2.26.221"
#endif

uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t channel = 0;
bool isActive = false;

String badgeID() {
  static char baseChars[] = "0123456789AbCdEfGhIjKlMnOpQrStUvWxYz";  //aBcDeFgHiJkLmNoPqRsTuVwXyZ
  uint8_t base = sizeof(baseChars);
  String result = "";
  uint32_t chipId = 0xa5000000;
  uint8_t crc = 0;

  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  do {
    result = String(baseChars[chipId % base]) + result;  // Add on the left
    crc += chipId % base;
    chipId /= base;
  } while (chipId != 0);
  return result + String(baseChars[crc % base]);
}

void OnDataSent(const uint8_t* mac, esp_now_send_status_t sendStatus) {
  if (mac[0] == 0) {
    Serial.println("E Data package was send with unknown Mac address.");
  } else {
    Serial.write(0x02);
    Serial.write(mac, 6);
    Serial.write(0x01);
    Serial.write(0x77);
    if (sendStatus != ESP_NOW_SEND_SUCCESS) {
      Serial.write(0x77);
    } else {
      Serial.write(0x88);
    }
  }
}

void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
  if (!isActive) return;
  bool broadcast = memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, ESP_NOW_ETH_ALEN) == 0;

  uint8_t * mac = recv_info->src_addr;
  uint8_t * origin = recv_info->src_addr;
  wifi_pkt_rx_ctrl_t *rx_ctrl = recv_info->rx_ctrl;
  uint8_t rssi = rx_ctrl->rssi;


  Serial.write(0x02);
  Serial.write(mac, 6);
  Serial.write(count+1);
  Serial.write((broadcast?0x80:0x00) + rssi>>1);
  Serial.write(buf, count);

}

void add_peer(const uint8_t* mac) {
  // Register peer
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, mac, 6);
    peerInfo.channel = channel;
    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.write(0x04);
      Serial.write(mac, 6);
      Serial.write(0x00);
      return;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.print(F("* NowTalk Bridge Version:"));
  Serial.println(VERSION);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print(F("* MAC Address: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("* WiFi Channel: "));
  channel = WiFi.channel();
  Serial.println(channel);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println(F("* Error initializing ESP-NOW"));
    return;
  }
  add_peer(broadcastAddress);

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println(F("* Running"));
}

unsigned long interval = 1000;
void performUpdate(Stream& updateSource, size_t updateSize);

void serialEvent() {
  unsigned long previousMillis = millis();
  while (Serial.available()) {
    // get the new byte:
    byte inChar = Serial.read();
    if (inChar == '*') {
      String x = Serial.readStringUntil((char)0x10);
      Serial.write('#');
      Serial.write(x.c_str());
      Serial.write('~');
      Serial.print(VERSION);
      Serial.write('~');
      Serial.print(WiFi.macAddress());
      Serial.write('~');
      Serial.print(WiFi.channel());
      Serial.write('~');
      Serial.print(badgeID());
    } else if (inChar == 0x02) {
      byte mac[6];
      byte len;
      previousMillis = millis();
      while (Serial.available() < 7) {
        if (millis() - previousMillis > interval) {
          return;
        }
      }
      Serial.readBytes(mac, 6);
      len = Serial.read();
      byte data[300];
      previousMillis = millis();
      while (Serial.available() < len) {
        if (millis() - previousMillis > interval) {
          return;
        }
      }
      Serial.readBytes(data, len);
      add_peer(mac);
      esp_now_send(mac, data, len);
    } else if (inChar == 0x03) {
      byte mac[6];
      previousMillis = millis();
      while (Serial.available() < 6) {
        if (millis() - previousMillis > interval) {
          return;
        }
      }
      Serial.readBytes(mac, 6);
      esp_now_del_peer(mac);
    } else if (inChar == 0x05) {
      esp_reset();
    }
  }
}


void rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}

void loop() {
  serialEvent();
}