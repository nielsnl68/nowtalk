/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp8266-nodemcu-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Arduino.h>

//#include <esp_now.h>
//#include <WiFi.h>
//#include <esp_wifi.h>


//#include "FS.h"
//#include "SPIFFS.h"
//#include "variables.h"

// #include "nowtalk.h"
/*
#include "switchboard.h"
#include "firmwareUpd.h"
//#include "talkwebsite.h"




*/
#ifdef SERVER 
#endif

#ifdef __ESP_NOW_H__
// Callback when data is sent
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
  if (sendStatus != ESP_NOW_SEND_SUCCESS)
  {
    if (config.isMaster)
    {
    ////  Handle_Node(mac, "Gone", "");
    }
    else
    {
      message(("* search new master"));
      bool isNewMaster = (config.masterAddress[0] == 0);
      if (!isNewMaster)
        esp_now_del_peer(config.masterAddress);
      config.masterAddress[0] = 0;
      broadcast(0x02, config.masterIP);
    }
  }
}

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *buf, int count)
{
  // copy to circular buffer
  nowtalk_t *data = &circbuf[write_idx];
  memcpy(data->mac, mac, 6);
  memcpy(data->buf, buf, count);
  data->count = count;

  write_idx = (write_idx + 1) % QUEUE_SIZE;
}
#endif

/***

void handleCommand()
{
  inputString = inputString.substring(3);
  //  Serial.println("* Command:" + inputString+" ");
  if (inputString.equals("MasTer"))
  {
    config.isMaster = true;
    saveConfiguration();
    Serial.println("! OK");
    ESP.restart();
  }
  else if (inputString.startsWith("IP:"))
  {
    inputString = inputString.substring(3);
    broadcast(0x05, String(config.masterIP) + "|" + inputString);
    strlcpy(config.masterIP, inputString.c_str(), sizeof(config.masterIP));

    Serial.println("! OK");
    saveConfiguration();
  }
  else if (inputString.startsWith("Name:"))
  {
    inputString = inputString.substring(3);
    strlcpy(config.userName, inputString.c_str(), sizeof(config.userName));

    Serial.println("! OK");
    saveConfiguration();
  }
#ifdef NOWTALK_UPD
  else if (inputString.startsWith("Update:"))
  {
    if (!config.isMaster)
    {
      Serial.println("! ERROR");
      return;
    }
    inputString = inputString.substring(7);
    String test = inputString.substring(0, inputString.indexOf(" "));
    size_t size = inputString.substring(inputString.indexOf(" ") + 1).toInt();
    boolean isClient = false;
    if (test == "master")  {
      isClient = false;
    } else if (test == "client"){
      isClient = true;
    } else {
      Serial.println("! ERROR");
      return;
    }

    load_update(size, isClient);
  }
  #endif
  else if (inputString.equals("clear"))
  {
    loadConfiguration(true);
    Serial.println("! OK");
    ESP.restart();
  }
  else if (inputString.startsWith("gone:"))
  {
    if (!config.isMaster)
    {
      Serial.println("! ERROR");
      return;
    }
    inputString = inputString.substring(5);
    byte mac[6];
    hexCharacterStringToBytes(mac, inputString.c_str());
    if (esp_now_is_peer_exist(mac))
    {
      esp_now_del_peer(mac);
      Serial.println("! OK");
    }
    else
    {
      Serial.println("! ERROR");
    }
  }
  else if (inputString.equals("test"))
  {
    Serial.print("* ");
    if (config.masterAddress[0] == 0)
    {
      Serial.print("<No Master Mac> ");
    }
    else
    {
      serial_mac(config.masterAddress);
    }
    Serial.print(config.masterIP);
    Serial.println(config.isMaster ? " Master" : " Client");
    Serial.println("! OK");
  }
  else if (inputString.startsWith("channel:"))
  {
    if (config.isMaster)
    {
      Serial.println("! ERROR");
      return;
    }
    inputString = inputString.substring(8);
    config.channel = inputString.toInt();
    esp_now_deinit();
    config.masterAddress[0] = 0;

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    //Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    Serial.println("! OK");
    saveConfiguration();

  }
  else
  {
    Serial.println("! ERROR");
  }
}

void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '\n')
    {
      stringComplete = true;
    }
    else
    {
      inputString += inChar;
    }
  }
}

*/



void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);
  /*
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    Serial.println("* SPIFFS Mount Failed");
    return;
  }
  loadConfiguration();

  inputString.reserve(300);
  
  
  if (config.isMaster)
  {
#ifdef NOWTALK_SWITCH
    setupDatabase();
#endif
  }



  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);

  if (!config.isMaster)
  {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
  }

  if (config.isMaster)
  {
    // Set device as a Wi-Fi Station
    WiFi.begin(config.ssid, config.password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.println("* Setting as a Wi-Fi Station..");
    }
#ifdef NOWTALK_WEB
    webSetup();
#endif
  }

  message("Running as " + config.isMaster ? "master" : "client");
  message("Version: " + String(VERSION, 3));

  Serial.print(F("* MasterIP: "));
  Serial.println(config.masterIP);
  Serial.print(F("* MAC Address: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("* Station IP Address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("* WiFi Channel: "));
  config.channel = WiFi.channel();
  Serial.println(config.channel);
  Serial.print(F("* Externel IP: "));
  Serial.println(GetExternalIP());

  if (esp_now_init() != 0)
  {
    Serial.println("* Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  config.lastTime = millis() - (config.timerDelay + 10);
  */
}


void loop()
{
  
#ifdef NOWTALK_WEB
  webLoop();
#endif
/*
  if (read_idx != write_idx)
  {
    nowtalk_t *espnow = &circbuf[read_idx];
#ifdef NOWTALK_SWITCH
    handlePackage(espnow->mac, espnow->buf, espnow->count);
#endif
    read_idx = (read_idx + 1) % QUEUE_SIZE;
  }

  if (!config.isMaster && (millis() - config.lastTime) > config.timerDelay)
  {
    // Set values to send
    if (config.masterAddress[0] == 0)
    {
      Serial.println("* Search master");
      broadcast(0x02, "");
    }
    else
    {
      send_message(config.masterAddress, 0x02, "");
    }
    config.lastTime = millis();
  }

  if (stringComplete)
  {

    if (inputString.startsWith("###"))
    {

      handleCommand();
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  else
  {
    serialEvent();
  }
  */
}
