#ifndef _WM8960_H
#define _WM8960_H

#include "i2c_bus.h"

/* WM8960 register space */
#define WM8960_CACHEREGNUM 	56

#define WM8960_LINVOL		  0x00
#define WM8960_RINVOL		  0x01
#define WM8960_LOUT1		  0x02
#define WM8960_ROUT1		  0x03
#define WM8960_CLOCK1		  0x04
#define WM8960_DACCTL1		0x05
#define WM8960_DACCTL2		0x06
#define WM8960_IFACE1		  0x07
#define WM8960_CLOCK2		  0x08
#define WM8960_IFACE2		  0x09
#define WM8960_LDACVOL	  0x0a
#define WM8960_RDACVOL	  0x0b

#define WM8960_RESET		  0x0f
#define WM8960_3D		      0x10
#define WM8960_ALC1		    0x11
#define WM8960_ALC2		    0x12
#define WM8960_ALC3		    0x13
#define WM8960_NOISEGATE	0x14
#define WM8960_LADCVOL	  0x15
#define WM8960_RADCVOL	  0x16
#define WM8960_ADDCTL1		0x17
#define WM8960_ADDCTL2		0x18
#define WM8960_POWER1		  0x19
#define WM8960_POWER2		  0x1a
#define WM8960_ADDCTL3		0x1b
#define WM8960_APOP1		  0x1c
#define WM8960_APOP2		  0x1d

#define WM8960_LINPATH		0x20
#define WM8960_RINPATH		0x21
#define WM8960_LOUTMIX		0x22

#define WM8960_ROUTMIX		0x25
#define WM8960_MONOMIX1		0x26
#define WM8960_MONOMIX2		0x27
#define WM8960_LOUT2VOL		0x28
#define WM8960_ROUT2VOL		0x29
#define WM8960_MONOVOL	  0x2a
#define WM8960_INBMIX1		0x2b
#define WM8960_INBMIX2		0x2c
#define WM8960_BYPASS1		0x2d
#define WM8960_BYPASS2		0x2e
#define WM8960_POWER3		  0x2f
#define WM8960_ADDCTL4		0x30
#define WM8960_CLASSD1		0x31

#define WM8960_CLASSD3		0x33
#define WM8960_PLLN		    0x34
#define WM8960_PLLK1	    0x35
#define WM8960_PLLK2	    0x36
#define WM8960_PLLK3	    0x37


/*
 * WM8960 Clock dividers
 */
#define WM8960_SYSCLKDIV 		0
#define WM8960_DACDIV			  1
#define WM8960_OPCLKDIV			2
#define WM8960_DCLKDIV			3
#define WM8960_TOCLKSEL			4

#define WM8960_SYSCLK_DIV_1		(0 << 1)
#define WM8960_SYSCLK_DIV_2		(2 << 1)

#define WM8960_SYSCLK_MCLK		(0 << 0)
#define WM8960_SYSCLK_PLL		  (1 << 0)
#define WM8960_SYSCLK_AUTO		(2 << 0)

#define WM8960_DAC_DIV_1		  (0 << 3)
#define WM8960_DAC_DIV_1_5		(1 << 3)
#define WM8960_DAC_DIV_2		  (2 << 3)
#define WM8960_DAC_DIV_3		  (3 << 3)
#define WM8960_DAC_DIV_4		  (4 << 3)
#define WM8960_DAC_DIV_5_5		(5 << 3)
#define WM8960_DAC_DIV_6		  (6 << 3)

#define WM8960_DCLK_DIV_1_5		(0 << 6)
#define WM8960_DCLK_DIV_2		  (1 << 6)
#define WM8960_DCLK_DIV_3		  (2 << 6)
#define WM8960_DCLK_DIV_4		  (3 << 6)
#define WM8960_DCLK_DIV_6		  (4 << 6)
#define WM8960_DCLK_DIV_8		  (5 << 6)
#define WM8960_DCLK_DIV_12		(6 << 6)
#define WM8960_DCLK_DIV_16		(7 << 6)

#define WM8960_TOCLK_F19		  (0 << 1)
#define WM8960_TOCLK_F21		  (1 << 1)

#define WM8960_OPCLK_DIV_1		(0 << 0)
#define WM8960_OPCLK_DIV_2		(1 << 0)
#define WM8960_OPCLK_DIV_3		(2 << 0)
#define WM8960_OPCLK_DIV_4		(3 << 0)
#define WM8960_OPCLK_DIV_5_5	(4 << 0)
#define WM8960_OPCLK_DIV_6		(5 << 0)

i2c_dev_t wm8960;

/**
  * @brief  Initialise WM8960.
  * @param  dev: The i2c device corresponding to wm8960 units
  * @retval ESP_OK: success.
  */
esp_err_t wm8960_init(i2c_dev_t *dev);

/**
  * @brief  Set WM8960 sample_rate.
  * @param  dev: The i2c device corresponding to wm8960 units
  * @param  rate: audio sample rate
  * @retval ESP_OK: success.
  */
esp_err_t wm8960_set_clk(i2c_dev_t *dev, int sample_rate, uint8_t bit_depth);

/**
  * @brief  Enable WM8960 soft mute mode.
  * @param  dev: The i2c device corresponding to wm8960 units
  * @param  enable: set true to enable soft mute, set false to disable
  * @retval ESP_OK: success.
  */
esp_err_t wm8960_enable_soft_mute(i2c_dev_t *dev, bool enable);

/**
  * @brief  Enable WM8960 mute.
  * @param  dev: The i2c device corresponding to wm8960 units
  * @param  enable: set true to enable mute, set false to disable
  * @retval ESP_OK: success.
  */
esp_err_t wm8960_set_mute(i2c_dev_t *dev, bool mute);

/**
  * @brief  Enable WM8960 3D Stereo Mode.
  * @param  dev: The i2c device corresponding to wm8960 units
  * @param  enable: set true to enable 3d mode, set false to disable
  * @param  depth: set to value [0-16] for depth 
  * @retval ESP_OK: success.
  */
esp_err_t wm8960_set_3D(i2c_dev_t *dev, bool enable, uint8_t depth);

/**
  * @brief  Set WM8960 DAC volume.
  * @param  dev: The i2c device corresponding to wm8960 units
  * @param  vol: value of DAC volume [0-100]
  * @param  init: set true if in startup mode (reads volume from nvs)
  * @retval ESP_OK: success.
  */
//esp_err_t wm8960_set_volume(i2c_dev_t *dev, uint8_t vol);

/**
  * @brief  Get WM8960 DAC volume.
  * @param  vol: Read volume from nvs
  * @retval ESP_OK: success.
  */
esp_err_t wm8960_get_volume(uint8_t* vol);

#endif /* _WM8960_H */