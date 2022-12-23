#pragma once

// This Id's should be syncronized with the platform 
// Valid ID's 0x01 - 0xff (255 posibilities in 1 byte)
enum MeasurementID
{
	MEASUREMENT_TEMPERATURE = 0x01,
	MEASUREMENT_HUMIDITY = 0x02,
	MEASUREMENT_LIGHT = 0x03,
	MEASUREMENT_CO2 = 0x04,
	MEASUREMENT_VOLTAGE_CH0	= 0x05,
	MEASUREMENT_VOLTAGE_CH1 = 0x06,
	MEASUREMENT_VOLTAGE_CH2 = 0x07,
	MEASUREMENT_VOLTAGE_CH3 = 0x08,
	MEASUREMENT_CURRENT = 0x09,
	MEASUREMENT_ALTITUDE = 0x0a,
	MEASUREMENT_PRESSURE = 0x0b,
	MEASUREMENT_VOCS = 0x0c,
	MEASUREMENT_ECO2 = 0x0d,
	MEASUREMENT_NOISE_DBA = 0x0e,
	MEASUREMENT_NOISE_DBC = 0x0f,
	MEASUREMENT_NOISE_DBZ = 0x10,
	MEASUREMENT_NOISE_FFT = 0x11,
	MEASUREMENT_PM1 = 0x12,
	MEASUREMENT_PM25 = 0x13,
	MEASUREMENT_PM10 = 0x14,
	MEASUREMENT_PN03 = 0x15,
	MEASUREMENT_PN05 = 0x16,
	MEASUREMENT_PN1 = 0x17,
	MEASUREMENT_PN25 = 0x18,
	MEASUREMENT_PN5 = 0x19,
	MEASUREMENT_PN10 = 0x1a,
	MEASUREMENT_PH = 0x1b,
	MEASUREMENT_CONDUCTIVITY = 0x1c,
	MEASUREMENT_TDS = 0x1d,
	MEASUREMENT_SALINITY = 0x1e,
	MEASUREMENT_SG = 0x1f,
	MEASUREMENT_DO = 0x20,
	MEASUREMENT_DOSAT = 0x21,
	MEASUREMENT_MOISTURE = 0x22,
	MEASUREMENT_DISTANCE = 0x23,
	MEASUREMENT_GPS_FIX = 0x24,
	MEASUREMENT_LATITUDE = 0x25,
	MEASUREMENT_LONGITUDE = 0x26,
	MEASUREMENT_SPEED = 0x27,
	MEASUREMENT_GPS_HDOP = 0x29,
	MEASUREMENT_GPS_SATNUM = 0x2a,
};

struct MeasurementConfig
{
	byte hash[4];
		// Check initConfig() on Sensors.cpp for more details on this
		// bit 0:7   - 8 -> SensorID
		// bit 8:15  - 8 -> MeasurementID
		// bit 16:17 - 2 -> I2C bus index
		// bit 18 	 - 1 -> Multiplexer present
		// bit 19:21 - 3 -> This + MUX_BASE_ADDRESS = mux i2c address
		// bit 22:24 - 3 -> Multiplexer channel (0-7)
		// bit 25:27 - 3 -> Sensor I2C address index

	bool enabled = true;
	// uint8_t priority = 100;
	uint8_t intervals = 1;
	uint8_t precision = 2;
	// bool oled_display -> Move this to OLED Device (default show all and create a disable list to add manually disabled ones)
};

struct Measurement
{
	const MeasurementID id;
	const char *name;
	const char *shortName;
	const char *unit;
	const uint8_t precision;
	MeasurementConfig * config;
};

static Measurement Light = {
	MEASUREMENT_LIGHT, 	// MeasurementID
	"Light",			// name
	"LIGHT", 			// short name
	"Lux", 				// unit
	0					// Precision (number of decimal digits)
};
static Measurement Co2 = {
	MEASUREMENT_CO2,
	"Carbon Dioxide",
	"CO2",
	"ppm",
	0
};
static Measurement Temperature = {
	MEASUREMENT_TEMPERATURE,
	"Temperature",
	"TEMP",
	"C",
	2
};
static Measurement Humidity = {
	MEASUREMENT_HUMIDITY,
	"Humidity",
	"HUM",
	"%",
	2
};
static Measurement Voltage_ch0 = {
	MEASUREMENT_VOLTAGE_CH0, 
	"Voltage",
	"VOLT",
	"V",
	6
};
static Measurement Voltage_ch1 = {
	MEASUREMENT_VOLTAGE_CH1, 
	"Voltage",
	"VOLT",
	"V",
	6
};
static Measurement Voltage_ch2 = {
	MEASUREMENT_VOLTAGE_CH2, 
	"Voltage",
	"VOLT",
	"V",
	6
};
static Measurement Voltage_ch3 = {
	MEASUREMENT_VOLTAGE_CH3, 
	"Voltage",
	"VOLT",
	"V",
	6
};
static Measurement Current = {
	MEASUREMENT_CURRENT, 
	"Current",
	"CURR",
	"A",
	6
};
static Measurement NoiseDBA = {
	MEASUREMENT_NOISE_DBA,
	"Noise dBA",
	"DBA",
	"dBA",
	2
};
static Measurement NoiseDBC = {
	MEASUREMENT_NOISE_DBC,
	"Noise dBC",
	"DBC",
	"dBC",
	2
};
static Measurement NoiseDBZ = {
	MEASUREMENT_NOISE_DBZ, 
	"Noise dBZ",
	"DBZ",
	"dB",
	2
};
static Measurement NoiseFFT = {
	MEASUREMENT_NOISE_FFT,
	"Noise FFT",
	"FFT",
	"",
	0
};
static Measurement Altitude = {
	MEASUREMENT_ALTITUDE,
	"Altitude",
	"ALT",
	"M",
	2
};
static Measurement Pressure = {
	MEASUREMENT_PRESSURE,
	"Barometric Pressure",
	"PRESS",
	"M",
	2
};
static Measurement VOCs = {
	MEASUREMENT_VOCS,
	"VOC Gas",
	"VOCS",
	"ppb",
	2
};
static Measurement eCO2 = {
	MEASUREMENT_ECO2,
	"eCO2 Gas",
	"ECO2",
	"ppm",
	2
};
static Measurement Pm1 = {
	MEASUREMENT_PM1,
	"PM 1.0",
	"PM1",
	"ug/m3",
	0
};
static Measurement Pm25 = {
	MEASUREMENT_PM25,
	"PM 2.5",
	"PM25",
	"ug/m3",
	0
};
static Measurement Pm10 = {
	MEASUREMENT_PM10,
	"PM 10.0",
	"PM10",
	"ug/m3",
	0
};
static Measurement Pn03 = {
	MEASUREMENT_PN03,
	"PN 0.3",
	"PN03",
	"#/0.1l",
	0
};
static Measurement Pn05 = {
	MEASUREMENT_PN05,
	"PN 0.5",
	"PN05",
	"#/0.1l",
	0
};
static Measurement Pn1 = {
	MEASUREMENT_PN1,
	"PN 1.0",
	"PN1",
	"#/0.1l",
	0
};
static Measurement Pn25 = {
	MEASUREMENT_PN25,
	"PN 2.5",
	"PN25",
	"#/0.1l",
	0
};
static Measurement Pn5 = {
	MEASUREMENT_PN5,
	"PN 5.0",
	"PN5",
	"#/0.1l",
	0
};
static Measurement Pn10 = {
	MEASUREMENT_PN10,
	"PN 10.0",
	"PN10",
	"#/0.1l",
	0
};
static Measurement pH = {
	MEASUREMENT_PH,
	"pH",
	"PH",
	"pH",
	2
};
static Measurement Conductivity = {
	MEASUREMENT_CONDUCTIVITY,
	"Conductivity",
	"COND",
	"uS/cm",
	2
};
static Measurement TotalDissolvedSolids = {
	MEASUREMENT_TDS,
	"Total Dissolved Solids",
	"TDS",
	"ppm",
	2
};
static Measurement Salinity = {
	MEASUREMENT_SALINITY,
	"Salinity",
	"SAL",
	"PSU(ppt)",
	2
};
static Measurement SpecificGravity = {
	MEASUREMENT_SG,
	"Specific Gravity",
	"SG",
	"",
	2
};
static Measurement DissolvedOxygen = {
	MEASUREMENT_DO,
	"Dissolved Oxygen",
	"DO",
	"mg/L",
	2
};
static Measurement DoSaturation = {
	MEASUREMENT_DOSAT,
	"DO Saturation",
	"DOSAT",
	"%",
	2
};
static Measurement Moisture = {
	MEASUREMENT_MOISTURE,
	"Soil Moisture",
	"MOIS",
	"%",
	2
};
static Measurement Distance = {
	MEASUREMENT_DISTANCE,
	"Distance",
	"DIST",
	"mm",
	2
};
static Measurement GpsFix = {
	MEASUREMENT_GPS_FIX,
	"GPS Fix Quality",
	"GPS_FIX",
	"",
	0
};
static Measurement Latitude = {
	MEASUREMENT_LATITUDE,
	"Latitude",
	"LAT",
	"Deg",
	4
};
static Measurement Speed = {
	MEASUREMENT_SPEED,
	"Speed",
	"SPEED",
	"m/s",
	2
};
static Measurement GpsHDOP = {
	MEASUREMENT_GPS_HDOP,
	"GPS HDOP",
	"GPS_HDOP",
	"",
	2
};
static Measurement GpsSatelliteNumber = {
	MEASUREMENT_GPS_SATNUM,
	"GPS Traked Satellites",
	"GPS_SATNUM",
	"",
	0
};
