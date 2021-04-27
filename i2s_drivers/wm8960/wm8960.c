#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "driver/i2c.h"

#include "i2c_bus.h"

#include "wm8960.h"

/* R25 - Power 1 */
#define WM8960_VMID_MASK 0x180
#define WM8960_VREF      0x40

/* R26 - Power 2 */
#define WM8960_PWR2_LOUT1	0x40
#define WM8960_PWR2_ROUT1	0x20
#define WM8960_PWR2_OUT3	0x02

/* R28 - Anti-pop 1 */
#define WM8960_POBCTRL   0x80
#define WM8960_BUFDCOPEN 0x10
#define WM8960_BUFIOEN   0x08
#define WM8960_SOFT_ST   0x04
#define WM8960_HPSTBY    0x01

/* R29 - Anti-pop 2 */
#define WM8960_DISOP     0x40
#define WM8960_DRES_MASK 0x30

static const char *TAG = "WM8960";

static const char *audio_nvs_nm = "audio";
static const char *vol_nvs_key = "dac_volume";

/* Forward Declarations for static functions */
/**
  * @brief  Write register of WM8960.
  * @param  reg: The number of register which to be read.
  * @retval The value of register.
  */
static esp_err_t wm8960_write_register(i2c_dev_t *dev, uint8_t reg_addr, uint16_t reg_val);

/**
  * @brief  Read register of WM8960.
  * @param  reg: The number of register which to be read.
  * @retval esp_err_t.
  */
static esp_err_t wm8960_read_register(i2c_dev_t *dev, uint8_t reg_addr);

/**
  * @brief  Set WM8960 Fractional PLL.
  * @param  PLLN: The integer part of R.
  * @param  reg: The fractional part of R.
  * @retval esp_err_t.
  */
static esp_err_t wm8960_set_pll(i2c_dev_t *dev, uint8_t PLLN, uint32_t PLLK);

/**
  * @brief  Set WM8960 I2S Interface.
  * @retval esp_err_t.
  */
static esp_err_t wm8960_set_interface(i2c_dev_t *dev);

/**
  * @brief  Setup WM8960 Power Management.
  * @retval esp_err_t.
  */
static esp_err_t wm8960_set_power(i2c_dev_t *dev);

/**
  * @brief  Setup WM8960 DAC.
  * @retval esp_err_t.
  */
static esp_err_t wm8960_dac_config(i2c_dev_t *dev);

/*
 * wm8960 register cache
 * We can't read the WM8960 register space when we are
 * using 2 wire for device control, so we cache them instead.
 */
static uint16_t wm8960_reg_val[56] = {
    0x0097, 0x0097, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x000A,
    0x01C0, 0x0000, 0x00FF, 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x007B, 0x0100, 0x0032, 0x0000, 0x00C3, 0x00C3, 0x01C0,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0100, 0x0100, 0x0050, 0x0050, 0x0050, 0x0050, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0050, 0x0050, 0x0000,
    0x0000, 0x0037, 0x004D, 0x0080, 0x0008, 0x0031, 0x0026, 0x00ED
};

esp_err_t wm8960_init(i2c_dev_t *dev){
    esp_err_t ret = 0;

    // Reset Wm8960
    ret = wm8960_write_register(dev, WM8960_RESET, 0x0);
    if(ret != ESP_OK)
        return ret;
    else
        ESP_LOGI(TAG, "reset completed");

    wm8960_set_clk(dev, 44100, 16);

    if(wm8960_set_power(dev) != 0)  {
        ESP_LOGI(TAG, "power mngr. config failed");
        printf("Source set fail !!\r\n");
        printf("Error code: %d\r\n",ret);
        return ret;
    }

    wm8960_dac_config(dev);

/*
    //Configure input routing
    //ret = wm8960_write_register(dev, 0x21, 1<<8 | 1<<3);      //Dont Configure LEFT
    ret = wm8960_write_register(dev, 0x21, 1<<8 | 1<<3);        //Connect RPGA to Boostmixer and MIC in single-ended mode
    ret |= wm8960_write_register(dev, 0x11, 1<<7);              //Enable Automatic Level Control
    ret |= wm8960_write_register(dev, 0x14, 1<<0);              //Enable Noise Gate
    if(ret != ESP_OK)  {
        ESP_LOGI(TAG, "input routing failed");
        return ret;
    }

    //Configure Jack Detect
    ret = wm8960_write_register(dev, 0x18, 1<<6 | 0<<5);       //Jack Detect
    ret |= wm8960_write_register(dev, 0x17, 1<<1 | 1<<0);
    ret |= wm8960_write_register(dev, 0x30, 1<<3 | 0<<2 | 1<<0);//0x000D,0x0005
*/
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "init complete");
    return ret;
}

esp_err_t wm8960_set_clk(i2c_dev_t *dev, int sample_rate, uint8_t bit_depth){
    esp_err_t ret;

    //Configure SYSCLK from MCLK
    if(sample_rate % 8000 == 0){
        /* SYSCLK = 12.288MHz
         * PLLin = 24/2
         * R = (4 x 2 x 12.288)/12MHz = 8.192
         * PLLN = int(R) = 8
         * PLLK = 2^24 * (R - int(R)) = 2^24 * 0.192 = 0x3126E8h
         */ 
        ret = wm8960_set_pll(dev, 0x08, 0x3126E8);
    } else {
        /* SYSCLK = 11.2896MHz
         * PLLin = 24/2
         * R = (4 x 2 x 11.2896)/12MHz = 7.5264
         * PLLN = int(R) = 7
         * PLLK = 2^24 * (R - int(R)) = 2^24 * 0.5264 = 0x86C226h
         */ 
        ret = wm8960_set_pll(dev, 0x07, 0x86C226);
    }
    if(ret != ESP_OK)  {
            ESP_LOGI(TAG, "SYSCLK config failed");
            return ret;
    }

    wm8960_set_interface(dev);

    //Configure peripheral clocking
    switch(sample_rate){
        case 32000: {
            wm8960_reg_val[WM8960_CLOCK1] |= 0x1<<6 | 0x1<<3; // ADC/DACDIV = SYSCLK / 1.5 = 32kHz
            ret = wm8960_write_register(dev, WM8960_CLOCK1, wm8960_reg_val[WM8960_CLOCK1]);
            break;
        }
        case 16000: {
            wm8960_reg_val[WM8960_CLOCK1] |= 0x3<<6 | 0x3<<3; // ADC/DACDIV = SYSCLK / 3 = 16kHz
            ret = wm8960_write_register(dev, WM8960_CLOCK1, wm8960_reg_val[WM8960_CLOCK1]);
            break;
        }
        case 8000: {
            wm8960_reg_val[WM8960_CLOCK1] |= 0x6<<6 | 0x6<<3; // ADC/DACDIV = SYSCLK / 6 = 8kHz
            ret = wm8960_write_register(dev, WM8960_CLOCK1, wm8960_reg_val[WM8960_CLOCK1]);
            break;
        }
        default: {
            ret = ESP_OK;
            break;
        }
    }
    if(ret != ESP_OK)  {
        ESP_LOGI(TAG, "SYSCLK config: failed to select sample rate");
        return ret;
    }
    ESP_LOGI(TAG, "sample rate set to %dkHz", sample_rate);
    return ret;
}

static esp_err_t wm8960_set_pll(i2c_dev_t *dev, uint8_t PLLN, uint32_t PLLK){
    esp_err_t ret;

    wm8960_reg_val[WM8960_CLOCK1] &= ~(1<<0); // PLL Selected
    if(wm8960_write_register(dev, WM8960_CLOCK1, wm8960_reg_val[WM8960_CLOCK1]) != ESP_OK){
        ESP_LOGI(TAG, "SYSCLK config: failed to deselect pll");
        ret |= ESP_FAIL;
    }

    wm8960_reg_val[WM8960_POWER2] |= 1<<0; // PLL On
    if(wm8960_write_register(dev, WM8960_POWER2, wm8960_reg_val[WM8960_POWER2]) != ESP_OK){
        ESP_LOGI(TAG, "SYSCLK config: pll failed to power on");
        ret |= ESP_FAIL;
    }

    wm8960_reg_val[WM8960_PLLN] |= 1<<5; // PLL Fractional Mode
    wm8960_reg_val[WM8960_PLLN] |= 1<<4; // PLL Prescaler On
    wm8960_reg_val[WM8960_PLLN] &= 0x1f0; // PLLN Reset
    wm8960_reg_val[WM8960_PLLN] |= PLLN; // PLLN Set
    if(wm8960_write_register(dev, WM8960_PLLN, wm8960_reg_val[WM8960_PLLN]) != ESP_OK){
        ESP_LOGI(TAG, "SYSCLK config: failed to set plln");
        ret |= ESP_FAIL;
    }
    
    wm8960_reg_val[WM8960_PLLK1] = PLLK >> 16; // PLLK High Word Set
    ret = wm8960_write_register(dev, WM8960_PLLK1, wm8960_reg_val[WM8960_PLLK1]);
    wm8960_reg_val[WM8960_PLLK2] = (PLLK >> 8) & ~(1<<8); // PLLK Middle Word Set
    ret |= wm8960_write_register(dev, WM8960_PLLK2, wm8960_reg_val[WM8960_PLLK2]);
    wm8960_reg_val[WM8960_PLLK3] = PLLK & ~(1<<8); // PLLK Low Word Set
    ret |= wm8960_write_register(dev, WM8960_PLLK3, wm8960_reg_val[WM8960_PLLK3]);
    if(ret != ESP_OK) ESP_LOGI(TAG, "SYSCLK config: failed to set pllk");

    wm8960_reg_val[WM8960_CLOCK1] |= 1<<2; // PLL POST Divider ON
    wm8960_reg_val[WM8960_CLOCK1] |= 1<<0; // PLL Selected
    if(wm8960_write_register(dev, WM8960_CLOCK1, wm8960_reg_val[WM8960_CLOCK1]) != ESP_OK){
        ESP_LOGI(TAG, "SYSCLK config: failed to select clock");
        ret |= ESP_FAIL;
    }

    return ret;
}

static esp_err_t wm8960_set_interface(i2c_dev_t *dev){
    esp_err_t ret;

    wm8960_reg_val[WM8960_IFACE1] &= 0x1F3; // Reset WL bits -> 16-bit audio
    wm8960_reg_val[WM8960_IFACE1] &= ~(1<<6); // Slave mode
    wm8960_reg_val[WM8960_IFACE1] &= 0x1FC; // Reset FORMAT bits
    wm8960_reg_val[WM8960_IFACE1] |= 1<<1; // I2S format
    ret = wm8960_write_register(dev, WM8960_IFACE1, wm8960_reg_val[WM8960_IFACE1]);

    return ret;
}

static esp_err_t wm8960_set_power(i2c_dev_t *dev){
    esp_err_t ret;

    wm8960_reg_val[WM8960_POWER1] &= ~(0x1FF); // Reset everything
    wm8960_reg_val[WM8960_POWER1] |= (1<<8 | 1<<7); // VMID: 2x5k for fast start-up
    wm8960_reg_val[WM8960_POWER1] |= (1<<6); // VREF On
    wm8960_reg_val[WM8960_POWER1] |= (1<<5 | 1<<4); // AINLR On
    wm8960_reg_val[WM8960_POWER1] |= (1<<3 | 1<<2); // ADCLR On
    wm8960_reg_val[WM8960_POWER1] |= (1<<1); // Mic BIAS on
    ret = wm8960_write_register(dev, WM8960_POWER1, wm8960_reg_val[WM8960_POWER1]);

    wm8960_reg_val[WM8960_POWER2] &= 0x001; // Reset everything except PLL EN
    wm8960_reg_val[WM8960_POWER2] |= (1<<8 | 1<<7); // DACLR On
    wm8960_reg_val[WM8960_POWER2] |= (1<<6 | 1<<5); // LROUT1 On
    //wm8960_reg_val[WM8960_POWER2] |= (1<<4 | 1<<3); // SPKLR On
    ret |= wm8960_write_register(dev, WM8960_POWER2, wm8960_reg_val[WM8960_POWER2]);

    wm8960_reg_val[WM8960_POWER3] &= ~(0x1FF); // Reset Everything
    wm8960_reg_val[WM8960_POWER3] |= (1<<5 | 1<<4); //LRMIC On
    wm8960_reg_val[WM8960_POWER3] |= (1<<3 | 1<<2); //LROMIX On
    ret |= wm8960_write_register(dev, WM8960_POWER3, wm8960_reg_val[WM8960_POWER3]);

    wm8960_reg_val[WM8960_ADDCTL1] &= ~(1<<8); //Disable Thermal Shutdown
    ret |= wm8960_write_register(dev, WM8960_ADDCTL1, wm8960_reg_val[WM8960_ADDCTL1]);

    return ret;
}

static esp_err_t wm8960_dac_config(i2c_dev_t *dev){
    esp_err_t ret;

    wm8960_reg_val[WM8960_LOUT1] |=  0x79;
    wm8960_reg_val[WM8960_ROUT1] |= (1<<8) | 0x79;
    ret = wm8960_write_register(dev, WM8960_LOUT1, wm8960_reg_val[WM8960_LOUT1]);
    ret |= wm8960_write_register(dev, WM8960_ROUT1, wm8960_reg_val[WM8960_ROUT1]);

    wm8960_reg_val[WM8960_LDACVOL] |=  0xD7;
    wm8960_reg_val[WM8960_RDACVOL] |=  (1<<8) | 0xD7;
    ret |= wm8960_write_register(dev, WM8960_LDACVOL, wm8960_reg_val[WM8960_LDACVOL]); //LDAC Volume
    ret |= wm8960_write_register(dev, WM8960_RDACVOL, wm8960_reg_val[WM8960_RDACVOL]); //RDAC Volume

    wm8960_reg_val[WM8960_LOUTMIX] |= (1<<8); //DAC -> LOUT2
    wm8960_reg_val[WM8960_ROUTMIX] |= (1<<8); //DAC -> ROUT2
    ret |= wm8960_write_register(dev, WM8960_LOUTMIX, wm8960_reg_val[WM8960_LOUTMIX]);
    ret |= wm8960_write_register(dev, WM8960_ROUTMIX, wm8960_reg_val[WM8960_ROUTMIX]);

    if(ret == ESP_OK){
        ret = wm8960_set_mute(dev, false);
    }

    return ret;
}

esp_err_t wm8960_set_mute(i2c_dev_t *dev, bool mute){
    esp_err_t ret = 0;

    if(!mute){
        wm8960_reg_val[WM8960_DACCTL1] &= ~(1<<3); //Unmute
        ret = wm8960_write_register(dev, WM8960_DACCTL1, wm8960_reg_val[WM8960_DACCTL1]);
    } else {
        wm8960_reg_val[WM8960_DACCTL1] |= 1<<3; //Mute
        ret = wm8960_write_register(dev, WM8960_DACCTL1, wm8960_reg_val[WM8960_DACCTL1]);
    }

    return ret;
}

esp_err_t wm8960_enable_soft_mute(i2c_dev_t *dev, bool enable){
    esp_err_t ret = 0;
    
    wm8960_reg_val[WM8960_DACCTL2] |= 1<<2; //Slow mute ramp rate
    if(!enable){
        wm8960_reg_val[WM8960_DACCTL2] &= ~(1<<3); //Disable
        ret = wm8960_write_register(dev, WM8960_DACCTL2, wm8960_reg_val[WM8960_DACCTL2]);
    } else {
        wm8960_reg_val[WM8960_DACCTL2] |= 1<<3; //Enable
        ret = wm8960_write_register(dev, WM8960_DACCTL2, wm8960_reg_val[WM8960_DACCTL2]);
    }

    return ret;
}

esp_err_t wm8960_set_3D(i2c_dev_t *dev, bool enable, uint8_t depth){
    esp_err_t ret;

    if(enable){
        wm8960_reg_val[WM8960_3D] |= (1<<0); //Enable 3D Mode
    } else {
        wm8960_reg_val[WM8960_3D] &= ~(1<<0); //Disable 3D Mode
    }
    wm8960_reg_val[WM8960_3D] |= depth<<1; //Set Percentage
    ret = wm8960_write_register(dev, WM8960_3D, wm8960_reg_val[WM8960_3D]);

    return ret;
}

esp_err_t wm8960_set_input_mute(i2c_dev_t *dev, bool mute){
    esp_err_t ret = 0;
    if (mute) {
        ret |= wm8960_write_register(dev, 0x00, 1<<8 | 1<<7 | 1<<6);
        ret |= wm8960_write_register(dev, 0x01, 1<<8 | 1<<7 | 1<<6);
    } else {
        ret |= wm8960_write_register(dev, 0x00, 1<<8 | 0<<7 | 1<<6);
        ret |= wm8960_write_register(dev, 0x01, 1<<8 | 0<<7 | 1<<6);
    }
    return ret;
}

/*esp_err_t wm8960_set_volume(i2c_dev_t *dev, uint8_t vol){
    esp_err_t ret = 0;

    uint8_t vol_to_set = 0x79; //80*(vol/100) + 47;

    wm8960_reg_val[WM8960_LOUT1] |=  0x79;
    wm8960_reg_val[WM8960_ROUT1] |= (1<<8) | 0x79;
    ret = wm8960_write_register(dev, WM8960_LOUT1, wm8960_reg_val[WM8960_LOUT1]);
    ret |= wm8960_write_register(dev, WM8960_ROUT1, wm8960_reg_val[WM8960_ROUT1]);

    wm8960_reg_val[WM8960_LDACVOL] |=  0xD7;
    wm8960_reg_val[WM8960_RDACVOL] |=  (1<<8) | 0xD7;
    ret |= wm8960_write_register(dev, WM8960_LDACVOL, wm8960_reg_val[WM8960_LDACVOL]); //LDAC Volume
    ret |= wm8960_write_register(dev, WM8960_RDACVOL, wm8960_reg_val[WM8960_RDACVOL]); //RDAC Volume

    return ret;
}*/

/*esp_err_t wm8960_set_volume(i2c_dev_t *dev, uint8_t vol, bool init){
    esp_err_t ret = 0;
    uint8_t vol_to_set = 0;
    nvs_handle_t audioStore;

    ret = nvs_open(audio_nvs_nm, NVS_READWRITE, &audioStore);

    if(init){
        ret |= nvs_get_u8(audioStore, vol_nvs_key, &vol_to_set);
    } else {
        if (vol == 0) {
            vol_to_set = 0;
        } else {
            ret |= nvs_set_u8(audioStore, vol_nvs_key, vol);
            vol_to_set = (vol/255) * 100;    //(vol / 10) * 5 + 200;
        }
    }
    
    ret |= wm8960_write_register(dev, 0x0A, vol_to_set);
    ret |= wm8960_write_register(dev, 0x0B, 0x100 | vol_to_set);

    ret |= nvs_commit(audioStore);
    nvs_close(audioStore);

    return ret;
}*/

esp_err_t wm8960_get_volume(uint8_t* vol){
    esp_err_t ret;
    nvs_handle_t audioStore;
    uint8_t volume = 0;

    ret = nvs_open(audio_nvs_nm, NVS_READONLY, &audioStore);
    ret |= nvs_get_u8(audioStore, vol_nvs_key, &volume);
    nvs_close(audioStore);

    *vol = volume;
    return ret;
}

static esp_err_t wm8960_write_register(i2c_dev_t *dev, uint8_t reg_addr, uint16_t reg_val){
    esp_err_t ret;
    uint8_t buff[2];
    uint16_t val = reg_val;

    buff[0] = (reg_addr << 1) |(uint8_t) ((val >> 8) & 0x01);
    buff[1] = (uint8_t) val & 0xFF;
    //printf("%x %x \t %x %x\n", reg_addr, val, buff[0], buff[1]);
    
    ret = i2c_bus_write_data(dev->port, dev->addr, buff, 2);
    if(ret == ESP_OK)
        wm8960_reg_val[reg_addr] = val;
    return ret;
}

static esp_err_t wm8960_read_register(i2c_dev_t *dev, uint8_t reg_addr){
    return wm8960_reg_val[reg_addr];
}