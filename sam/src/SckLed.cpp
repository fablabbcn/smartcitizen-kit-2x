#include "SckLed.h"


void SckLed::setup()
{
	pinMode(pinRED, OUTPUT);
	pinMode(pinGREEN, OUTPUT);
	pinMode(pinBLUE, OUTPUT);

	ledColor = colors[WHITE];
	pulseMode = PULSE_STATIC;
	tick();
}
void SckLed::update(ColorName colorName, pulseModes pulse, bool force)
{
	
	if (pulse == pulseMode && colorName == ledColor.name && !force) return;

	pulseMode = pulse;
	ledColor = colors[colorName];
	disableTimer5();

	switch (pulseMode) {
		case PULSE_STATIC:
			break;
		case PULSE_HARD_SLOW:
			blinkON = true;
			configureTimer5(slowInterval);
			break;
		case PULSE_HARD_FAST:
			blinkON = true;
			configureTimer5(fastInterval);
			break;
		case PULSE_SOFT:
			colorIndex = 0;
			if (colorName == RED) currentPulse = pulseRed;
			else if (colorName == BLUE) currentPulse = pulseBlue;
			else if (colorName == PINK) currentPulse = pulsePink;
			configureTimer5(refreshPeriod);
	}

	tick();
}
void SckLed::tick()
{

	if (pulseMode == PULSE_SOFT) {
		Color c = *(currentPulse + colorIndex);

		if (chargeStatus == CHARGE_CHARGING) {
			if (colorIndex >= 0 && colorIndex <= 2) c = colors[ORANGE];
		} else if (chargeStatus == CHARGE_FINISHED) {
			if (colorIndex >= 0 && colorIndex <= 2) c = colors[GREEN];
		} else if (chargeStatus == CHARGE_LOW) {
			if (colorIndex >= 6 && colorIndex <= 8) c = colors[ORANGE];
			else if (colorIndex >= 11 && colorIndex <= 13) c = colors[ORANGE];
			else if (colorIndex >= 16 && colorIndex <= 18) c = colors[ORANGE];
			else c = *(currentPulse + 24);
		}

		analogWrite(pinRED, 255 - c.r);
		analogWrite(pinGREEN, 255 - c.g);
		analogWrite(pinBLUE, 255 - c.b);
		if (colorIndex == 24) direction = -1;
		else if (colorIndex == 0) direction = 1;
		colorIndex += direction; 
	} else if (pulseMode == PULSE_STATIC) { 
		analogWrite(pinRED, 255 - ledColor.r);
		analogWrite(pinGREEN, 255 - ledColor.g);
		analogWrite(pinBLUE, 255 - ledColor.b);
	} else {
		if (blinkON) {
			analogWrite(pinRED, 255 - ledColor.r);
			analogWrite(pinGREEN, 255 - ledColor.g);
			analogWrite(pinBLUE, 255 - ledColor.b);
		} else {
			pinMode(pinRED, OUTPUT);
			pinMode(pinGREEN, OUTPUT);
			pinMode(pinBLUE, OUTPUT);
			digitalWrite(pinRED, 1);
			digitalWrite(pinGREEN, 1);
			digitalWrite(pinBLUE, 1);
		}
		blinkON = !blinkON;
	}
}
void SckLed::off()
{
	disableTimer5();

	colorIndex = 0;
	pulseMode = PULSE_STATIC;

	pinMode(pinRED, OUTPUT);
	pinMode(pinGREEN, OUTPUT);
	pinMode(pinBLUE, OUTPUT);

	digitalWrite(pinRED, HIGH);
	digitalWrite(pinGREEN, HIGH);
	digitalWrite(pinBLUE, HIGH);
}
void SckLed::configureTimer5(uint16_t periodMS)
{

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
void SckLed::disableTimer5()
{

	if(timer5_Runing) {
		//use this to disable the counter :
		TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while(TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);
		timer5_Runing = false;
	}
}
