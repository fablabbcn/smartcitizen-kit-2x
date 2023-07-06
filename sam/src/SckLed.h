#pragma once

#include <Arduino.h>
#include "Pins.h"

class SckLed
    {
    public:

        enum ColorName {
            BLUE = 0,
            RED = 1,
            PINK = 2,
            WHITE = 3,
            GREEN,
            YELLOW,
            ORANGE,
            RED2,
            BLUE2,
            PINK2,
            COLOR_COUNT
        };

        struct Color { uint8_t r; uint8_t g; uint8_t b; ColorName name; };
 
        const Color colors[COLOR_COUNT] = {
            { 0,     29,     225,    BLUE    },
            { 250,   4,      0,      RED     },
            { 129,   12,     112,    PINK    },
            { 254,   254,    254,    WHITE   },
            { 0,     254,    0,      GREEN   },
            { 154,   100,    0,      YELLOW  },
            { 235,   30,     0,      ORANGE  },
            { 255,   10,     0,      RED2    },
            { 0,     39,     255,    BLUE2   },
            { 149,   22,     132,    PINK2   }
        };
        Color getColor(ColorName wichName) {
            for (uint8_t i=0; i<COLOR_COUNT; i++) if (colors[i].name == wichName) return colors[i];
        }

        // Color fade values
        struct Color_float { float r; float g; float b; ColorName name; };
        const Color_float diff[4] = {
            { 0.0,    1.16,   9.0,  BLUE    },
            { 10.0,   0.16,   0.0,  RED     },
            { 5.16,   0.48,   4.48, PINK    },
            { 10.0,   10.0,   10.0, WHITE   } 
        };
        Color_float getDiff(ColorName wichName) {
            for (uint8_t i=0; i<4; i++) if (diff[i].name == wichName) return diff[i];
        }

        enum pulseModes {
            PULSE_SOFT,
            PULSE_WARNING,
            PULSE_ERROR,
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
        uint8_t brightness = 100;

    private:

        /* void setRGBColor(Color myColor); */
        void configureTimer5(uint16_t periodMS);
        void disableTimer5();
        Color multiply(Color wichColor, float divider);
        void show(Color color);

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
        Color_float diffColor;
    };
