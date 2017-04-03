#pragma once

/* 	-------------------------------
	|	SCK Baseboard Pinout	  |
 	-------------------------------
*/
// ESP8266 Management
#define CH_PD   		31		// PB23
#define POWER_WIFI  	30		// PB22
#define GPIO0   		11		// PA16
#define CS_ESP  		13		// PA17

// RGB Led 
#define PIN_LED_RED 	6		// PA20
#define PIN_LED_GREEN 	12		// PA19
#define PIN_LED_BLUE 	10		// PA18

// Button 
#define PIN_BUTTON 		7		// PA21

// Serial USB leds
#define SERIAL_TX_LED	26		// PA27
#define SERIAL_RX_LED	25		// PB3

// Sensor Board Conector
#define IO0 9					// PA7 -- CO Sensor Heather
#define IO1 8       			// PA6 -- NO2 Sensor Heater
#define IO2 3					// PA9 -- Unused
#define IO3 4					// PA8 -- Unused
#define S0 A3         			// PA4 -- CO Sensor
#define S1 A4         			// PA5 -- NO2 Sensor
#define S2 A1         			// PB8 -- CO Current Sensor
#define S3 A2         			// PB9 -- NO2 Current Sensor
#define S4 A5         			// PB2 -- Sound Sensor
#define S5 A0         			// PA2 -- Unused

// SPI Configuration
#define CS_SDCARD	2				// PA14 -- SPI Select SDcard

// Power Management
#define VCC 	3300.				// mV
#define PS 		38					// PA13 -- TPS63001 PS/SYNC

// I2C address
#define SHT21_I2C_DIR			0x40	// SHT21 Temperature and Humidity sensor
#define BH1730_I2C_DIR			0x29	// BH1730 Light sensor
#define ADC_DIR              	0x48    // Direction of the ADC
#define POT1                 	0x50
#define POT2                 	0x51    // I2C Direction of the Potenciometer 2 for MICS heather resistor (Rh) (channel_0 > CO) y (channel_1 > NO2)
#define POT3                 	0x52    // I2C Direction of the Potenciometer 3 for MICS sensor resistor (Rs)
#define POT4                 	0x53

#define RESOLUTION_ANALOG    	4095.   //Resolucion de las entradas analogicas
#define ohmsPerStep				392.1568     //Constante de conversion a resistencia de potenciometrosen ohmios