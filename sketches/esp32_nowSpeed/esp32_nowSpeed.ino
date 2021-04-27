/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp8266-nodemcu-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
#define VERSION 32.111


#include <esp_now.h>
#include <WiFi.h>

typedef struct {
  unsigned long msgid ;
  char msg[145] = "qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321qwertyuioplkjhgfdsazxcvbnm0987654321";
} espnowsend_t;


typedef struct {
    uint8_t mac[6];
    espnowsend_t msg;
    size_t size;
} espnow_t;

#define QUEUE_SIZE  512

static volatile int read_idx = 0;
static volatile int write_idx = 0;
static volatile unsigned long msgid = 1;
static volatile unsigned long receivedMessages = 0;
static volatile boolean hasSend = false;
static espnow_t circbuf[QUEUE_SIZE];

uint8_t bcmac[6] =   {0xff, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

unsigned long lastTime = 0;  
unsigned long timerDelay = 10000;  // send readings timer

void serial_mac(const uint8_t *mac_addr) {
  char macStr[64];
  snprintf(macStr, sizeof(macStr), "New mac: %02x:%02x:%02x:%02x:%02x:%02x",
  mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
}



espnowsend_t msg;

esp_err_t send_message() {
  msg.msgid = msgid ;  
  msgid = msgid +1 ;
  esp_err_t  result = esp_now_send(bcmac, (const uint8_t *) &msg, sizeof(msg));
  return result;
}

word numberOfErrors = 0;

// Callback when data is sent
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus) {
  if (sendStatus != ESP_NOW_SEND_SUCCESS ){
    numberOfErrors = numberOfErrors +1;
  }
  hasSend = true;
  
}

 // Callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *buf, int size) {
    // copy to circular buffer
    espnow_t data;
    memcpy(data.mac, mac, 6);
    memcpy(&data.msg.msgid, buf, sizeof(data.msg.msgid));
    data.size = size;
    circbuf[write_idx] = data;
    receivedMessages =receivedMessages +1;
    write_idx = (write_idx + 1) % QUEUE_SIZE;
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
}

word missingPackages = 0;
word oldpackages = 0;

unsigned long lastMsgId =0;
unsigned long lastsendMsgId =0;
unsigned long oldReceived =0;

void loop() {
   if (read_idx != write_idx) {
       // Serial.print(read_idx, HEX); Serial.println(write_idx,HEX);
        espnowsend_t espnow = circbuf[read_idx].msg;
        if (lastMsgId == 0) lastMsgId = espnow.msgid - 1;
        int x = (espnow.msgid - lastMsgId)-1; 
        if (x >= 1)  { missingPackages= missingPackages+x;}
        if (x <=-1)  { oldpackages= oldpackages+1; }
        else { lastMsgId= espnow.msgid;}
        
        read_idx = (read_idx + 1) % QUEUE_SIZE;
    }
  if (hasSend) {
    send_message();
    hasSend = false;
  }
  if ((millis() - lastTime) > timerDelay) {
      unsigned long y = msgid ;
      unsigned long z = receivedMessages;
      Serial.print("Send: ");
      Serial.print(y - lastsendMsgId);
      Serial.print(", Recv: ");
      Serial.print(z - oldReceived);
      Serial.print(", Missed: ");
      Serial.print(missingPackages,3);
      Serial.print(", Old: ");
      Serial.print(oldpackages,3);   
      Serial.print(", Errors: ");
      Serial.print(numberOfErrors,3);   
      Serial.print(", queued: ");
      int x = read_idx - write_idx;
      if (x<0) x= x+QUEUE_SIZE;
//         Serial.print(read_idx, HEX);Serial.print(" "); Serial.print(write_idx,HEX); Serial.print(" ");
      Serial.println(x);
      
      lastsendMsgId = y;
      oldReceived = z;

      missingPackages =0;
      oldpackages=0;   
      numberOfErrors=0;   


         
      lastTime = millis();
  }
}
