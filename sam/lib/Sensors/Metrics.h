#pragma once

enum MetricType
{
	METRIC_TEMPERATURE,
	METRIC_HUMIDITY,
	METRIC_LIGHT,
	METRIC_CO2,
	METRIC_VOLTAGE,
	METRIC_CURRENT,
	METRIC_ALTITUDE,
	METRIC_PRESSURE,
	METRIC_VOCS,
	METRIC_ECO2,
	METRIC_NOISE_DBA,
	METRIC_NOISE_DBC,
	METRIC_NOISE_DBZ,
	METRIC_NOISE_FFT,
	METRIC_PM1,
	METRIC_PM25,
	METRIC_PM10,
	METRIC_PN03,
	METRIC_PN05,
	METRIC_PN1,
	METRIC_PN25,
	METRIC_PN5,
	METRIC_PN10,
	METRIC_PH,
	METRIC_CONDUCTIVITY,
	METRIC_TDS,
	METRIC_SALINITY,
	METRIC_SG,
	METRIC_DO,
	METRIC_DOSAT,
	METRIC_MOISTURE,
	METRIC_DISTANCE,
	METRIC_GPS_FIX,
	METRIC_LATITUDE,
	METRIC_LONGITUDE,
	METRIC_SPEED,
	METRIC_GPS_HDOP,
	METRIC_GPS_SATNUM,

	METRIC_COUNT
};

struct Metric
{
	const MetricType type;
	const char *name;
	const char *shortName;
	const char *unit;
	const uint8_t precision;
	const uint8_t id;
};

struct MetricConfig
{

	bool enabled = true;
	// uint8_t priority = 100;
	uint8_t intervals = 1;
	uint8_t precision = 2;
	// bool oled_display -> Move this to OLED Device (default show all and create a disable list to add manually disabled ones)
};

static Metric Light = {
	METRIC_LIGHT, 	// MetricType
	"Light",		// name
	"LIGHT", 		// short name
	"Lux", 			// unit
	0,				// Precision (number of decimal digits)
	14 				// platform id
};
static Metric Co2 = {
	METRIC_CO2,
	"Carbon Dioxide",
	"CO2",
	"ppm",
	0,
	158
};
static Metric Temperature = {
	METRIC_TEMPERATURE,
	"Temperature",
	"TEMP",
	"C",
	2,
	35
};
static Metric Humidity = {
	METRIC_HUMIDITY,
	"Humidity",
	"HUM",
	"%",
	2,
	67
};
static Metric Voltage = {
	METRIC_VOLTAGE, 
	"Voltage",
	"VOLT",
	"V",
	6,
	133
};
static Metric Current = {
	METRIC_CURRENT, 
	"Current",
	"CURR",
	"A",
	6,
	0
};
static Metric NoiseDBA = {
	METRIC_NOISE_DBA,
	"Noise dBA",
	"DBA",
	"dBA",
	2,
	53
};
static Metric NoiseDBC = {
	METRIC_NOISE_DBC,
	"Noise dBC",
	"DBC",
	"dBC",
	2,
	0
};
static Metric NoiseDBZ = {
	METRIC_NOISE_DBZ, 
	"Noise dBZ",
	"DBZ",
	"dB",
	2,
 	0	
};
static Metric NoiseFFT = {
	METRIC_NOISE_FFT,
	"Noise FFT",
	"FFT",
	"",
	0,
	0
};
static Metric Altitude = {
	METRIC_ALTITUDE,
	"Altitude",
	"ALT",
	"M",
	2,
	0
};
static Metric Pressure = {
	METRIC_PRESSURE,
	"Barometric Pressure",
	"PRESS",
	"M",
	2,
	58
};
static Metric VOCs = {
	METRIC_VOCS,
	"VOC Gas",
	"VOCS",
	"ppb",
	2,
	113
};
static Metric eCO2 = {
	METRIC_ECO2,
	"eCO2 Gas",
	"ECO2",
	"ppm",
	2,
	112
};
static Metric Pm1 = {
	METRIC_PM1,
	"PM 1.0",
	"PM1",
	"ug/m3",
	0,
	89
};
static Metric Pm25 = {
	METRIC_PM25,
	"PM 2.5",
	"PM25",
	"ug/m3",
	0,
	87
};
static Metric Pm10 = {
	METRIC_PM10,
	"PM 10.0",
	"PM10",
	"ug/m3",
	0,
	88
};
static Metric Pn03 = {
	METRIC_PN03,
	"PN 0.3",
	"PN03",
	"#/0.1l",
	0,
	0
};
static Metric Pn05 = {
	METRIC_PN05,
	"PN 0.5",
	"PN05",
	"#/0.1l",
	0,
	0
};
static Metric Pn1 = {
	METRIC_PN1,
	"PN 1.0",
	"PN1",
	"#/0.1l",
	0,
	0
};
static Metric Pn25 = {
	METRIC_PN25,
	"PN 2.5",
	"PN25",
	"#/0.1l",
	0,
	0
};
static Metric Pn5 = {
	METRIC_PN5,
	"PN 5.0",
	"PN5",
	"#/0.1l",
	0,
	0
};
static Metric Pn10 = {
	METRIC_PN10,
	"PN 10.0",
	"PN10",
	"#/0.1l",
	0,
	0
};
static Metric pH = {
	METRIC_PH,
	"pH",
	"PH",
	"pH",
	2,
	43
};
static Metric Conductivity = {
	METRIC_CONDUCTIVITY,
	"Conductivity",
	"COND",
	"uS/cm",
	2,
	45
};
static Metric TotalDissolvedSolids = {
	METRIC_TDS,
	"Total Dissolved Solids",
	"TDS",
	"ppm",
	2,
	122
};
static Metric Salinity = {
	METRIC_SALINITY,
	"Salinity",
	"SAL",
	"PSU(ppt)",
	2,
	123
};
static Metric SpecificGravity = {
	METRIC_SG,
	"Specific Gravity",
	"SG",
	"",
	2,
	46
};
static Metric DissolvedOxygen = {
	METRIC_DO,
	"Dissolved Oxygen",
	"DO",
	"mg/L",
	2,
	48
};
static Metric DoSaturation = {
	METRIC_DOSAT,
	"DO Saturation",
	"DOSAT",
	"%",
	2,
	49
};
static Metric Moisture = {
	METRIC_MOISTURE,
	"Soil Moisture",
	"MOIS",
	"%",
	2,
	49
};
static Metric Distance = {
	METRIC_DISTANCE,
	"Distance",
	"DIST",
	"mm",
	2,
	98
};
static Metric GpsFix = {
	METRIC_GPS_FIX,
	"GPS Fix Quality",
	"GPS_FIX",
	"",
	0,
	128	
};
static Metric Latitude = {
	METRIC_LATITUDE,
	"Latitude",
	"LAT",
	"Deg",
	4,
	125	
};
static Metric Speed = {
	METRIC_SPEED,
	"Speed",
	"SPEED",
	"m/s",
	2,
	129
};
static Metric GpsHDOP = {
	METRIC_GPS_HDOP,
	"GPS HDOP",
	"GPS_HDOP",
	"",
	2,
	131
};
static Metric GpsSatelliteNumber = {
	METRIC_GPS_SATNUM,
	"GPS Traked Satellites",
	"GPS_SATNUM",
	"",
	0,
	130
};
