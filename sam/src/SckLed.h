#pragma once

#include <Arduino.h>
#include "Pins.h"

class SckLed
{
public:

	enum ColorName {
		RED,
		GREEN,
		BLUE,
		LIGHT_GREEN,
		LIGHT_BLUE,
		PINK,
		YELLOW,
		ORANGE,
		WHITE,
		BLACK,
		RED2,
		BLUE2,
		PINK2,
		COLOR_COUNT
	};
	struct Color { uint8_t r; uint8_t g; uint8_t b; ColorName name; };

	enum pulseModes {
		PULSE_SOFT,
		PULSE_HARD_SLOW,
		PULSE_HARD_FAST,
		PULSE_STATIC
	};

	enum ChargeStatus {
		CHARGE_NULL,
		CHARGE_CHARGING,
		CHARGE_FINISHED,
		CHARGE_LOW,
		CHARGE_EMERGENCY
	};

	void setup();
	void update(ColorName colorName, pulseModes pulse, bool force=false);
	void off();
	void tick();

	// Powerfeedback
	ChargeStatus chargeStatus = CHARGE_NULL;

private:

	/* void setRGBColor(Color myColor); */
	void configureTimer5(uint16_t periodMS);
	void disableTimer5();

	const Color colors[COLOR_COUNT] = {
		{250,	4,	0, 	RED},		// RED
		{0, 	254, 	0, 	GREEN},		// GREEN
		{0, 	29, 	225, 	BLUE},		// BLUE
		{0, 	254, 	50, 	LIGHT_GREEN},	// LIGHT_GREEN
		{0, 	29, 	254, 	LIGHT_BLUE},	// LIGHT_BLUE
		{129, 	12, 	112, 	PINK},		// PINK
		{154, 	100,	0, 	YELLOW},	// YELLOW
		{235, 	30,	0, 	ORANGE},	// ORANGE
		{254,	254,	254, 	WHITE},		// WHITE
		{0,	0,	0, 	BLACK},		// BLACK
		{255,	10,	0, 	RED2},		// RED2
		{0, 	39, 	255, 	BLUE2},		// BLUE2
		{149, 	22, 	132, 	PINK2},		// PINK2
	};

	// Color fades
	const Color pulseBlue[25] PROGMEM = {{0,1,9},{0,2,18},{0,3,27},{0,4,36},{0,5,45},{0,7,54},{0,8,63},{0,9,72},{0,10,81},{0,11,90},{0,13,99},{0,14,108},{0,15,117},{0,16,126},{0,17,135},{0,19,144},{0,20,153},{0,21,162},{0,22,171},{0,23,180},{0,25,189},{0,26,198},{0,27,207},{0,28,216},{0,29,225}};
	const Color pulseRed[25] PROGMEM	= {{10,0,0},{20,0,0},{30,0,0},{40,0,0},{50,0,0},{60,1,0},{70,1,0},{80,1,0},{90,1,0},{100,1,0},{110,2,0},{120,2,0},{130,2,0},{140,2,0},{150,2,0},{160,3,0},{170,3,0},{180,3,0},{190,3,0},{200,3,0},{210,4,0},{220,4,0},{230,4,0},{240,4,0},{250,4,0}};
	const Color pulsePink[25] PROGMEM = {{5,0,4},{10,1,8},{15,1,13},{20,2,17},{25,2,22},{31,3,26},{36,3,31},{41,4,35},{46,4,40},{51,5,44},{57,5,49},{62,6,53},{67,6,58},{72,7,62},{77,7,67},{83,8,71},{88,8,76},{93,9,80},{98,9,85},{103,10,89},{109,10,94},{114,11,98},{119,11,103},{124,12,107},{129,12,112}};

	// Hardware interrupt settings
	const uint8_t refreshPeriod = 40;
	bool timer5_Runing = false;

	// Timer stuff for hard pulses
	const uint16_t slowInterval = 300;
	const uint16_t fastInterval = 150;

	// Manage color an pulseModes
	volatile uint8_t colorIndex = 0;
	volatile bool blinkON;
	volatile int8_t direction = 1;

	volatile pulseModes pulseMode = PULSE_SOFT;
	Color ledColor;
	const Color *currentPulse;
};
