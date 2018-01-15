#include "SckUrban.h"

bool SckUrban::setup() {

	// TODO implementar una prueba de deteccion y si falla retornar falso.

	// Light

	// Temperature and Humidity
	sht31.begin();
	
	// Gases (MICS)

	// Temporal, hay que integrarlo en el driver del MICS
	// to protect MICS turn off heaters (HIGH=off, LOW=on)
	pinMode(pinPWM_HEATER_CO, OUTPUT);
	digitalWrite(pinPWM_HEATER_CO, LOW);
	pinMode(pinPWM_HEATER_NO2, OUTPUT);
	digitalWrite(pinPWM_HEATER_NO2, LOW);

	pinMode(pinREAD_CO, INPUT);
	pinMode(pinREAD_NO2, INPUT);


	
	// Noise

	// Barometric Pressure
	
	// Dust Particles



	return true;
};

float SckUrban::getTemperature() {

	return sht31.readTemperature();
}

float SckUrban::getHumidity() {

	return sht31.readHumidity();
}