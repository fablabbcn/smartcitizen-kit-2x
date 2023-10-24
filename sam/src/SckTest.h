#pragma once

#define TEST_WIFI_SSID "ssid"
#define TEST_WIFI_PASSWD "password"

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
        uint8_t test_SHT();
        uint8_t test_Light();
        uint8_t test_UVA();
        uint8_t test_Pressure();
        uint8_t test_Noise();
        uint8_t test_SEN5X();
        uint8_t test_auxWire();

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
            TEST_TEMP,          // 5 C
            TEST_HUM,           // 6 percent
            TEST_LIGHT,         // 7 Lux
            TEST_UVA,           // 8 uW/cm2
            TEST_PRESS,         // 9 kPa
            TEST_NOISE,         // 10 dbA
            TEST_SEN5X,         // 11 ug/m3
            TEST_AUXWIRE,       // 12 bool
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
