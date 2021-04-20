#include "nowtalk.h"
#include "variabes.h"

void broadcast_update(fs::FS &fs, const uint8_t * mac ) {
   File updateBin = fs.open("/update.bin");
   if (updateBin) {
      
      size_t updateSize = updateBin.size();
      
      if (updateSize > 0) {
         Serial.println("* Try to start update");
         send_message(mac,0xe0,String(updateSize));
         add_peer(mac);   
         char buf [240];
         while (true) {
           size_t z = updateBin.readBytes(buf, 240);
           send_message(mac,0xe7, String(buf));
           if (z != 240) break;
         }
         send_message(mac,0xee,"");
          
      } else {
         Serial.println("* Error, file is empty");
      }

      updateBin.close();
      EEPROM.write(EEPROMaddr+2, 255);
      EEPROM.commit();  
        
   }
   else {
      Serial.println("* Could not load update.bin");
   }
}

void broadcast_update() {
  uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  broadcast_update(SPIFFS, broadcastAddress);
}

// perform the actual update from a given stream
bool performUpdate(Stream &updateSource, size_t updateSize) {
  bool result = false;
   if (Update.begin(updateSize)) {      
      size_t written = Update.writeStream(updateSource);
      if (written != updateSize) {
         Serial.println("!ERROR Size mismatch");
      }
      if (Update.end()) {
         if (Update.isFinished()) {
            Serial.println("! OK");
            result = true;
         } else {
            Serial.println("! ERROR Update not finished? Something went wrong!");
         }
      } else {
         Serial.println("!ERROR Error Occurred. Error #: " + String(Update.getError()));
      }

   } else {
      Serial.println("! ERROR Not enough space to begin OTA");
   }
   return result;
}

void load_update(int size) {
      fs::FS &fs = SPIFFS;
    if (!fs.exists("/update.bin") || fs.remove("/update.bin")) {
      File file = fs.open("/update.bin", FILE_WRITE);
      if(!file){
          Serial.println("! ERROR");
          return;
      }
      Serial.println("! READY");
      while (size >0) {
        while (Serial.available()) {
          char inChar = (char)Serial.read();
          if(!file.write(Serial.read())){
            Serial.println("! ERROR");
          }
          size --;
        }
      }
      size_t updateSize = file.size();

      if (updateSize > 0) {
         Serial.println("* Try to start update");
         if (performUpdate(file, updateSize)) {
            EEPROM.write(EEPROMaddr+2, 0);
            EEPROM.commit();
         }
      } else {
         Serial.println("! ERROR file is empty");
      }
      file.close();    
      delay(1000);
      ESP.restart();
    }

}
