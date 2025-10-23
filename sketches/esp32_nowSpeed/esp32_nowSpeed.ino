/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp8266-nodemcu-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
#define VERSION 32.125


#include <esp_now.h>
#include <WiFi.h>
#include "display.h"


typedef struct {
  char header[11] = "SpeedTest_";
  unsigned long msgid ;
  char msg[290] = "_wertyuiolkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321";
} espnowsend_t;


typedef struct {
    uint8_t mac[6];
    espnowsend_t msg;
    uint8_t size;
} espnow_t;

#define QUEUE_SIZE  10

static volatile int read_idx = 0;
static volatile int write_idx = 0;
static volatile unsigned long msgid = 1;
static volatile unsigned long receivedMessages = 0;
static volatile boolean hasSend = false;
static espnow_t circbuf[QUEUE_SIZE];

uint8_t bcmac[6] =   {0xff, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

unsigned long lastTime = 0;  
unsigned long timerDelay = 1000;  // send readings timer

void serial_mac(const uint8_t *mac_addr) {
  char macStr[64];
  snprintf(macStr, sizeof(macStr), "New mac: %02x:%02x:%02x:%02x:%02x:%02x",
  mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
}



espnowsend_t msg;

esp_err_t send_message() {
  msg.msgid = msgid ;  
  msgid++ ;
  uint8_t size = sizeof(msg)>250?250:sizeof(msg);
  esp_err_t  result = esp_now_send(bcmac, (const uint8_t *) &msg, size);
  return result;
}

word numberOfErrors = 0;

// Callback when data is sent
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus) {
  if (sendStatus != ESP_NOW_SEND_SUCCESS ){
    numberOfErrors = numberOfErrors +1;
  }
  hasSend = true;

  send_message(); 
  
}


word missingPackages = 0;
word oldpackages = 0;

unsigned long lastMsgId =0;
unsigned long lastsendMsgId =0;
unsigned long oldReceived =0;
unsigned long ReceiveTimer = 0;
 // Callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *buf, int size) {
    // copy to circular buffer
    ReceiveTimer = millis();
    espnow_t espnow;
    memcpy(espnow.mac, mac, 6);
    if (memcmp(&espnow.msg.header,buf,sizeof(espnow.msg.header)) == 0) {
      memcpy(&espnow.msg, buf, 250);
        if (lastMsgId == 0) lastMsgId = espnow.msg.msgid - 1;
        int x = (espnow.msg.msgid - lastMsgId)-1; 
        if (x !=0) {
          // Serial.printf("%6d-%6d = %d   %d\n",  lastMsgId, espnow.msg.msgid, x, missingPackages );
       }
        if (x > 0)  { missingPackages += x;}
        if (x < 0)  { oldpackages= oldpackages+1;}
        else { lastMsgId= espnow.msg.msgid;} 
        if (bcmac[0] == 0xff) {
          memcpy(bcmac, mac, 6);
          add_peer(bcmac);
        }
        receivedMessages =receivedMessages +1;
    } else {
      Serial.print("receive error");
    }
    
  //  write_idx = (write_idx + 1) % QUEUE_SIZE;
}


void add_peer(const uint8_t * mac) {
  // Register peer
  if (!esp_now_is_peer_exist(mac)) {    
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("* Failed to add peer");
      return;
    } else
      serial_mac(mac);
  }
}


void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  initTFT();
  
  if (esp_now_init() != 0) {
    Serial.println("* Error initializing ESP-NOW");
    return;
  }
  
  Serial.println();
  Serial.print(F("* Version: "));
  Serial.println(VERSION,3);
  Serial.print  (F("* MAC Address: "));
  Serial.println(WiFi.macAddress());

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  add_peer(bcmac);
  send_message();
  ShowMessage("Waiting");
}


void loop() {
   showVoltage();
   if ((ReceiveTimer != 0 )&& ((millis() - ReceiveTimer) > 10000)) {
    ReceiveTimer = 0 ;
    ShowMessage("No Data");
    lastMsgId = 0;
   }
 
   if (((millis() - lastTime) > timerDelay) && (ReceiveTimer != 0)) {
      unsigned long y = msgid ;
      unsigned long z = receivedMessages;
     // Serial.printf("Send: %6d, Recv: %6d, Missed: %3d, Old: %3d, Errors: %3d\n", 
     //                 y - lastsendMsgId, z - oldReceived,
     //                 missingPackages, oldpackages, numberOfErrors);   
      showData(y - lastsendMsgId, z - oldReceived, missingPackages, oldpackages, numberOfErrors);
      lastsendMsgId = y ;
      oldReceived   = z;

      missingPackages =0;
      oldpackages     =0;   
      numberOfErrors  =0;   


         
      lastTime = millis();
  }
}
