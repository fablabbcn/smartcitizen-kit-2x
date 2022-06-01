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
// * Light - BH1730FVC -> (0x29)
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
class Sck_BH1730FVC
{
	// Datasheet
	// https://www.mouser.es/datasheet/2/348/bh1730fvc-e-1018573.pdf

	private:
		bool updateValues();
		
		// Config values
		uint8_t ITIME; 			// Integration Time (datasheet page 9)
		float ITIME_ms;
		const uint8_t ITIME_max = 252;
		const uint8_t ITIME_min = 1;
		const uint16_t goUp = 32768; 	// On the high part
		float Tmt; 			// Measurement time (datasheet page 9)
		const float Tint = 2.8; 	// Internal Clock Period (datasheet page 4 --> 2.8 typ -- 4.0 max)
		uint8_t Gain = 1;

		float DATA0; 			// Visible Light
		float DATA1; 			// Infrared Light
	public:
		bool debug = false;
		uint8_t address = 0x29;
		int reading;
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

		uint32_t timeout = 100;	// Time in ms to wait for a reading
		uint8_t retrys = 3;
		bool debug = false;
		bool update();
		bool sendComm(uint16_t comm);
		uint8_t crc8(const uint8_t *data, int len);
	public:
		uint8_t address;
		// Conntructor
		Sck_SHT31(TwoWire *localWire, uint8_t customAddress=0x44);

		float temperature;
		float humidity;
		bool start();
		bool stop();
		bool getReading();
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
		uint32_t lastReading = 0;

		static const uint8_t buffLong = 31; // + Start_byte_1 = 32
		uint8_t buffer[buffLong];

		const byte PM_START_BYTE_1 		= 0x42;
		const byte PM_START_BYTE_2 		= 0x4d;

		const byte PM_CMD_GET_PASSIVE_READING 	= 0xe2;
		const byte PM_CMD_CHANGE_MODE 		= 0xe1;
		const byte PM_CMD_SLEEP_ACTION 		= 0xe4;

		const byte PM_MODE_PASSIVE 		= 0x00;
		const byte PM_MODE_ACTIVE 		= 0x01;
		const byte PM_SLEEP 			= 0x00;
		const byte PM_WAKEUP 			= 0x01;

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

		RTCZero* rtc;
		bool started = false;
		uint32_t wakeUpTime = 0;
		uint8_t retries = 0;
		const uint8_t MAX_RETRIES = 3;
		bool oldSensor = false;

		bool fillBuffer();
		bool processBuffer();
		bool sendCmd(byte cmd, byte data=0x00, bool checkResponse=true);
		bool sleep();
		bool wake();

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

		// Other data provided by the sensor
		uint8_t version = 0;
		uint8_t errorCode; 	// Not documented on Datasheet

		// Default values for this are in config.h 
		bool powerSave; 	// If true the sensor will be turned off between readings.
		uint32_t warmUpPeriod; 	// Wait this time (sec) before taking readings to let sensor stabilize

		bool start();
		bool stop();
		bool getReading(OneSensor *wichSensor, SckBase *base);
		bool debug = false;
		bool monitor = false;
};

// VOC ans ECO2 - CCS811
class Sck_CCS811
{
	// Datasheet https://ams.com/documents/20143/36005/CCS811_DS000459_7-00.pdf/3cfdaea5-b602-fe28-1a14-18776b61a35a
	// TODO review  the utility of baseline on datasheet and implement control interface if needed
	// TODO check consumption and quality in different drive modes: 1 sec [default], 10 sec, 60 sec or 0.25 sec (RAW mode)

	public:
		Sck_CCS811(RTCZero* myrtc) {
			rtc = myrtc;
		}

		const byte address = 0x5a;
		bool start();
		bool stop();
		bool getReading(SckBase *base);
		uint16_t getBaseline();
		bool setBaseline(uint16_t wichBaseline);
		bool setDriveMode(uint8_t wichDrivemode);

		//Mode 0 = Idle
		//Mode 1 = read every 1s
		//Mode 2 = every 10s
		//Mode 3 = every 60s
		//Mode 4 = RAW mode
		uint8_t driveMode = 3;

		bool debug = false;
		bool compensate = true; 	// Compensation is for both sensors or none
		float VOCgas;
		float ECO2gas;
	private:
		uint32_t startTime = 0;
		uint32_t lastReadingMill = 0;
		const uint32_t warmingTime = 300; 	// Minimal time for sensor stabilization in seconds(the kit will not return readings during this period) 5 minutes as default
		bool alreadyStarted = false;
		CCS811 ccs = CCS811(address);
		RTCZero* rtc;
};


class SckUrban
{
	private:
		RTCZero* rtc;
	public:
		SckUrban(RTCZero* myrtc) {
			rtc = myrtc;
		}

		bool present();
		bool start(SensorType wichSensor);
		bool stop(SensorType wichSensor);

		// String getReading(); https://stackoverflow.com/questions/14840173/c-same-function-parameters-with-different-return-type
		void getReading(SckBase *base, OneSensor *wichSensor);
		bool control(SckBase *base, SensorType wichSensor, String command);

		// Light
		Sck_BH1730FVC sck_bh1730fvc;

		// Temperature and Humidity
		Sck_SHT31 sck_sht31 = Sck_SHT31(&Wire);

		// Noise
		Sck_Noise sck_noise;

		// Barometric pressure and Altitude
		Sck_MPL3115A2 sck_mpl3115A2;

		// VOC and ECO2
		Sck_CCS811 sck_ccs811 = Sck_CCS811(rtc);

		// PM sensor
		Sck_PM sck_pm = Sck_PM(rtc);
};
