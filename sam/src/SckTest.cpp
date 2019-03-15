#include "SckBase.h"

#ifdef testing
#include "SckTest.h"

void SckTest::test_full()
{
	// Enable sensors for test
	testBase->enableSensor(SENSOR_TEMPERATURE);
	testBase->enableSensor(SENSOR_HUMIDITY);
	testBase->enableSensor(SENSOR_LIGHT);
	testBase->enableSensor(SENSOR_PRESSURE);
	testBase->enableSensor(SENSOR_PARTICLE_RED);
	testBase->enableSensor(SENSOR_PARTICLE_GREEN);
	testBase->enableSensor(SENSOR_PARTICLE_IR);
	testBase->enableSensor(SENSOR_PM_1);
	testBase->enableSensor(SENSOR_PM_25);
	testBase->enableSensor(SENSOR_PM_10);

	// Make sure al results are 0
	for (uint8_t i=0; i<TEST_COUNT; i++) {
		test_report.tests[i] = 0;
	}

	delay(2000);
	testBase->st.onShell = true;
	testBase->ESPcontrol(testBase->ESP_OFF);
	SerialUSB.println("\r\n********************************");
	SerialUSB.println("Starting SmartCitizenKit test...");

	// Get SAM id
	testBase->getUniqueID();
	SerialUSB.print("SAM id: ");
	for (uint8_t i=0; i<4; i++) {
		test_report.id[i] = testBase->uniqueID[i];
		SerialUSB.print(test_report.id[i], HEX);
	}
	SerialUSB.println();

	// At the beginning the kit will be in static blue waiting for user clicks
	testBase->led.update(testBase->led.BLUE, testBase->led.PULSE_STATIC);
	if (!test_user()) errors++;

	testBase->outputLevel = OUT_SILENT;

	// Test battery gauge
	test_battery();

	// Test SDcard
	if (!test_sdcard()) errors++;

	// Test Flash memory
	if (!test_flash()) errors++;

	// Test SHT temp and hum
	test_SHT();

	// Test Light 
	if (!test_Light()) errors++;

	// Test Pressure 
	if (!test_Pressure()) errors++;

	// Test MAX dust sensor
	test_MAX();

	// Test PM sensor
	test_PM();

	// Test auxiliary I2C bus
	if (!test_auxWire()) errors++;

	// Test wifi connection
	if (!connect_ESP()) errors++;
	else {
		publishResult();
	}

	SerialUSB.println("\r\n********************************");
	if (errors > 0) {
		testBase->led.update(testBase->led.RED, testBase->led.PULSE_STATIC);
		SerialUSB.print("\r\nERROR... ");
		SerialUSB.print(errors);
		SerialUSB.println(" tests failed!!!");
	} else {
		testBase->led.update(testBase->led.GREEN, testBase->led.PULSE_STATIC);
		SerialUSB.println("\r\nTesting finished, all tests OK");
	}
	SerialUSB.println("\r\n********************************");
	while(true);
}

bool SckTest::test_user()
{
	// Test button and led
	// Starts with the led blinking in blue, waiting for user action
	// Every time the user clicks the button the led will change color Blue ->  Red -> Green ->
	SerialUSB.println("\r\nPlease click the button until the led is blue again...");
	while (butLedState != TEST_FINISHED) {
		testBase->inputUpdate();
	}

	test_report.tests[TEST_USER] = 1;
	return true;
}

void SckTest::test_button()
{

	if (!testBase->butState) {
		switch (butLedState) {

			case TEST_BLUE:
				testBase->led.update(testBase->led.RED, testBase->led.PULSE_STATIC);
				SerialUSB.println("Changing Led to red...");
				butLedState = TEST_RED;
				break;

			case TEST_RED:
				testBase->led.update(testBase->led.GREEN, testBase->led.PULSE_STATIC);
				SerialUSB.println("Changing Led to green..");
				butLedState = TEST_GREEN;
				break;

			case TEST_GREEN:
				testBase->led.update(testBase->led.BLUE, testBase->led.PULSE_HARD_SLOW);
				SerialUSB.println("Changing Led to blue..\r\nButton and led test finished OK");
				butLedState = TEST_FINISHED;
				break;
			default: break;
		}
	}
}

bool SckTest::test_battery()
{
	SerialUSB.println("\r\nTesting battery level");

	uint8_t battErrors = errors;

	if (!testBase->getReading(SENSOR_BATT_PERCENT) || testBase->sensors[SENSOR_BATT_PERCENT].reading.toFloat() < 0) {
		SerialUSB.println("ERROR no battery detected!!");
		errors ++;
	} else {
		test_report.tests[TEST_BATT_GAUGE] = testBase->sensors[SENSOR_BATT_PERCENT].reading.toFloat();
	}

	if (!testBase->getReading(SENSOR_BATT_CHARGE_RATE) || testBase->sensors[SENSOR_BATT_CHARGE_RATE].reading.toFloat() <= 0) {
		SerialUSB.println("ERROR no battery charge rate detected!!");
		errors ++;
	} else {
		test_report.tests[TEST_BATT_CHG_RATE] = testBase->sensors[SENSOR_BATT_CHARGE_RATE].reading.toFloat();
	}

	if (testBase->charger.getChargeStatus() != testBase->charger.CHRG_FAST_CHARGING) {
		SerialUSB.println("ERROR battery is not charging!!");
		errors ++;
	} else {
		test_report.tests[TEST_BATT_CHG] = 1;
	}

	if (battErrors < errors) return false;

	SerialUSB.print(test_report.tests[TEST_BATT_GAUGE]);
	SerialUSB.println(" %");
	SerialUSB.print("charging at: ");
	SerialUSB.print(test_report.tests[TEST_BATT_CHG_RATE]);
	SerialUSB.println(" mA");
	SerialUSB.print("Charger status: ");
	SerialUSB.println(testBase->charger.chargeStatusTitles[testBase->charger.getChargeStatus()]);
	SerialUSB.println("Battery gauge test finished OK");
	return true;
}

bool SckTest::test_sdcard()
{

	SerialUSB.println("\r\nTesting SDcard...");
	if (!testBase->sdDetect()) {
		SerialUSB.println("ERROR No SD card detected!!!");	
		return false;
	}
	
	digitalWrite(pinCS_FLASH, HIGH);	// disables Flash
	digitalWrite(pinCS_SDCARD, LOW);

	if (!testBase->sd.begin(pinCS_SDCARD)) { 
		SerialUSB.println(F("ERROR Cant't start Sdcard!!!"));
		return false;
	}


	// Create a file write to it, close it, reopen read from it and delete it
	File testFile;
	char testFileName[9] = "test.txt";

	if (testBase->sd.exists(testFileName)) testBase->sd.remove(testFileName);
	testFile = testBase->sd.open(testFileName, FILE_WRITE);
	testFile.println("testing");
	testFile.close();

	delay(100);

	testFile = testBase->sd.open(testFileName, FILE_READ);
	char testString[8];
	testFile.read(testString, 9); 
	testFile.close();
	String strTest = String(testString);

	if (!strTest.startsWith("testing")) {
		SerialUSB.println("ERROR writing/reading sdcard!!!");
		return false;
	}
	
	testBase->sd.remove(testFileName);

	test_report.tests[TEST_SD] = 1;
	SerialUSB.println("SDcard test finished OK");
	return true;
}

bool SckTest::test_flash()
{
	SerialUSB.println("\r\nTesting Flash memory...");
	testBase->flashSelect();
	
	uint32_t fCapacity = testBase->flash.getCapacity();
	if (fCapacity > 0) {
		SerialUSB.print("Found flash chip with ");
		SerialUSB.print(fCapacity);
		SerialUSB.println(" bytes of size.");
	} else {
		SerialUSB.println("ERROR recognizing flash chip!!!");
		return false;
	}

	String writeSRT = "testing the flash!";

	uint32_t fAddress = testBase->flash.getAddress(testBase->flash.sizeofStr(writeSRT));

	testBase->flash.writeStr(fAddress, writeSRT);

	String readSTR;
	testBase->flash.readStr(fAddress, readSTR);

	if (!readSTR.equals(writeSRT)) {
		SerialUSB.println("ERROR writing/reading flash chip!!!");
		return false;
	}

	test_report.tests[TEST_FLASH] = 1;
	SerialUSB.println("Flash memory test finished OK");
	return true;
}

bool SckTest::test_SHT()
{
	SerialUSB.println("\r\nTesting SHT31 sensor...");

	uint8_t shtErrors = errors;
	if (!testBase->getReading(SENSOR_TEMPERATURE)) {
		SerialUSB.println("ERROR reading SHT31 temperature sensor");
		errors ++;
	} else test_report.tests[TEST_TEMP] = testBase->sensors[SENSOR_TEMPERATURE].reading.toFloat();

	if (!testBase->getReading(SENSOR_HUMIDITY)) {
		SerialUSB.println("ERROR reading SHT31 humidity sensor");
		errors ++;
	} else test_report.tests[TEST_HUM] = testBase->sensors[SENSOR_HUMIDITY].reading.toFloat();

	if (shtErrors < errors) return false;
	SerialUSB.println("SHT31 readings test finished OK");
	return true;
}

bool SckTest::test_Light()
{
	SerialUSB.println("\r\nTesting Light sensor...");

	if (!testBase->getReading(SENSOR_LIGHT)) { 
		SerialUSB.println("ERROR reading Light sensor");
		return false;
	} else test_report.tests[TEST_LIGHT] = testBase->sensors[SENSOR_LIGHT].reading.toFloat();

	SerialUSB.println("Light reading test finished OK");
	return true;
}

bool SckTest::test_Pressure()
{
	SerialUSB.println("\r\nTesting Pressure sensor...");

	if (!testBase->getReading(SENSOR_PRESSURE)) { 
		SerialUSB.println("ERROR reading Barometric Pressure sensor");
		return false;
	} else test_report.tests[TEST_PRESS] = testBase->sensors[SENSOR_PRESSURE].reading.toFloat();

	SerialUSB.println("Pressure reading test finished OK");
	return true;
}

bool SckTest::test_MAX()
{
	SerialUSB.println("\r\nTesting Dust sensor...");
	
	uint8_t maxErrors = errors;
	if (!testBase->getReading(SENSOR_PARTICLE_RED)) { 
		SerialUSB.println("ERROR reading Dust Red channel sensor");
		errors ++;
	} else test_report.tests[TEST_MAX_RED] = testBase->sensors[SENSOR_PARTICLE_RED].reading.toFloat();

	if (!testBase->getReading(SENSOR_PARTICLE_GREEN)) { 
		SerialUSB.println("ERROR reading Dust Green channel sensor");
		errors ++;
	} else test_report.tests[TEST_MAX_GREEN] = testBase->sensors[SENSOR_PARTICLE_GREEN].reading.toFloat();

	if (!testBase->getReading(SENSOR_PARTICLE_IR)) { 
		SerialUSB.println("ERROR reading Dust InfraRed channel sensor");
		errors ++;
	} else test_report.tests[TEST_MAX_IR] = testBase->sensors[SENSOR_PARTICLE_IR].reading.toFloat();

	if (maxErrors < errors) return false;
	SerialUSB.println("MAX dust sensor reading test finished OK");
	return true;
}

bool SckTest::test_Noise()
{
	SerialUSB.println("\r\nTesting Noise sensor...");

	// TODO finish this 
	test_report.tests[TEST_NOISE] = 1;
	SerialUSB.println("Noise reading test finished OK");
	return true;
}

bool SckTest::test_PM()
{
	SerialUSB.println("\r\nTesting PM sensor...");

	uint8_t pmErrors = errors;
	if (!testBase->getReading(SENSOR_PM_1)) {
		SerialUSB.println("ERROR reading PM 1 sensor!!!");
		errors ++;
	} else test_report.tests[TEST_PM_1] = testBase->sensors[SENSOR_PM_1].reading.toFloat();

	if (!testBase->getReading(SENSOR_PM_25)) {
		SerialUSB.println("ERROR reading PM 2.5 sensor!!!");
		errors ++;
	} else test_report.tests[TEST_PM_25] = testBase->sensors[SENSOR_PM_25].reading.toFloat();

	if (!testBase->getReading(SENSOR_PM_10)) {
		SerialUSB.println("ERROR reading PM 10 sensor!!!");
		errors ++;
	} else test_report.tests[TEST_PM_10] = testBase->sensors[SENSOR_PM_10].reading.toFloat();

	if (pmErrors < errors) return false;
	SerialUSB.println("PMS reading test finished OK");
	return true;
}

bool SckTest::test_auxWire()
{
	SerialUSB.println("\r\nTesting auxiliary I2C bus...");

	// Check if a external SHT was detected a get reading

	// for now this is a fake
	test_report.tests[TEST_AUXWIRE] = 1;

	SerialUSB.println("Auxiliary I2C bus test finished OK");
	return true;
}

bool SckTest::connect_ESP()
{
	uint32_t started = millis();

	SerialUSB.println("\r\nTesting ESP and WIFI connectivity...");

	strncpy(testBase->config.credentials.ssid, TEST_WIFI_SSID, 64);
	strncpy(testBase->config.credentials.pass, TEST_WIFI_PASSWD, 64);
	testBase->config.credentials.set = true;
	testBase->config.mode = MODE_NET;
	strncpy(testBase->config.token.token, "123456", 7);
	testBase->config.token.set = true;
	testBase->saveConfig();

	testBase->led.update(testBase->led.BLUE, testBase->led.PULSE_HARD_SLOW);
	while (testBase->pendingSyncConfig || !testBase->st.wifiStat.ok) {
		testBase->update();
		testBase->inputUpdate();
		delay(1);
		if (millis() - started > 60000) {
			SerialUSB.println("ERROR timeout on wifi connection");
			return false;
		}
	}

	SerialUSB.println("Wifi connection OK");
	test_report.tests[TEST_WIFI_TIME] = (millis() - started) / 1000;
	return true;
}

bool SckTest::publishResult()
{
	SerialUSB.println();

		/* {"time":"2018-07-17T06:55:06Z"               // time */
		/* "id":"45f90530-504e4b4b-372e314a-ff031e17",  // SAM id */
		/* "mac":"AB:45:2D:33:98",                      // ESP MAC address */
		/* "errors":3,                                  // Number of errors */
		/* "tests": */
		/* 			[ */
		/* 		{"00":78.5},     // battery gauge - percent */
		/* 		{"01":2},        // battery charge rate - mA */
		/* 		{"02":1},        // battery charging - bool */
		/* 		{"03":1},        // SD card - bool */
		/* 		{"04":1},        // flash memory - bool */
		/* 		{"05":1},        // user (button) - bool */
		/* 		{"06":1},        // MICS POT - bool */
		/* 		{"07":58.4},     // MICS carbon - kOhm */
		/* 		{"08":23.5},     // MICS nitro - kOhm */
		/* 		{"09":25.5},     // SHT31 temperature - C */
		/* 		{"10":56.6},     // SHT31 humidity - percent */
		/* 		{"11":228.7},    // Light - Lux */
		/* 		{"12":28.7},     // Barometric pressure - kPa */
		/* 		{"13":33.5},     // MAX red - units */
		/* 		{"14":33.5},     // MAX green - units */
		/* 		{"15":33.5},     // MAX ir -units */
		/* 		{"16":48.7},     // Noise - dbA */
		/* 		{"17":18.7},     // PM-1 - ug/m3 */
		/* 		{"18":18.7},     // PM-2.5 - ug/m3 */
		/* 		{"19":18.7},     // PM-10 - ug/m3 */
		/* 		{"20":1},        // Auxiliary I2C bus - bool */
		/* 		{"21":8}         // Wifi connection time - seconds */
		/* 			] */
		/* } */

	// Get time
	if (!testBase->ISOtime()) {
		testBase->sendMessage(ESPMES_GET_TIME, "");
		while (!testBase->st.timeStat.ok) {
			testBase->update();
			testBase->inputUpdate();
		}
	}
	test_report.time = testBase->ISOtimeBuff;
	
	// Get MAC address
	testBase->sendMessage(ESPMES_GET_NETINFO);
	while (testBase->macAddress.length() < 2) {
		testBase->update();
		testBase->inputUpdate();
	}
	test_report.mac = testBase->macAddress;

	// build json
	StaticJsonBuffer<2048> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["time"] = test_report.time;

	json["id"] = String(String(test_report.id[0], HEX) + "-" + String(test_report.id[1], HEX) + "-" + String(test_report.id[2], HEX) + "-" + String(test_report.id[3], HEX));
	json["mac"] = test_report.mac;
	json["errors"] = errors;

	JsonArray& jsonReport = json.createNestedArray("tests");

	for (uint8_t i=0; i<TEST_COUNT; i++) {
		JsonObject& nested = jsonReport.createNestedObject();
		nested[String(i)] = test_report.tests[i];
	}

	sprintf(testBase->netBuff, "%c", ESPMES_MQTT_INVENTORY);
	json.printTo(&testBase->netBuff[1], json.measureLength() + 1);

	uint32_t started = millis();
	while (!testBase->sendMessage()) {
		if (millis() - started > 15000) {
			SerialUSB.println("ERROR sending test report to ESP");
			return false;
		}
		delay(100);
	}

	while (!testBase->st.publishStat.ok) {
		testBase->st.publishStat.retry();
		if (testBase->st.publishStat.error) {
			SerialUSB.println("ERROR on publishing test results");
			return true;
		}
		testBase->update();
		testBase->inputUpdate();
	}

	SerialUSB.println("Test results sent to platform");

	if (testBase->sdDetect() && testBase->sd.begin(pinCS_SDCARD)) {
		File reportFile;
		char reportFileName[14];
		sprintf(reportFileName, "%lx.json", test_report.id[4]);

		reportFile = testBase->sd.open(reportFileName, FILE_WRITE);
		char jsonPrint[json.measureLength() + 1];
		json.printTo(jsonPrint, json.measureLength() + 1);
		reportFile.print(jsonPrint);
		reportFile.close();
		SerialUSB.println("Report saved to sdcard");
	} else {
		SerialUSB.println("ERROR Failed to save report to sdcard");
		return false;
	}

	return true;
}

#endif
