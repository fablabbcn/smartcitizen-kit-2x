#pragma once

#include <Wire.h>
#include <Arduino.h>

#include <Sensors.h>
#include "Pins.h"
#include "MAX30105.h"
#include <Adafruit_MPL3115A2.h>
#include "SckSoundTables.h"
#include <I2S.h>

// Firmware for SmartCitizen Kit - Urban Sensor Board SCK 2.0
// It includes drivers for this sensors:
//
// * Light - BH1721 -> (0x29)
// * Temperature and Humidity - SHT31 -> (0x44)
// * CO and NO2 - MICS4515	-> (digital POT 0x2F)
// * Noise  - Invensense ICS43432 I2S microphone;microphone:
// * Barometric pressure - MPL3115 -> (0x60)
// * Dust Particles - MAX30105 -> (0x57)

enum SensorState
{
	SENSOR_BUSY,
	SENSOR_READY,
	SENSOR_ERROR
};

// Light
class Sck_BH1721FVC
{
	// Datasheet
	// http://rohmfs.rohm.com/en/products/databook/datasheet/ic/sensor/light/bh1721fvc-e.pdf

	private:
		bool sendCommand(byte wichCommand);
		byte readRegister(byte wichRegister);

	public:
		uint8_t address = 0x29;
		float reading;
		bool start();
		bool stop();
		bool get(bool wait=true);
};

// Temperature and Humidity
class Sck_SHT31
{
	// Datasheet
	// https://www.sensirion.com/fileadmin/user_upload/customers/sensirion/Dokumente/2_Humidity_Sensors/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital.pdf
	// This code is based on Adafruit SHT31 library, thanks! (https://github.com/adafruit/Adafruit_SHT31)
	private:

		TwoWire *_Wire;

		// Commands
		const uint16_t SOFT_RESET = 0x30A2;
		const uint16_t SINGLE_SHOT_HIGH_REP = 0x2400;

		uint32_t timeout = 15;	// Time in ms to wait for a reading
		uint32_t lastTime = 0;
		void sendComm(uint16_t comm);
		uint8_t crc8(const uint8_t *data, int len);
	public:
		uint8_t address = 0x44;
		// Conntructor
		Sck_SHT31(TwoWire *localWire);

		float temperature;
		float humidity;
		bool start();
		bool stop();
		bool update(bool wait=true);
};

// Gases CO and NO2
class Sck_MICS4514
{
	// Datasheet
	// http://files.manylabs.org/datasheets/MICS-4514.pdf
	// TODO create a generic class and instantiate CO, NO2
	private:

		// Carbon Monoxide
		const uint8_t pinPWM_HEATER_CO = pinBOARD_CONN_5;		// PA8 - HEAT_CO
		const uint8_t pinREAD_CO = pinBOARD_CONN_9;			// PB3 - READ_CO
		const float dutyCycle_CO = 50.8;

		// Nitrogen Dioxide
		const uint8_t pinPWM_HEATER_NO2 = pinBOARD_CONN_3;		// PA9 - HEAT_NO2
		const uint8_t pinREAD_NO2 = pinBOARD_CONN_11;			// PB2 - READ_NOX
		const float dutyCycle_NO2 = 76.2;

		const uint32_t ANALOG_RESOLUTION = 4095;
		const uint32_t VCC = 3300;

		bool heaterRunning = false;
		uint32_t startHeaterTime = 0;
		uint32_t stopHeaterTime = 0;

		// CO2 Fixed resistor
		uint32_t coLoadResistor = 750000;

		// NO2 adjustable load resistor
		const byte POT_NO2_LOAD_ADDRESS = 0x2F;
		const float ohmsPerStep	= 10000.0/127; // Ohms for each potenciometer step
		bool setNO2load(uint32_t value);

	public:
		float coResistance;
		float no2Resistance;
		uint16_t no2LoadResistor;
		bool start(uint32_t startTime);
		bool stop(uint32_t stopTime);
		bool startHeater();
		bool getCOresistance();
		bool getNO2resistance();
		bool getNO2load();
		uint32_t getHeatTime(uint32_t currentTime);
		float average(uint8_t wichPin);
};

// Noise
class Sck_Noise
{
	private:
		const uint16_t CONST_FACTOR = 256;
		const double RMS_HANN = 0.61177;
		const uint8_t FULL_SCALE_DBSPL = 120;
		const uint8_t BIT_LENGTH = 24;
		const double FULL_SCALE_DBFS = 20*log10(pow(2,(BIT_LENGTH)));
		bool FFT(int32_t *source);
		void arm_bitreversal(int16_t * pSrc16, uint32_t fftLen, uint16_t * pBitRevTab);
		void arm_radix2_butterfly( int16_t * pSrc, uint32_t fftLen, int16_t * pCoef);
		void applyWindow(int16_t *src, const int16_t *window, uint16_t len);
		double dynamicScale(int32_t *source, int16_t *scaledSource);

	public:
		const uint32_t sampleRate = 44100;
		static const uint16_t SAMPLE_NUM = 512;
		static const uint16_t FFT_NUM = 256;
		float readingDBA;
		float readingDBC;
		float readingDBZ;
		int32_t readingFFT[FFT_NUM];
		bool start();
		bool stop();
		bool getReading();

};

// Barometric pressure and Altitude
class Sck_MPL3115A2
{
	// Datasheet
	// https://cache.freescale.com/files/sensors/doc/data_sheet/MPL3115A2.pdf

	private:
		Adafruit_MPL3115A2 Adafruit_mpl3115A2 = Adafruit_MPL3115A2();

	public:
		uint8_t address = 0x60;
		float altitude;
		float pressure;
		float temperature;
		bool start();
		bool stop();
		bool getAltitude(bool wait=true);
		bool getPressure(bool wait=true);
		bool getTemperature(bool wait=true);
};

// Dust Particles
class Sck_MAX30105
{
	// Datasheet
	// https://datasheets.maximintegrated.com/en/ds/MAX30105.pdf

	private:
		MAX30105 sparkfun_max30105;

	public:
		uint8_t address = 0x57;
		float redChann;
		float greenChann;
		float IRchann;
		float temperature;
		bool start();
		bool stop();
		bool getRed(bool wait=true);
		bool getGreen(bool wait=true);
		bool getIR(bool wait=true);
		bool getTemperature(bool wait=true);	// NOT WORKING!!! (sparkfun lib)
};

class SckBase;

class SckUrban
{
	private:
		struct Resistor {
			byte deviceAddress;
			byte resistorAddress;
		};
	public:
		bool setup(SckBase *base);
		bool start(SensorType wichSensor);
		bool stop(SensorType wichSensor);

		// String getReading(); https://stackoverflow.com/questions/14840173/c-same-function-parameters-with-different-return-type
		String getReading(SckBase *base, SensorType wichSensor, bool wait=true);
		bool control(SckBase *base, SensorType wichSensor, String command);

		// Light
		Sck_BH1721FVC sck_bh1721fvc;

		// Temperature and Humidity
		Sck_SHT31 sck_sht31 = Sck_SHT31(&Wire);

		// Gases CO and NO2
		Sck_MICS4514 sck_mics4514;

		// Noise
		Sck_Noise sck_noise;

		// Barometric pressure and Altitude
		Sck_MPL3115A2 sck_mpl3115A2;

		// Dust Particles
		Sck_MAX30105 sck_max30105;
};

