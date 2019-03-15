#pragma once

#include <Wire.h>
#include <Arduino.h>

#include <Sensors.h>
#include "Pins.h"
#include <Adafruit_MPL3115A2.h>
#include "SckSoundTables.h"
#include <I2S.h>

// Firmware for SmartCitizen Kit - Urban Sensor Board SCK 2.0
// It includes drivers for this sensors:
//
// * Light - BH1721 -> (0x29)
// * Temperature and Humidity - SHT31 -> (0x44)
// * Noise  - Invensense ICS43432 I2S microphone;microphone:
// * Barometric pressure - MPL3115 -> (0x60)

// Pins
const uint8_t pinPM_SERIAL_RX = pinBOARD_CONN_11;
const uint8_t pinPM_SERIAL_TX = pinBOARD_CONN_13;

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

// Noise
class Sck_Noise
{
	private:
		const double RMS_HANN = 0.61177;
		const uint8_t FULL_SCALE_DBSPL = 120;
		const uint8_t BIT_LENGTH = 24;
		const double FULL_SCALE_DBFS = 20*log10(pow(2,(BIT_LENGTH)));
		bool FFT(int32_t *source);
		void arm_bitreversal(int16_t * pSrc16, uint32_t fftLen, uint16_t * pBitRevTab);
		void arm_radix2_butterfly( int16_t * pSrc, int16_t fftLen, int16_t * pCoef);
		void applyWindow(int16_t *src, const uint16_t *window, uint16_t len);
		double dynamicScale(int32_t *source, int16_t *scaledSource);
		void fft2db();

	public:
		bool debugFlag = false;
		const uint32_t sampleRate = 44100;
		static const uint16_t SAMPLE_NUM = 512;
		static const uint16_t FFT_NUM = 256;
		float readingDB;
		int32_t readingFFT[FFT_NUM];
		bool start();
		bool stop();
		bool getReading(SensorType wichSensor);

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

//PM sensors
class Sck_PM
{
	private:
		bool detectionFailed = false;

		uint32_t lastReading = 0;
		uint8_t values[6] = {0,0,0,0,0,0};	// 6 bytes 0:1->pm1, 2:3->pm25, 4:5->pm10

		static const uint8_t buffLong = 23;
		unsigned char buff[buffLong];

	public:
		// Readings
		uint16_t pm1;
		uint16_t pm25;
		uint16_t pm10;

		bool started = false;

		bool start();
		bool stop();
		bool update();
		bool reset();
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

		// Noise
		Sck_Noise sck_noise;

		// Barometric pressure and Altitude
		Sck_MPL3115A2 sck_mpl3115A2;

		// PM sensor
		Sck_PM sck_pm;
};

