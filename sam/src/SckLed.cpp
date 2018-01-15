#include "SckLed.h"

void SckLed::setup() {
	pinMode(pinRED, OUTPUT);
	pinMode(pinGREEN, OUTPUT);
	pinMode(pinBLUE, OUTPUT);

	setRGBColor(colors[GREEN]);
}
void SckLed::update(ColorName colorName, pulseModes pulse) {

	pulseMode = pulse;

	if (pulse != PULSE_STATIC) {
		if (colorName == RED) currentPulse = pulseRed;
		else if (colorName == BLUE) currentPulse = pulseBlue;
		else if (colorName == PINK) currentPulse = pulsePink;
		configureTimer5(refreshPeriod);
	} else {
		ledColor = colors[colorName];
		disableTimer5();
	}
	tick();
}
void SckLed::tick() {

	uint16_t ledInterval = 0;

	switch(pulseMode) {
		case PULSE_SOFT:{

			ledColor = *(currentPulse + colorIndex);

			if (colorIndex == 24) nextIndex = -1;
			else if (colorIndex == 0) nextIndex = 1;
			colorIndex += nextIndex;
			
			if (charging) {
				if (colorIndex == 0 || colorIndex == 1) ledColor = colors[ORANGE];

			} else if (batFull) {
				if (colorIndex == 0 || colorIndex == 1) ledColor = colors[GREEN];
			
			} else if (lowBatt) {
				if (colorIndex == 1 || colorIndex == 3) ledColor = colors[ORANGE];
				else ledColor = colors[BLACK];
			}
			break;
		} 
		case PULSE_HARD_SLOW: ledInterval = 300;
		case PULSE_HARD_FAST: ledInterval = 80;
		case PULSE_STATIC: break;
		default: {
			if (millis() - hardTimer > ledInterval) {
				if (nextIndex == 1) ledColor = currentPulse[24];
				else ledColor = colors[BLACK];
				hardTimer = millis();
				nextIndex *= -1;
			}
		}
	}

	setRGBColor(ledColor);
}
void SckLed::setRGBColor(Color myColor) {

	if (myColor.r == 0) {
		pinMode(pinRED, OUTPUT);
		digitalWrite(pinRED, HIGH);
	} else analogWrite(pinRED, 255 - myColor.r);
	
	if (myColor.g == 0) {
		pinMode(pinGREEN, OUTPUT);
		digitalWrite(pinGREEN, HIGH);
	} else analogWrite(pinGREEN, 255 - myColor.g);

	if (myColor.b == 0) {
		pinMode(pinBLUE, OUTPUT);
		digitalWrite(pinBLUE, HIGH);
	} else analogWrite(pinBLUE, 255 - myColor.b);
}
void SckLed::off() {
	disableTimer5();
	
	colorIndex = 0;
	ledColor = colors[BLACK];
	pulseMode = PULSE_STATIC;

	pinMode(pinRED, OUTPUT);
	pinMode(pinGREEN, OUTPUT);
	pinMode(pinBLUE, OUTPUT);
	
	digitalWrite(pinRED, HIGH);
	digitalWrite(pinGREEN, HIGH);
	digitalWrite(pinBLUE, HIGH);
}
void SckLed::configureTimer5(uint16_t periodMS) {

	if (!timer5_Runing) {

		// clock the timer with the core cpu clock (48MHz)
		GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
		while(GCLK->STATUS.bit.SYNCBUSY);

		// Reset the TC
		TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		while(TC5->COUNT16.CTRLA.bit.SWRST);

		// Set Timer counter Mode to 16 bits
		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

		// Set TC5 mode as match frequency
		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 | TC_CTRLA_ENABLE;

		TC5->COUNT16.CC[0].reg = uint16_t(46.875 * periodMS);

		// TC5->COUNT16.CC[0].reg = (uint16_t) 187; //0.5us = 240 clock cycle at 48MHz (core clock)
													// 4ms = 187 clock cycle at 46875
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		// Configure interrupt request
		NVIC_DisableIRQ(TC5_IRQn);
		NVIC_ClearPendingIRQ(TC5_IRQn);
		NVIC_SetPriority(TC5_IRQn, 2); //you can change priority between 0 (Highest priority) and 2 (lowest)
		NVIC_EnableIRQ(TC5_IRQn);

		// Enable the TC5 interrupt request
		TC5->COUNT16.INTENSET.bit.MC0 = 1;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		
		//enable the counter (from now your getting interrupt)
		TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);

		timer5_Runing = true;
	}
}
void SckLed::disableTimer5() {

	if(timer5_Runing) {
		//use this to disable the counter :
		TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		timer5_Runing = false;
	}
}