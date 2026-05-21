#include "SckBase.h"

#ifdef TESTING
#include "SckTest.h"
#endif

SckBase base;

#ifdef TESTING
SckTest sckTest(&base);
#endif

bool reset_pending = false;

// Led update interrupt
void TC5_Handler (void)
{
    base.led.tick();
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

// Button events interrupt
void ISR_button()
{
    base.butState = digitalRead(pinBUTTON);
#ifdef TESTING
    sckTest.test_button();
#else
    base.butFeedback();
#endif
}

// Card detect interrupt.
// Only set the pending flag here — sdDetect() calls sckOut() and may
// trigger SPI transactions (SD debug logging), both of which are unsafe
// to execute from interrupt context. The main loop picks up the flag in
// reviewState() and calls sdDetect() from normal execution context.
void ISR_sdDetect()
{
    base.sdInitPending = true;
}

// Charger / USB interrupt (BQ24259 INT pin, active-low open-drain).
// Fires on VBUS connect/disconnect and fault condition changes.
// The body is intentionally empty: the sole purpose of registering this
// ISR is to allow the EIC to wake the MCU from STANDBY when the charger
// asserts INT. updatePower() / detectUSB() polls the charger IC via I2C
// on every wakeup tick and handles the actual state change.
void ISR_charger() {}

// void ISR_alarm() {
//  base.wakeUp();
// };

void setup()
{

    base.setup();

#ifdef TESTING
    sckTest.test_full();
#endif
}

void loop()
{
    base.update();
    base.inputUpdate();
}
