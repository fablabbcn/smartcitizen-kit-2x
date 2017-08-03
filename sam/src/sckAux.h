#pragma once


// Include the necessary libraries for the auxiliary sensor to be used
#include <Wire.h>

// INA219 libs
#include <Adafruit_INA219.h>

// AlphaDelta libs
#include "Adafruit_SHT31.h"
#include <MCP3424.h>

// Groove_OLED libs
#include <U8g2lib.h>

// DS2482 library (I2C-1Wire bridge)
#include <DS2482.h>

#include <Sensors.h>


struct Resistor {
	byte deviceAddress;
	byte resistorAddress;
};

bool I2Cdetect(byte address);

class AuxBoards {
public:
	bool begin(SensorType wichSensor);
	float getReading(SensorType wichSensor);
	bool getBusyState(SensorType wichSensor);
	String control(SensorType wichSensor, String command);
	void print(SensorType wichSensor, String payload);
	void displayReading(String title, String reading, String unit, String time);


private:
};

class AlphaDelta {
public:

	const byte sht31Address = 0x44;

	bool begin();
	
	// SHT31 Temperature and Humidity Sensor
	Adafruit_SHT31 sht31 = Adafruit_SHT31();
	float getTemperature();
	float getHumidity();

	// Alphasense Gas Sensors
	enum Electrodes {AE_1, WE_1, AE_2, WE_2, AE_3, WE_3};
	double getElectrode(Electrodes wichElectrode);

	// Alphasense Digital Potenciometers
	Resistor POT_AE1 = {0x55, 0x01};
	Resistor POT_WE1 = {0x55, 0x00};
	Resistor POT_AE2 = {0x56, 0x01};
	Resistor POT_WE2 = {0x56, 0x00};
	Resistor POT_AE3 = {0x54, 0x01};
	Resistor POT_WE3 = {0x54, 0x00};

	void setPot(Resistor wichPot, uint32_t value);
	uint32_t getPot(Resistor wichPot);
	const float ohmsPerStep = 392.1568;     // Resistor conversion constant in Ohms. (100,000 / 255)

private:
};

class GrooveI2C_ADC {
public:

	bool begin();
	float getReading();

	const byte deviceAddress 	= 0x59;
	const float V_REF 			= 3.30;
	const byte REG_ADDR_RESULT	= 0x00;
	const byte REG_ADDR_ALERT	= 0x01;
	const byte REG_ADDR_CONFIG	= 0x02;
	const byte REG_ADDR_LIMITL	= 0x03;
	const byte REG_ADDR_LIMITH	= 0x04;
	const byte REG_ADDR_HYST	= 0x05;
	const byte REG_ADDR_CONVL	= 0x06;
	const byte REG_ADDR_CONVH	= 0x07;

private:
};

class INA219 {
public:

	const byte deviceAddress = 0x41;

	enum typeOfReading {BUS_VOLT, SHUNT_VOLT, CURRENT, LOAD_VOLT};

	Adafruit_INA219 ada_ina219 = Adafruit_INA219(deviceAddress);
	bool begin();
	float getReading(typeOfReading wichReading=CURRENT);

private:
};

class Groove_OLED {
public:

	const byte deviceAddress = 0x3c;

	struct displaySensorList {
		SensorType sensor;
		bool display;
	};

	U8G2_SSD1327_SEEED_96X96_F_HW_I2C U8g2_oled = U8G2_SSD1327_SEEED_96X96_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);

	bool begin();
	void print(String payload);
	void displayReading(String title, String reading, String unit, String time);

	displaySensorList displayable[SENSOR_COUNT];

private:
};

/*! @class DS2482_100
 *  @brief class for handling the DS18B20 temperature sensor connected to the I2C port
 *   through the DS2482-100 board. This was based on an example made by
 *   <a href="https://github.com/paeaetech/paeae.git">paeae</a>
 */
class WaterTemp_DS18B20 {
	//
public:

	byte deviceAddress = 0x18;

	DS2482 DS_bridge = DS2482(0);		// This means that we are using 0x18 I2C address

	byte data[8];
	byte addr[8];

	uint8_t conf =0x05;

	/**
	 * Start the transmission of data for the DS18B20 trough the DS2482_100 bridge
	 */
	bool begin();
	/**
	 * Read the temperature of the DS18B20 through the DS2482_100 bridge
	 * @return Temperature
	 */
	float getReading();
private:
};

class Atlas {
public:

	byte deviceAddress;
	SensorType atlasType;
	bool PH = false;
	bool EC = false;
	bool DO = false;
	float newReading;
	float newReadingB;
	String atlasResponse;
	uint32_t lastCommandSent = 0;
	enum State {
		REST,
		TEMP_COMP_SENT,
		ASKED_READING,
	};
	State state = REST;
	
	// Constructor varies by sensor type
	Atlas(SensorType wichSensor) {
		atlasType = wichSensor;

		switch(atlasType) {
			case SENSOR_ATLAS_PH: {
				deviceAddress = 0x63;
				PH = true;
				break;

			} case SENSOR_ATLAS_EC:
			case SENSOR_ATLAS_EC_SG: {
				deviceAddress = 0x64;
				EC = true;
				break;

			} case SENSOR_ATLAS_DO:
			case SENSOR_ATLAS_DO_SAT: {
				deviceAddress = 0x61;
				DO = true;
				break;				

			} default: break;
		}

    }

	bool begin();
	bool beginDone = false;
	float getReading();
	bool getBusyState();
	
	void goToSleep();
	bool sendCommand(char* command);
	bool tempCompensation();
	uint8_t getResponse();
	
	uint16_t longWait = 910; //ms
	uint16_t mediumWait = 610; //ms
	uint16_t shortWait = 310; //ms

private:
};
