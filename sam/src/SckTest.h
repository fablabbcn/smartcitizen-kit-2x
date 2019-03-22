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
		bool test_VOC();
		bool test_Noise();
		bool test_PM();
		bool test_auxWire();

		bool connect_ESP();
		bool publishResult();

		enum ButtonLed_sate { TEST_BLUE, TEST_RED, TEST_GREEN, TEST_FINISHED };
		volatile ButtonLed_sate butLedState = TEST_BLUE;

		enum Test_type {
			TEST_BATT_VOLT,			// 0 bool
			TEST_BATT_CHG, 			// 1 bool
			TEST_SD, 			// 2 bool
			TEST_FLASH, 			// 3 bool
			TEST_USER, 			// 4 (button) bool
			TEST_TEMP, 			// 5 C
			TEST_HUM, 			// 6 percent
			TEST_LIGHT, 			// 7 Lux
			TEST_PRESS, 			// 8 kPa
			TEST_VOCS, 			// 9 ppb
			TEST_ECO2, 			// 10 ppm
			TEST_NOISE, 			// 11 dbA
			TEST_PM_1, 			// 12 ug/m3
			TEST_PM_25, 			// 13 ug/m3
			TEST_PM_10, 			// 14 ug/m3
			TEST_AUXWIRE, 			// 15 bool
			TEST_WIFI_TIME, 		// 16 seconds

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
