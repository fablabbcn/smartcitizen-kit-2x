#pragma once

// Button
const uint8_t pinBUTTON = 7;			// PA21

// RGB Led pins
const uint8_t pinRED 	= 6;			// PA20
const uint8_t pinGREEN 	= 12;			// PA19
const uint8_t pinBLUE 	= 10;			// PA18

// Other leds
const uint8_t pinLED_USB = 26;			// PA27 -- Usb Serial Led

// ESP pins
const uint8_t pinPOWER_ESP = 	30;		// PB22 -- ESP power
const uint8_t pinESP_TX_WIFI = 	17;		// PA04 -- WiFi TX Serial -- SERCOM0:PAD[0]
const uint8_t pinESP_RX_WIFI = 	18; 		// PA05 -- WiFi RX Serial -- SERCOM0:PAD[1]
const uint8_t pinESP_GPIO0 = 	16;		// PB09 -- ESP GPIO0
const uint8_t pinESP_CH_PD = 	15;		// PB08 -- ESP CHPD

// Power management
const uint8_t pinMEASURE_BATT = 14;		// PA02 -- Battery analog level
const uint8_t pinCHARGER_INT = 	42;		// PA03 -- Charger interrupt

// Sdcard
const uint8_t pinCARD_DETECT = 5;		// PA15 -- Card detect
const uint8_t pinCS_SDCARD = 2;			// PA14 -- SPI Select SDcard
const uint8_t pinCS_FLASH = 38;			// PA13 -- SPI Select Flash

// I2C internal bus
const uint8_t pinWIRE_SDA = 20;			// PA22 -- SDA_A
const uint8_t pinWIRE_SCL = 21; 		// PA23 -- SCL_A

// Auxiliary I2C bus
const uint8_t pinPOWER_AUX_WIRE = 31;		// PB23 -- Power Auxiliary I2C bus
const uint8_t pinAUX_WIRE_SDA = 11;		// PA16 -- Auxiliary I2C SDA -- SERCOM1:PAD[0]
const uint8_t pinAUX_WIRE_SCL = 13;		// PA17 -- Auxiliary I2C SCL -- SERCOM1:PAD[1]

// I2S bus
const uint8_t pinI2S_SCK = 1;			// PA10 -- I2S_SCK
const uint8_t pinI2S_SD = 9;			// PA07 -- I2S_SD
const uint8_t pinI2S_FS = 0;			// PA11 -- I2S_FS

// PMS sensor
const uint8_t pinPM_ENABLE = 8;			// PA06 -- Enable PMS step up power (active HIGH)

// BOARD CONNECTOR
const uint8_t pinBOARD_CONN_3 = 3;		// PA09 -- Free 1
const uint8_t pinBOARD_CONN_4 = pinWIRE_SCL;	// PA23 -- SCL_A
const uint8_t pinBOARD_CONN_5 = 4;		// PA08 -- Free 2
const uint8_t pinBOARD_CONN_6 = pinWIRE_SDA; 	// PA22 -- SDA_A
const uint8_t pinBOARD_CONN_7 = 27;		// PA28 -- Free 3
const uint8_t pinBOARD_CONN_8 =	pinI2S_SCK;	// PA10 -- I2S_SCK
// const uint8_t pinBOARD_CONN_9 =		// 5v/1.5A Line
const uint8_t pinBOARD_CONN_10 = pinI2S_SD;	// PA07 -- I2S_SD
const uint8_t pinBOARD_CONN_11 = 25;		// PB03 -- RX_PMS -- PMS RX Serial -- SERCOM_ALT5:PAD[1]
const uint8_t pinBOARD_CONN_12 = pinI2S_FS;	// PA11 -- I2S_FS
const uint8_t pinBOARD_CONN_13 = 19;		// PB02 -- TX_PMS -- PMS TX Serial -- SERCOM_ALT5:PAD[0]
