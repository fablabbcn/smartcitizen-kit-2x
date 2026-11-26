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
// The WDT keeps running in standby, so the Early Warning interrupt also wakes
// the kit every 8 seconds while it sleeps. SckBase::goToSleep() suspends the
// WDT and goes back to sleep when that was the only reason to wake up.

#define SCK_WDT_TIMEOUT_S 60        // Approximate reset target: the reset comes 56-64 s after the last feed() (EW granularity), so feed well within that
#define SCK_WDT_EW_PERIOD_S 8       // Early Warning interrupt period (seconds)
#define SCK_WDT_GCLK 5              // Generic clock generator used by the WDT

class SckWatchdog
    {
    public:
        // Info about the place where the code was stuck (WDT) or crashed (HardFault) before the last reset
        enum HangType { HANG_NONE, HANG_WDT, HANG_FAULT };
        struct HangInfo { bool valid; HangType type; uint32_t pc; uint32_t lr; };

        void setup();                   // Configure and start the WDT
        void enable();
        void disable();
        void feed();                    // Tell the WDT the main loop is alive
        void suspend();                 // Keep the WDT alive during known long blocking operations (ex. flash chip erase)
        void resume();
        bool enabled();
        bool suspended();
        uint32_t lastFeed = 0;          // millis() of the last feed()
        volatile bool ewWakeup = false; // Set by the Early Warning interrupt (it also wakes the kit from standby)
        HangInfo lastHang();            // Valid only if the last reset was caused by the WDT or a HardFault
        void earlyWarning(uint32_t *stackFrame);    // Called from WDT interrupt
        void hardFault(uint32_t *stackFrame);       // Called from HardFault handler

    private:
        volatile uint8_t _ewCounter = 0;
        volatile bool _suspended = false;
        bool _enabled = false;
        HangInfo _lastHang = { false, HANG_NONE, 0, 0 };
        const uint8_t _ewLimit = SCK_WDT_TIMEOUT_S / SCK_WDT_EW_PERIOD_S;
        void _sync();
    };

extern SckWatchdog wdt;
