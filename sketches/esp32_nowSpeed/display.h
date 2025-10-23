#include <TFT_eSPI.h>
#include <SPI.h>
#include "esp_adc_cal.h"
#include <Wire.h>
#include <WiFi.h>

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34

uint16_t vref = 1100;
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

void initTFT(){
  
    /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
    */
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    } else {
        Serial.println("Default Vref: 1100mV");
    }

}

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void GoSleep() {
        int r = digitalRead(TFT_BL);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Press again to wake up",  tft.width() / 2, tft.height() / 2 );
        espDelay(6000);
        digitalWrite(TFT_BL, !r);

        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
        //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        delay(200);
        esp_deep_sleep_start();
}

void showVoltage(bool init= false)
{   
    static uint64_t timeStamp = 0;
    if (init) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE); // Orange
      tft.fillRect(0, 0, tft.width(), 20, TFT_BLUE);
      int x = tft.drawString("Mac:", 2, 2,2);
      tft.setTextColor(TFT_WHITE, TFT_BLUE); // Orange

      tft.setCursor(x+6,2,2);
      tft.print(WiFi.macAddress()); 
    }
    

    if (millis() - timeStamp > 1000|| init) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = String(battery_voltage,1) + "V";
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(TFT_LIGHTGREY, TFT_BLUE); // Orange
        tft.drawString(voltage,  tft.width()-2, 2, 2 );
        if (battery_voltage < 2.6) GoSleep();
    }
}

void showData(unsigned long send,unsigned long recv,unsigned long lost,unsigned long old,unsigned long error){
        tft.fillRect(0, 21, tft.width(), tft.height(), TFT_BLACK);
        
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(0x7BEF, TFT_BLACK); // Orange
        int x1,x2,x3,x4, x=30 ;
        x1 = tft.drawString("S:",  0+x, 40, 4 );
        x2 = tft.drawString("R:",  0+x, 80, 4 );
        x3 = tft.drawString("L:",120+x, 40, 4 );
        x4 = tft.drawString("O:",120+x, 80, 4 );
        tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Orange
        tft.setCursor(x1+4+x,40,4);
        tft.printf("%4d",send); 
        tft.setCursor(x1+4+x,80,4);
        tft.printf("%4d",recv); 
        tft.setCursor(x4+124+x,40,4);
        tft.printf("%3d",lost); 
        tft.setCursor(x4+124+x,80,4);
        tft.printf("%3d",old); 
}

void ShowMessage(char * msg){
        
        showVoltage(true);
        tft.setTextDatum(MC_DATUM);
  
        tft.setTextColor(TFT_RED, TFT_BLACK); // Orange
        tft.setTextSize(3);
        tft.drawString( msg, tft.width() / 2, ((tft.height()-20) / 2) +20, 1 );
        tft.setTextSize(1);
}
