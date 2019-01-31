#pragma once

#include <Wire.h>
#include <Arduino.h>
#include <RTCZero.h>

#include <Sensors.h>
#include "Pins.h"
#include <Adafruit_MPL3115A2.h>
#include "SckSoundTables.h"
#include <I2S.h>
#include <SparkFunCCS811.h>


// Firmware for SmartCitizen Kit - Urban Sensor Board SCK 2.0
// It includes drivers for this sensors:
//
// * Light - BH1721 -> (0x29)
// * Temperature and Humidity - SHT31 -> (0x44)
// * Noise  - Invensense ICS43432 I2S microphone;microphone:
// * Barometric pressure - MPL3115 -> (0x60)
// * VOC and ECO2 - CCS811 -> (0x5a)

class SckBase;

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
		bool get();
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
		bool update();
};

// Noise
class Sck_Noise
{
	private:
		bool alreadyStarted = false;
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
		bool getAltitude();
		bool getPressure();
		bool getTemperature();
};

//PM sensors
class Sck_PM
{
	private:
		bool detectionFailed = false;
		uint32_t lastFail = 0;
		uint32_t lastReading = 0;

		static const uint8_t buffLong = 30; 	// Excluding both start chars

		// Serial transmission from PMS
		// 0: Start char 1 0x42 (fixed)
		// 1: Start char 2 0x4d (fixed)
		// 2-3 : Frame length = 2x13 + 2 (data + parity)

		// 4-5: PM1.0 concentration (CF = 1, standard particles) Unit ug/m^3
		// 6-7: PM2.5 concentration (CF = 1, standard particulates) Unit ug/m^3
		// 8-9: PM10 concentration (CF = 1, standard particulate matter) Unit ug/m^3

		// 10-11: PM1.0 concentration (in the atmosphere) Unit ug/m^3
		// 12-13: PM2.5 concentration (in the atmosphere) Unit ug/m^3
		// 14-15: PM10 concentration (in the atmosphere) Unit ug/m^3

		// 16-17: Particles in 0.1 liter of air > 0.3um 
		// 18-19: Particles in 0.1 liter of air > 0.5um 
		// 20-21: Particles in 0.1 liter of air > 1.0um 
		// 22-23: Particles in 0.1 liter of air > 2.5um 
		// 24-25: Particles in 0.1 liter of air > 5.0um 
		// 26-27: Particles in 0.1 liter of air > 10um 

		// 28: Version number
		// 29: Error code

		// 30-31: Sum of each byte from start_1 ... error_code 

		unsigned char buff[buffLong];
		uint32_t rtcStarted = 0;
		uint32_t rtcStopped = 0;
		uint32_t rtcReading = 0;
		RTCZero* rtc;

	public:
		Sck_PM(RTCZero* myrtc) {
			rtc = myrtc;
		}
		// Readings
		uint16_t pm1;
		uint16_t pm25;
		uint16_t pm10;
		uint16_t pn03;
		uint16_t pn05;
		uint16_t pn1;
		uint16_t pn25;
		uint16_t pn5;
		uint16_t pn10;

		bool started = false;
		bool active = false;
		uint16_t oneShotPeriod = 15;

		bool start();
		bool stop();
		bool update();
		int16_t oneShot(uint16_t period);
		bool reset();
};

// VOC ans ECO2 - CCS811
class Sck_CCS811
{
	// TODO review  the utility of baseline on datasheet and implement cotrol interface if needed
	// TODO check consumption and quality in different drive modes: 1 sec [default], 10 sec, 60 sec or 0.25 sec (RAW mode)

	public:
		const byte address = 0x5a;
		bool start();
		bool stop();
		bool getReading(SckBase *base);
		uint16_t getBaseline();
		bool setBaseline(uint16_t wichBaseline);

		bool compensate = true; 	// Compensation is for both sensors or none
		float VOCgas;
		float ECO2gas;
	private:

		//Mode 0 = Idle
		//Mode 1 = read every 1s
		//Mode 2 = every 10s
		//Mode 3 = every 60s
		//Mode 4 = RAW mode
		const uint8_t driveMode = 3;

		uint32_t startTime = 0;
		uint32_t lastReadingMill = 0;
		const uint32_t warmingTime = 300000; 	// Minimal time for sensor stabilization in ms (the kit will not return readings during this period) 5 minutes as default
		bool alreadyStarted = false;
		CCS811 ccs = CCS811(address);
};


class SckUrban
{
	private:
		RTCZero* rtc;
	public:
		SckUrban(RTCZero* myrtc) {
			rtc = myrtc;
		}

		bool setup();
		bool start(SensorType wichSensor);
		bool stop(SensorType wichSensor);

		// String getReading(); https://stackoverflow.com/questions/14840173/c-same-function-parameters-with-different-return-type
		void getReading(SckBase *base, OneSensor *wichSensor);
		bool control(SckBase *base, SensorType wichSensor, String command);

		// Light
		Sck_BH1721FVC sck_bh1721fvc;

		// Temperature and Humidity
		Sck_SHT31 sck_sht31 = Sck_SHT31(&Wire);

		// Noise
		Sck_Noise sck_noise;

		// Barometric pressure and Altitude
		Sck_MPL3115A2 sck_mpl3115A2;

		// VOC and ECO2
		Sck_CCS811 sck_ccs811;

		// PM sensor
		Sck_PM sck_pm = Sck_PM(rtc);
};

