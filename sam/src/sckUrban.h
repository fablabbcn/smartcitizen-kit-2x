#pragma once

#include <Wire.h>
#include <Arduino.h>

#include "Constants.h"
#include <Sensors.h>



class SckUrban {
public:

	struct Resistor {
		byte deviceAddress;
		byte resistorAddress;
	};

	void setup();

	float getReading(SensorType wichSensor);
	String control(SensorType wichSensor, String command);

	// Noise
	float getNoise();
	void gainChange(uint8_t value);
	float getsound();
	void writeResistorRaw(byte resistor, int value);
	int gain_step = 0;
	float sounddB = 0;
	
	// Temp and humidity
	uint16_t readSHT(uint8_t type);
	float getHumidity();
	float getTemperature();
	
	// Light
	float getLight();

	// Gases
	const float HEATER_REGULATOR_RESISTOR = 24000;					// (Ohm) Heater voltage regulators resistor
	void gasSetup(SensorType wichSensor);
	void gasOn(SensorType wichSensor);
	void gasOff(SensorType wichSensor);
	void gasHeat(SensorType wichSensor, uint32_t wichCurrent);
	float gasGetRegulatorVoltage(SensorType wichSensor);
	float gasGetDropVoltage(SensorType wichSensor);
	void gasSetRegulatorVoltage(SensorType wichSensor, uint32_t wichVoltage);
	float gasGetHeaterCurrent(SensorType wichSensor);
	void gasCorrectHeaterCurrent(SensorType wichSensor);
	uint32_t gasHeatingTime(SensorType wichSensor);						// @return time since sensor heater started in seconds.
	uint32_t gasGetLoadResistance(SensorType wichSensor);
	float gasGetSensorResistance(SensorType wichSensor);
	float gasRead(SensorType wichSensor);
	/* Notes
		HeaterCurrent = HeaterVoltage / HEATER_RESISTOR
		HeaterResistance = (RegulatorVoltage - HeaterVoltage) / HeaterCurrent
		RegulatorVoltage = HeaterCurrent * (HeaterResistance + HEATER_RESISTOR)
	*/

	// Carbon Monoxide
	const uint8_t SHUTDOWN_CONTROL_REGULATOR_CO_SENSOR_HEATER_PIN = 9;		// (pin) 9-PA7 -- Low disables sensor heater
	const uint8_t CO_HEATER_VOLTAGE_PIN = 15;								// (pin) A1-PB8 -- CO Current Sensor
	const uint8_t CO_SENSOR_VOLTAGE_PIN = 17;								// (pin) A3-PA4 -- CO Sensor
	Resistor POT_CO_LOAD_RESISTOR = {POT3, 0x00};
	Resistor POT_CO_REGULATOR = {POT2, 0x00};
	const uint8_t CO_HEATER_RESISTOR = 10; 									// (Ohm) RDRED Resistencia en el sensor CO sensor
	uint32_t CO_HEATING_CURRENT = 32;										// (mA) Normal operational current
	uint32_t CO_HEATER_RESISTANCE = 74;									// (Ohm) Heating resistance at nominal power 
	bool gasCOheaterState = false;
	uint32_t startHeaterTime_CO = 0;

	// Nitrogen Dioxide
	const uint8_t SHUTDOWN_CONTROL_REGULATOR_NO2_SENSOR_HEATER_PIN = 8;		// (pin) 8-PA6 Low disables sensor heater
	const uint8_t NO2_HEATER_VOLTAGE_PIN = 16;								// (pin) A2-PB9 -- CO Current Sensor
	const uint8_t NO2_SENSOR_VOLTAGE_PIN = 18;								// (pin) A4-PA5 -- CO Sensor
	Resistor POT_NO2_LOAD_RESISTOR = {POT3, 0x01};
	Resistor POT_NO2_REGULATOR = {POT2, 0x01};
	const uint8_t NO2_HEATER_RESISTOR = 39; 								// (Ohm) Resistencia en el sensor NO2 sensor
	uint32_t NO2_HEATING_CURRENT = 26;										// (mA) Normal operational current
	uint32_t NO2_HEATER_RESISTANCE = 66;									// (Ohm) Heating resistance at nominal power
	bool gasNO2heaterState = false;
	uint32_t startHeaterTime_NO2 = 0;

	// Utility functions
	void setPot(Resistor wichPot, uint32_t value);
	uint32_t getPot(Resistor wichPot);
	void writeI2C(byte deviceaddress, byte address, byte data );
	byte readI2C(int deviceaddress, byte address);
	void ADCini();
	void ADCoff();
	float average(uint8_t wichPin);

private:
};



/*
NOTAS

-- Por I2C tenemos:
	* El potenciometro digital
	* Humidity
	* Temperature
	* UV
	* Light

-- Por pines analogicos
	* CO Sensor
	* NO Sensor
	* CO Current
	* NO Current
	* Sound Sensor

*/

