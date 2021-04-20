/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp8266-nodemcu-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
#define VERSION 32.120
/*
Request message codes:
  0x01 = Master notification
  0x02 = Ping Pong Master returns with 0x01
  0x04 = request owner IP
  0x05 = update owner IP
  0x06 = Ack update owner IP
  

  0x10 = Request call
  0x11 = Receive call
  0x12 = close call
  0x14 = reconnect to
  
  0x20 = voice package send / received.
  
  0xe0 = start update firmware
  0xe7 = upload update firmware
  0xee = Execute update now
  
  0xff = Alarm help needed.

 */


#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "ESPAsyncWebServer.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <EEPROM.h>
#include <Update.h>
#include "FS.h"
#include "SPIFFS.h"
#include "variabes.h"
#include "nowtalk.h"
#include "talkwebsite.h"
#include "firmwareUpd.h"


// Callback when data is sent
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus) {
  if (sendStatus != ESP_NOW_SEND_SUCCESS ){
    if (isMaster) {
      Handle_Node(mac, "Gone","");
    } else {
      message( ("* search new master"));
      bool isNewMaster = (masterAddress[0] == 0);           
      if (!isNewMaster) esp_now_del_peer(masterAddress);
      masterAddress[0] = 0;
      broadcast( 0x02, MasterIP);  
    }
    
  }
}

 // Callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *buf, int count) {
    // copy to circular buffer
    nowtalk_t *data = &circbuf[write_idx];
    memcpy(data->mac, mac, 6);
    memcpy(data->buf, buf, count);
    data->count = count;

    write_idx = (write_idx + 1) % QUEUE_SIZE;
}

void handlePackage(const uint8_t mac[6], const uint8_t * buf, size_t count) {
  struct_message myData;
  char line[256];
  memcpy(line, buf, count);
  line[count] = 0;

  myData.action =0;
  myData.info[0] = 0;
  memcpy(&myData, line, count);
  char msg[256];
  int n = snprintf(msg, 255, "< %02x%02x%02x%02x%02x%02x %02x %s", mac[0], mac[1], mac[2], 
                                                                  mac[3], mac[4], mac[5],
                                                                  myData.action, myData.info);
  message(msg);

  if (myData.action == 0x01) {
     bool isNewMaster = (masterAddress[0] == 0);
     if (isNewMaster) {
        int n = snprintf(msg, 255, "* New Master at %02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], 
                                                                  mac[3], mac[4], mac[5]);
        message(msg);
     } else {
       esp_now_del_peer(masterAddress);
     }
     
     add_peer(mac);
     MasterIP.trim();
     if (isNewMaster && MasterIP=="<none>")  {
        send_message(mac,0x04,"");
     }
     memcpy(&masterAddress, mac, 6);
     isMaster = false;
     EEPROM.writeBool(EEPROMaddr, isMaster);
     EEPROM.commit();
  } else
  if (myData.action == 0x02) {
    if  (!isMaster) return;
    add_peer(mac);
    Handle_Node(mac, "Alive", myData.info);
    send_message(mac, 0x01, "");
  } else 
  if (myData.action == 0x04 ) {
    if  (!isMaster) return;
    send_message(mac, 0x05, String(myData.info)+" "+MasterIP);
  } else 
  if (myData.action == 0x05) {
     String info = String(myData.info);
     String test = info.substring(0,info.indexOf(" "));test.trim();
     if (test=="") test = "<none>";
     MasterIP.trim() ;
     
    if ( MasterIP== test) {
      MasterIP = info.substring(info.indexOf(" ")+1);
      MasterIP.trim() ;
      if (MasterIP=="") MasterIP = "<none>";
      send_message(mac, 0x06, MasterIP);  
      message( "* Update IP to "+ MasterIP);
      EEPROM.writeString(EEPROMaddr+10, MasterIP);
      EEPROM.commit(); 
    
    }
  } else 
  if (myData.action == 0x06) {
    if (!isMaster) return;
    Handle_Node(mac,"Ack","");
  } else {
     int n = snprintf( msg, 255, "E Unknown message code receiced from %02x%02x%02x%02x%02x%02x [%02x] %s", mac[0], mac[1], mac[2], 
                                                                  mac[3], mac[4], mac[5], myData.action, myData.info);
     message(msg);
     return;
  }
  #ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, HIGH);
  #endif
}



void handleCommand() {
  inputString = inputString.substring(3);
//  Serial.println("* Command:" + inputString+" ");
  if (inputString.equals("MasTer")) {
    isMaster = true;
    EEPROM.writeBool(EEPROMaddr, isMaster);
    EEPROM.commit();
    broadcast( 0x01, "");
    Serial.println("! OK");
  } else if (inputString.startsWith("IP:")) {
    inputString = inputString.substring(3);
    broadcast(0x05, MasterIP+"|"+inputString);
    MasterIP = inputString;
    EEPROM.writeString(EEPROMaddr+10, MasterIP);
    EEPROM.commit();
    Serial.println("! OK");
  } else if (inputString.startsWith("UpDate:")) {
    if (!isMaster) {
      Serial.println("! ERROR");
      return;
    }
    inputString = inputString.substring(7);
    size_t  size = inputString.toInt();

    load_update( size);
    
  } else if (inputString.equals("clear")) {
      for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 255);
      }
      EEPROM.commit();
      masterAddress[0]=0;
      isMaster = false;
      MasterIP = "<none>";
      Serial.println("! OK");
  } else if (inputString.startsWith("gone:")) {
    if (!isMaster) {
      Serial.println("! ERROR");
      return;
    }
    inputString = inputString.substring(5);
    byte mac[6];
    hexCharacterStringToBytes(mac, inputString.c_str());
    if (esp_now_is_peer_exist(mac)) {    
       esp_now_del_peer(mac);
       Serial.println("! OK");
    } else {
       Serial.println("! ERROR");
    }
  } else if (inputString.equals("test")) {
    Serial.print("* ");
    if (masterAddress[0]==0) {
      Serial.print("<No Master Mac> ");
    } else {  
      serial_mac(masterAddress);
    }
    Serial.print(MasterIP);
    Serial.println(isMaster?" Master":" Client");
    Serial.println("! OK");
  } else if (inputString.startsWith("channel:")) {
    if (isMaster) {
      Serial.println("! ERROR");
      return;
    }
    inputString = inputString.substring(8);
    channel = inputString.toInt();
    esp_now_deinit();
    masterAddress[0]=0;
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    
    //Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
  
    EEPROM.write(EEPROMaddr+4, channel);
    EEPROM.commit();
    Serial.println("! OK");
  } else {
     Serial.println("! ERROR");
  }
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}


// callback function that will be executed when data is received
void handleServerMsg(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  // Copies the sender mac address to a string
/*
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  
  board["id"] = incomingReadings.id;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.hum;
  board["readingId"] = String(incomingReadings.readingId);
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());

  */
}


String GetExternalIP()
{
  String ip ;
/*  
  WiFiClient client;
  if (!client.connect("api.ipify.org", 80)) {
    Serial.println("Failed to connect with 'api.ipify.org' !");
  }
  else {
    int timeout = millis() + 5000;
    client.println("GET /?format=text HTTP/1.1\r\nHost: api.ipify.org\r\n\r\n");
 
    while (client.available() == 0) {
      if (timeout - millis() < 0) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return "";
      }
    }
    int size;
     ip[0] = 0;
    while ((size = client.available()) > 0) {
      char * msg = (char *)malloc(size);
      size = client.readBytes(msg, size);
&msg[size]= "\0";
      strcat(ip ,   msg);
      free(msg);
    }
       Serial.println(ip);
  }
  */

  HTTPClient http;
  http.begin("http://api.ipify.org/?format=text");
  int statusCode = http.GET();
  ip = http.getString();
  http.end();
  return ip ;
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
  EEPROM.begin(512);

  MasterIP = "<none>";
  inputString.reserve(300);
  if (EEPROM.read(EEPROMaddr) != 255) {
    isMaster = EEPROM.read(EEPROMaddr);
  }
  if ( EEPROM.read(EEPROMaddr+10) != 255) {
    MasterIP = EEPROM.readString(EEPROMaddr+10);
    MasterIP.trim();
    if (MasterIP=="") MasterIP = "<none>";
  }


  
  if (isMaster) {
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("* SPIFFS Mount Failed");
        return;
    }
  }
 

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);

  if (!isMaster && EEPROM.read(EEPROMaddr+4) != 255) {
    channel = EEPROM.read(EEPROMaddr+4);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
  }
  
  if (isMaster) {
    // Set device as a Wi-Fi Station
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("* Setting as a Wi-Fi Station..");
    }

    createUserList();
  } 


  
  #ifdef LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
  #endif


  if (isMaster) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });
     
    events.onConnect([](AsyncEventSourceClient *client){
      if(client->lastId()){
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      // send event with message "hello!", id current millis
      // and set reconnect delay to 1 second

      StaticJsonDocument<256> doc;
      doc["version"]   = VERSION;
      doc["masterIP"]  = MasterIP;
      doc["externelIP"] = GetExternalIP();
      doc["localIP"]   = IpAddress2String(WiFi.localIP());
      doc["gatewayIP"] = IpAddress2String(WiFi.gatewayIP());
      doc["mac"]       = WiFi.macAddress();
      doc["channel"]   = WiFi.channel();
      char jsonString[512];
      serializeJson(doc, jsonString);
      client->send(jsonString, "new_status", millis());
     });
    

    
    server.addHandler(&events);
    server.begin();
  }


  message ("Running as " + isMaster?"master":"client"); 
  message ("Version: "+ String(VERSION,3));
  
  Serial.print(F("* MasterIP: "));
  Serial.println(MasterIP);
  Serial.print  (F("* MAC Address: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("* Station IP Address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("* WiFi Channel: "));
  channel = WiFi.channel();
  Serial.println(channel);
  Serial.print(F("* Externel IP: "));
  Serial.println(GetExternalIP());

  
  if (esp_now_init() != 0) {
    Serial.println("* Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  lastTime = millis() - (timerDelay+10);
    if (isMaster && EEPROM.read(EEPROMaddr+2) != 255) {
    broadcast_update();
  }
}

unsigned long lastEventTime = millis();

void loop() {
  if (isMaster) {
    if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
     events.send("ping",NULL,millis());
      lastEventTime = millis();
    }
  }

  if (read_idx != write_idx) {
      nowtalk_t *espnow = &circbuf[read_idx];
      handlePackage(espnow->mac, espnow->buf, espnow->count);
      read_idx = (read_idx + 1) % QUEUE_SIZE;
  }
  
  if (!isMaster && (millis() - lastTime) > timerDelay) {
    
  #ifdef LED_BUILTIN
    digitalWrite(LED_BUILTIN, LOW);
  #endif
    // Set values to send
    if (masterAddress[0] == 0) {
      Serial.println("* Search master");
      broadcast( 0x02, MasterIP);
    } else {
      send_message(masterAddress, 0x02, "");
    }
    lastTime = millis();
  }
  
  if (stringComplete) {
    
    if (inputString.startsWith("###")) {
      
      handleCommand();
        
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  } else {
    serialEvent();
  }
}
