#pragma once

// Button
const uint8_t pinBUTTON = 7;		// PA21

// RGB Led pins
const uint8_t pinRED 	= 6;		// PA20
const uint8_t pinGREEN 	= 12;		// PA19
const uint8_t pinBLUE 	= 10;		// PA18

// ESP pins
const uint8_t pinPOWER_ESP = 	30;	// PB22
const uint8_t pinESP_TX_WIFI = 	17;	// PA04 - Txwifi in SERCOM0:PAD[0]
const uint8_t pinESP_RX_WIFI = 	18; // PA05 - RXwifi in SERCOM0:PAD[1]
const uint8_t pinESP_GPIO0 = 	16;	// PB08
const uint8_t pinESP_CH_PD = 	15;	// PB09

// Power management
const uint8_t pinBATTERY_ALARM = 14;	// PA02 -- ALARMB -- Battery alarm
const uint8_t pinOTG = 8;				// PA06 -- Charger OTG control
const uint8_t pinCHARGER_INT = 	42;		// PA03 -- Charger interrupt

// Sdcard
const uint8_t pinCARD_DETECT = 5;		// PA15 -- Card detect
const uint8_t pinCS_SDCARD = 2;			// PA14 -- SPI Select SDcard
const uint8_t pinCS_FLASH = 38;			// PA13 -- SPI Select Flash

// I2C internal bus
const uint8_t pinWIRE_SDA = 20;			// PA22 SDA_A
const uint8_t pinWIRE_SCL = 21; 		// PA23 SCL_A

// Auxiliary I2C bus
const uint8_t pinPOWER_AUX_WIRE = 31;	// PB23 - Power Auxiliary I2C bus
const uint8_t pinAUX_WIRE_SDA = 11;		// PA16 - SERCOM1:PAD[0]
const uint8_t pinAUX_WIRE_SCL = 13;		// PA17 - SERCOM1:PAD[1]

// I2S bus
const uint8_t pinI2S_SCK = 2;			// PA10 - I2S_SCK
const uint8_t pinI2S_SD = 9;			// PA7 - I2S_SD
const uint8_t pinI2S_FS = 0;			// PA11 - I2S_FS

// BOARD CONNECTOR
const uint8_t pinBOARD_CONN_3 = 3;				// PA9
const uint8_t pinBOARD_CONN_4 = pinWIRE_SCL;	// PA23 SCL_A
const uint8_t pinBOARD_CONN_5 = 4;				// PA8
const uint8_t pinBOARD_CONN_6 = pinWIRE_SDA; 	// PA22 SDA_A
// const uint8_t pinBOARD_CONN_7 =					// NOT CONNECTED
const uint8_t pinBOARD_CONN_8 =	pinI2S_SCK;		// PA10 - I2S_SCK
const uint8_t pinBOARD_CONN_9 =	25;				// PB3
const uint8_t pinBOARD_CONN_10 = pinI2S_SD;		// PA7 - I2S_SD
const uint8_t pinBOARD_CONN_11 = 19;			// PB2
const uint8_t pinBOARD_CONN_12 = pinI2S_FS;		// PA11 - I2S_FS
