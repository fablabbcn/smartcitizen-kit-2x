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
