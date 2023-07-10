#include "SckLed.h"

void SckLed::setup()
{
    pinMode(pinRED, OUTPUT);
    pinMode(pinGREEN, OUTPUT);
    pinMode(pinBLUE, OUTPUT);

    update(WHITE, PULSE_SOFT, true);
}
void SckLed::update(ColorName colorName, pulseModes pulse, bool force)
{
    // If there is no changes return without doing nothig
    if ( pulse == pulseMode && colorName == ledColor.name && !force) return;

    pulseMode = pulse;
    ledColor = getColor(colorName);
    disableTimer5();

    switch (pulseMode) {
        case PULSE_WARNING:
            blinkON = true;
            configureTimer5(slowInterval);
            break;
        case PULSE_ERROR:
            blinkON = true;
            configureTimer5(fastInterval);
            break;
        case PULSE_SOFT:
            colorIndex = 0;
            diffColor = getDiff(colorName);
            configureTimer5(refreshPeriod);
            break;
    }

    tick();
}
void SckLed::tick()
{
    Color pulseColor;
    bool digitalOFF = false;

    switch(pulseMode) {
        case PULSE_SOFT:

            // Calculate new color
            pulseColor.r = (uint8_t)(diffColor.r * colorIndex);
            pulseColor.g = (uint8_t)(diffColor.g * colorIndex);
            pulseColor.b = (uint8_t)(diffColor.b * colorIndex);

            // Charge indicator blink if it is the case
            if (chargeStatus == CHARGE_CHARGING) {
                if (colorIndex >= 0 && colorIndex <= 2) pulseColor = getColor(ORANGE);
            } else if (chargeStatus == CHARGE_FINISHED) {
                if (colorIndex >= 0 && colorIndex <= 2) pulseColor = getColor(GREEN);
            } else if (chargeStatus == CHARGE_LOW) {
                if (colorIndex >= 6 && colorIndex <= 8) pulseColor = getColor(ORANGE);
                else if (colorIndex >= 11 && colorIndex <= 13) pulseColor = getColor(ORANGE);
                else if (colorIndex >= 16 && colorIndex <= 18) pulseColor = getColor(ORANGE);
                else pulseColor = multiply(ledColor, 0.05);
            }

            // Move to new index
            if (colorIndex == 24)       direction = -1;
            else if (colorIndex == 0)   direction = 1;
            colorIndex += direction;
            break;

        case PULSE_STATIC:
            blinkON = true;
        case PULSE_WARNING:
        case PULSE_ERROR:
            if (blinkON) {
                pulseColor = ledColor;
            } else {
                if (pulseMode == PULSE_WARNING) pulseColor = multiply(ledColor, 0.1);
                else pulseColor = multiply(ledColor, 0);
            }
            blinkON = !blinkON;
            break;
    }

    // apply changes
    show(pulseColor);
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
void SckLed::show(Color color)
{
    Color c;
    c.r = (uint8_t)((color.r * brightness) / 100);
    c.g = (uint8_t)((color.g * brightness) / 100);
    c.b = (uint8_t)((color.b * brightness) / 100);

    if (c.r == 0) {
        pinMode(pinRED, OUTPUT);
        digitalWrite(pinRED, HIGH);
    } else analogWrite(pinRED,     255 - c.r);

    if ( c.g == 0) {
        pinMode(pinGREEN, OUTPUT);
        digitalWrite(pinGREEN, HIGH);
    } else analogWrite(pinGREEN,   255 - c.g);


    if (c.b == 0) {
        pinMode(pinBLUE, OUTPUT);
        digitalWrite(pinBLUE, HIGH);
    } else analogWrite(pinBLUE,    255 - c.b);
}
SckLed::Color SckLed::multiply(Color wichColor, float mult)
{
    Color result;
    result.r = wichColor.r * mult;
    result.g = wichColor.g * mult;
    result.b = wichColor.b * mult;
    return result;
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
