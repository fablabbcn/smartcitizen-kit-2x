#pragma once

#define SC_TEST_WIFI_SSID "ssid"
#define SC_TEST_WIFI_PASSWD "password"
#define SC_TEST_SHT 1
#define SC_TEST_LIGHT 1
#define SC_TEST_UVA 1
#define SC_TEST_PRESSURE 1
#define SC_TEST_NOISE 1
#define SC_TEST_SEN5X 1
#define SC_TEST_AUXWIRE 1

#include "SckBase.h"
#include "SckSerial.h"

class SckTest
    {
    private:
        SckBase* testBase;

        uint8_t errors = 0;
        bool title = true;

        bool test_user();
        uint8_t test_battery();
        bool test_charger();
        uint8_t test_sdcard();
        uint8_t test_flash();

        // sensors
#if SC_TEST_SHT
        uint8_t test_SHT();
#endif
#if SC_TEST_LIGHT
        uint8_t test_Light();
#endif
#if SC_TEST_UVA
        uint8_t test_UVA();
#endif
#if SC_TEST_PRESSURE
        uint8_t test_Pressure();
#endif
#if SC_TEST_NOISE
        uint8_t test_Noise();
#endif
#if SC_TEST_SEN5X
        uint8_t test_SEN5X();
#endif
#if SC_TEST_AUXWIRE
        uint8_t test_auxWire();
#endif

        bool connect_ESP();
        bool publishResult();

        enum ButtonLed_sate { TEST_BLUE, TEST_RED, TEST_GREEN, TEST_FINISHED };
        volatile ButtonLed_sate butLedState = TEST_BLUE;

        enum Test_type {
            TEST_BATT_VOLT,     // 0 float
            TEST_BATT_CHG,      // 1 bool
            TEST_SD,            // 2 bool
            TEST_FLASH,         // 3 bool
            TEST_USER,          // 4 (button) bool
#if SC_TEST_SHT
            TEST_TEMP,          // 5 C
            TEST_HUM,           // 6 percent
#endif
#if SC_TEST_LIGHT
            TEST_LIGHT,         // 7 Lux
#endif
#if SC_TEST_UVA
            TEST_UVA,           // 8 uW/cm2
#endif
#if SC_TEST_PRESSURE
            TEST_PRESS,         // 9 kPa
#endif
#if SC_TEST_NOISE
            TEST_NOISE,         // 10 dbA
#endif
#if SC_TEST_SEN5X
            TEST_SEN5X,         // 11 ug/m3
#endif
#if SC_TEST_AUXWIRE
            TEST_AUXWIRE,       // 12 bool
#endif
            TEST_WIFI_TIME,     // 13 seconds

            TEST_COUNT
        };

        struct Test_report {
            String time;
            uint32_t id[4];
            String mac;
            float tests[TEST_COUNT];
        };

        Test_report test_report;


    public:

        // Constructor
        SckTest(SckBase* base) {
            testBase = base;
        }

        void test_full();
        void test_button();
    };

// TODO
// SAM firmware upload error reporting
