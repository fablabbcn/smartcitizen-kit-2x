#pragma once

#include <Arduino.h>

// Watchdog timer
//
// The hardware WDT runs on its own generic clock generator (GCLK5, fed by the
// internal OSCULP32K and divided down to 1024 Hz). GCLK2 must NOT be touched:
// RTCZero uses it (sourced from the 32.768 kHz crystal) to drive the RTC, and
// reconfiguring it to OSCULP32K makes the RTC drift.
//
// The hardware period is 16 seconds (the SAMD21 maximum). The Early Warning
// interrupt fires every 8 seconds and re-arms the hardware WDT as long as the
// main loop keeps calling feed(). This gives a longer, configurable software
// timeout (SCK_WDT_TIMEOUT_S) while still resetting after 16 seconds if
// interrupts get blocked.
//
// The generator does not run in standby, so the WDT is frozen while the kit
// sleeps and resumes counting on wakeup.

#define SCK_WDT_TIMEOUT_S 60        // Main loop must call feed() at least this often (seconds)
#define SCK_WDT_EW_PERIOD_S 8       // Early Warning interrupt period (seconds)
#define SCK_WDT_GCLK 5              // Generic clock generator used by the WDT

class SckWatchdog
    {
    public:
        // Info about the place where the code was stuck before the last WDT reset
        struct HangInfo { bool valid; uint32_t pc; uint32_t lr; };

        void setup();                   // Configure and start the WDT
        void enable();
        void disable();
        void feed();                    // Tell the WDT the main loop is alive
        void suspend();                 // Keep the WDT alive during known long blocking operations (ex. flash chip erase)
        void resume();
        bool enabled();
        bool suspended();
        uint32_t lastFeed = 0;          // millis() of the last feed()
        HangInfo lastHang();            // Valid only if the last reset was caused by the WDT
        void earlyWarning(uint32_t *stackFrame);    // Called from WDT interrupt

    private:
        volatile uint8_t _ewCounter = 0;
        volatile bool _suspended = false;
        bool _enabled = false;
        HangInfo _lastHang = { false, 0, 0 };
        const uint8_t _ewLimit = SCK_WDT_TIMEOUT_S / SCK_WDT_EW_PERIOD_S;
        void _sync();
    };

extern SckWatchdog wdt;
