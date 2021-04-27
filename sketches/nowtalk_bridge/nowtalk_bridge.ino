#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <espnow.h>
  
#ifndef STASSID
#define STASSID "FusionNet"
#define STAPSK  "Lum3nS0fT"
#endif

#define VERSION 2.00

const char* ssid = STASSID;
const char* password = STAPSK;
int channel = 0;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void OnDataSent(uint8_t *mac, uint8_t sendStatus) {
  if (sendStatus != 0){
    Serial.write(0x02); Serial.write(mac,6); Serial.write(0x01);Serial.write(0x77);
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
//  char data [200];
  
    Serial.write(0x02);
    Serial.write(mac,6);
    Serial.write(0x00);
   // Serial.write(incomingData,1);
}


void add_peer(uint8_t * mac) {
  if (!esp_now_is_peer_exist(mac)){
    esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, channel, NULL, 0);
  } 
}

void setup() {
  Serial.begin(115200);
  Serial.print(F("NowTalk Server Version:"));
  Serial.println(VERSION,3);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("nowTalkServer");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %3u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.print(F("WiFi Channel: "));
  channel = WiFi.channel();
  Serial.println( channel);
 
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
  
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Running");
 
}
unsigned long previousMillis = millis();
void loop() {
  ArduinoOTA.handle();
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
