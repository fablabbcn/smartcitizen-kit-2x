#pragma once

#define TEST_WIFI_SSID "ssid"
#define TEST_WIFI_PASSWD "pass"

#include "SckBase.h"


class SckTest
{
	private:
		SckBase* testBase;

		uint8_t errors = 0;

		bool test_user();
		bool test_battery();
		bool test_charger();
		bool test_sdcard();
		bool test_flash();

		// sensors
		bool test_SHT();
		bool test_Light();
		bool test_Pressure();
		bool test_Noise();
		bool test_PM();
		bool test_auxWire();

		bool connect_ESP();
		bool publishResult();

		enum ButtonLed_sate { TEST_BLUE, TEST_RED, TEST_GREEN, TEST_FINISHED };
		volatile ButtonLed_sate butLedState = TEST_BLUE;

		enum Test_type {
			TEST_BATT_GAUGE, 		// 0 percent
			TEST_BATT_CHG_RATE, 		// 1 mA
			TEST_BATT_CHG, 			// 2 bool
			TEST_SD, 			// 3 bool
			TEST_FLASH, 			// 4 bool
			TEST_USER, 			// 5 (button) bool
			TEST_TEMP, 			// 6 C
			TEST_HUM, 			// 7 percent
			TEST_LIGHT, 			// 8 Lux
			TEST_PRESS, 			// 9 kPa
			TEST_NOISE, 			// 13 dbA
			TEST_PM_1, 			// 14 ug/m3
			TEST_PM_25, 			// 15 ug/m3
			TEST_PM_10, 			// 16 ug/m3
			TEST_AUXWIRE, 			// 17 bool
			TEST_WIFI_TIME, 		// 18 seconds

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
