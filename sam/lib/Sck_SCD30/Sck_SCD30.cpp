#include "Sck_SCD30.h"

bool Ctrl_SCD30::start(TwoWire * _wire, byte address)
{
	if (started) return true;

	devAddr = address;
	wireBus = _wire;

	if (debug) sparkfun_scd30.enableDebugging(SerialUSB);

	// Without this delay sensor init fails sometimes
	delay(500);

	// Unset measbegin option to avoid begin() function to set measuring interval to default value of 2 seconds.
	if (!sparkfun_scd30.begin(*wireBus, false, false)) return false;

	// Start measuring with this function respects the saved interval
	if (!sparkfun_scd30.beginMeasuring()) return false;

	started = true;
	return true;
}

bool Ctrl_SCD30::stop()
{
	sparkfun_scd30.StopMeasurement();

	started = false;
	return true;
}

int8_t Ctrl_SCD30::getReading(Metric metric, char * buff)
{
	float result;

	switch (metric.type)
	{
		case SENSOR_SCD30_CO2:
			result = sparkfun_scd30.getCO2();
			break;

		case SENSOR_SCD30_TEMP:
			result = sparkfun_scd30.getTemperature();
			break;

		case SENSOR_SCD30_HUM:
			result = sparkfun_scd30.getHumidity();
			break;

		default:
			return -1;
	}

	snprintf(buff, READING_BUFF_SIZE, "%.*f", metric.precision, result);
	return 0;
}

uint16_t Ctrl_SCD30::interval(uint16_t newInterval)
{
	// Even if the sensor responds OK it doesn't seems to accept any value grater than 1000
	if (newInterval >= 2 && newInterval <= 1800) sparkfun_scd30.setMeasurementInterval(newInterval);

	uint16_t currentInterval;
	sparkfun_scd30.getMeasurementInterval(&currentInterval);

	// Restart measuring so we don't need to wait the current interval to finish (useful when you come from very long intervals)
	sparkfun_scd30.StopMeasurement();
	sparkfun_scd30.beginMeasuring();

	return currentInterval;
}

bool Ctrl_SCD30::autoSelfCal(int8_t value)
{
	// Value: 0 -> disable, 1 -> enable, any other -> get current setting

	if (value == 1)	sparkfun_scd30.setAutoSelfCalibration(true);
	else if (value == 0) sparkfun_scd30.setAutoSelfCalibration(false);

	return sparkfun_scd30.getAutoSelfCalibration();
}

uint16_t Ctrl_SCD30::forcedRecalFactor(uint16_t newFactor)
{
	if (newFactor >= 400 && newFactor <= 2000) {
		// Maybe not needed, but done for safety
		sparkfun_scd30.setAutoSelfCalibration(false);
		// Send command to SCD30
		sparkfun_scd30.setForcedRecalibrationFactor(newFactor);
	}
	uint16_t saved_value = 0;
	// Check saved value
	sparkfun_scd30.getForcedRecalibration(&saved_value);
	return saved_value;
}

float Ctrl_SCD30::tempOffset(float userTemp, bool off)
{
	// We expect from user the REAL temperature measured during calibration
	// We calculate the difference against the sensor measured temperature to set the correct offset. Please wait for sensor to stabilize temperatures before aplying an offset.
	// Temperature offset should always be positive (the sensor is generating heat)

	uint16_t currentOffsetTemp;
	sparkfun_scd30.getTemperatureOffset(&currentOffsetTemp);

	float temperature = sparkfun_scd30.getTemperature();

	if (temperature > userTemp) sparkfun_scd30.setTemperatureOffset(temperature - userTemp);
	else if (off) sparkfun_scd30.setTemperatureOffset(0);

	sparkfun_scd30.getTemperatureOffset(&currentOffsetTemp);

	return currentOffsetTemp / 100.0;
}

