#pragma once

#include <Metrics.h>

enum DeviceType
{
	// DEVICE_INA219,
	// DEVICE_VL6180,
	DEVICE_BH1730,
	DEVICE_ADS1X15,
	DEVICE_SCD30,

	DEVICE_COUNT
};

struct DeviceInfo
{
	const DeviceType type;
	const char * name;
	const byte addressList[8];
	const uint8_t metricNum;
	const Metric * metricList[];
};

static DeviceInfo sck_bh1730 =
{
	DEVICE_BH1730, 		// DeviceType
	"BH1730",			// Name
	{ 0x29 }, 			// I2C address list (8 max)
	1, 					// Number of provided metrics
	{ &Light }			// Metrics list
};

static DeviceInfo sck_scd30 =
{
	DEVICE_SCD30,
	"SCD30",
	{ 0x61 },
	3,
	{ &Co2, &Temperature, &Humidity }
};

static DeviceInfo sck_ads1x15 =
{
	DEVICE_ADS1X15,
	"ADS1X15",
	{ 0x48, 0x49, 0x4A, 0x4B },
	4,
	{ &Voltage, &Voltage, &Voltage, &Voltage }
};
