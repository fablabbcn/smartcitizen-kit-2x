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
	testBase->enableSensor(SENSOR_CCS811_VOCS);
	testBase->enableSensor(SENSOR_CCS811_ECO2);
	testBase->enableSensor(SENSOR_NOISE_DBA);
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

	// Test battery
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

	// Test Noise
	if (!test_Noise()) errors++;

	// Test Air Quality
	test_VOC();

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
	SerialUSB.println("\r\nTesting battery");

	uint8_t battErrors = errors;

	testBase->charger.chargeState(0);

	// One pause to check for battery
	uint32_t startPoint = millis();
	delay(100);
	while (millis() - startPoint < 2000) {
		testBase->updatePower();
		if (testBase->battery.present) break;
	}

	if (!testBase->battery.present) {
		SerialUSB.println("ERROR no battery detected!!");
		errors ++;
	} else {
		testBase->charger.chargeState(1);
		startPoint = millis();
		while (millis() - startPoint < 2000) {
			testBase->updatePower();
		}
	}

	if (!testBase->getReading(SENSOR_BATT_VOLTAGE) || testBase->sensors[SENSOR_BATT_VOLTAGE].reading.toFloat() <= 0) {
		SerialUSB.println("ERROR reading battery voltage!");
		errors ++;
	} else {
		test_report.tests[TEST_BATT_VOLT] = testBase->sensors[SENSOR_BATT_VOLTAGE].reading.toFloat();
	}

	if (testBase->charger.getChargeStatus() != testBase->charger.CHRG_FAST_CHARGING) {
		SerialUSB.println("ERROR battery is not charging!!");
		errors ++;
	} else {
		test_report.tests[TEST_BATT_CHG] = 1;
	}

	if (battErrors < errors) return false;

	SerialUSB.print("Battery voltage: ");
	SerialUSB.print(test_report.tests[TEST_BATT_VOLT]);
	SerialUSB.println(" V");
	SerialUSB.print("Charger status: ");
	SerialUSB.println(testBase->charger.chargeStatusTitles[testBase->charger.getChargeStatus()]);
	SerialUSB.println("Battery test finished OK");
	return true;
}

bool SckTest::test_sdcard()
{

	SerialUSB.println("\r\nTesting SDcard...");
	testBase->sdDetect();
	if (!testBase->st.cardPresent) {
		SerialUSB.println("ERROR No SD card detected!!!");	
		return false;
	}
	
	digitalWrite(pinCS_FLASH, HIGH);	// disables Flash
	digitalWrite(pinCS_SDCARD, LOW);

	if (!testBase->sdInit()) { 
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
	sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_TEMPERATURE].title, test_report.tests[TEST_TEMP], testBase->sensors[SENSOR_TEMPERATURE].unit);
	SerialUSB.println(testBase->outBuff);
	sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_HUMIDITY].title, test_report.tests[TEST_HUM], testBase->sensors[SENSOR_HUMIDITY].unit);
	SerialUSB.println(testBase->outBuff);
	SerialUSB.println("SHT31 sensors test finished OK");
	return true;
}

bool SckTest::test_Light()
{
	SerialUSB.println("\r\nTesting Light sensor...");

	if (!testBase->getReading(SENSOR_LIGHT)) { 
		SerialUSB.println("ERROR reading Light sensor");
		return false;
	} else test_report.tests[TEST_LIGHT] = testBase->sensors[SENSOR_LIGHT].reading.toFloat();

	sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_LIGHT].title, test_report.tests[TEST_LIGHT], testBase->sensors[SENSOR_LIGHT].unit);
	SerialUSB.println(testBase->outBuff);
	SerialUSB.println("Light sensor test finished OK");
	return true;
}

bool SckTest::test_Pressure()
{
	SerialUSB.println("\r\nTesting Pressure sensor...");

	if (!testBase->getReading(SENSOR_PRESSURE)) { 
		SerialUSB.println("ERROR reading Barometric Pressure sensor");
		return false;
	} else test_report.tests[TEST_PRESS] = testBase->sensors[SENSOR_PRESSURE].reading.toFloat();

	sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_PRESSURE].title, test_report.tests[TEST_PRESS], testBase->sensors[SENSOR_PRESSURE].unit);
	SerialUSB.println(testBase->outBuff);
	SerialUSB.println("Pressure sensor test finished OK");
	return true;
}

bool SckTest::test_Noise()
{
	SerialUSB.println("\r\nTesting Noise sensor...");

	if (!testBase->getReading(SENSOR_NOISE_DBA)) {
		SerialUSB.println("ERROR reading Noise sensor");
		return false;
	} else test_report.tests[TEST_NOISE] = testBase->sensors[SENSOR_NOISE_DBA].reading.toFloat();

	sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_NOISE_DBA].title, test_report.tests[TEST_NOISE], testBase->sensors[SENSOR_NOISE_DBA].unit);
	SerialUSB.println(testBase->outBuff);
	SerialUSB.println("Noise sensor test finished OK");
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
	sprintf(testBase->outBuff, "%s: %.0f %s", testBase->sensors[SENSOR_PM_1].title, test_report.tests[TEST_PM_1], testBase->sensors[SENSOR_PM_1].unit);
	SerialUSB.println(testBase->outBuff);
	sprintf(testBase->outBuff, "%s: %.0f %s", testBase->sensors[SENSOR_PM_25].title, test_report.tests[TEST_PM_25], testBase->sensors[SENSOR_PM_25].unit);
	SerialUSB.println(testBase->outBuff);
	sprintf(testBase->outBuff, "%s: %.0f %s", testBase->sensors[SENSOR_PM_10].title, test_report.tests[TEST_PM_10], testBase->sensors[SENSOR_PM_10].unit);
	SerialUSB.println(testBase->outBuff);
	SerialUSB.println("PMS sensors test finished OK");
	return true;
}

bool SckTest::test_VOC()
{
	SerialUSB.println("\r\nTesting Air Quality sensor...");

	uint8_t vocErrors = errors;
	if (!testBase->getReading(SENSOR_CCS811_VOCS)) { 
		SerialUSB.println("ERROR reading Air Quality VOC's sensor");
		errors++;
	} else test_report.tests[TEST_VOCS] = testBase->sensors[SENSOR_CCS811_VOCS].reading.toFloat();

	if (!testBase->getReading(SENSOR_CCS811_ECO2)) { 
		SerialUSB.println("ERROR reading Air Quality ECO2 sensor");
		errors++;
	} else test_report.tests[TEST_ECO2] = testBase->sensors[SENSOR_CCS811_ECO2].reading.toFloat();

	if (vocErrors < errors) return false;
	sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_CCS811_VOCS].title, test_report.tests[TEST_VOCS], testBase->sensors[SENSOR_CCS811_VOCS].unit);
	SerialUSB.println(testBase->outBuff);
	sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_CCS811_ECO2].title, test_report.tests[TEST_ECO2], testBase->sensors[SENSOR_CCS811_ECO2].unit);
	SerialUSB.println(testBase->outBuff);
	SerialUSB.println("Air Quality sensor test finished OK");
	return true;
}

bool SckTest::test_auxWire()
{
	SerialUSB.println("\r\nTesting auxiliary I2C bus...");

	// Check if a external SHT was detected a get reading
	if (!testBase->sensors[SENSOR_SHT31_TEMP].enabled) {
		SerialUSB.println("ERROR No external SHT31 sensor found on Auxiliary I2C bus!!!");	
		return false;
	}

	if (!testBase->getReading(SENSOR_SHT31_TEMP)) {
		SerialUSB.println("ERROR reading external SHT31 sensor");
		errors ++;
	} else test_report.tests[TEST_AUXWIRE] = 1;

	sprintf(testBase->outBuff, "%s: %s %s", testBase->sensors[SENSOR_SHT31_TEMP].title, testBase->sensors[SENSOR_SHT31_TEMP].reading.c_str(), testBase->sensors[SENSOR_SHT31_TEMP].unit);
	SerialUSB.println(testBase->outBuff);
	SerialUSB.println("Auxiliary I2C bus test finished OK");
	return true;
}

bool SckTest::connect_ESP()
{
	strncpy(testBase->config.credentials.ssid, TEST_WIFI_SSID, 64);
	strncpy(testBase->config.credentials.pass, TEST_WIFI_PASSWD, 64);
	testBase->config.credentials.set = true;
	testBase->config.mode = MODE_NET;
	strncpy(testBase->config.token.token, "123456", 7);
	testBase->config.token.set = true;
	testBase->saveConfig();

	SerialUSB.println("\r\nTesting ESP and WIFI connectivity...");

	testBase->led.update(testBase->led.BLUE, testBase->led.PULSE_HARD_SLOW);
	uint32_t started = millis();
	while (testBase->pendingSyncConfig || !testBase->st.wifiStat.ok) {

		if (millis() % 5000 == 0) {
			delay(1);
			testBase->ESPcontrol(testBase->ESP_REBOOT);
		}

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
		/* 		{"00":3.5},     // battery - voltage */
		/* 		{"01":1},        // battery charging - bool */
		/* 		{"02":1},        // SD card - bool */
		/* 		{"03":1},        // flash memory - bool */
		/* 		{"04":1},        // user (button) - bool */
		/* 		{"05":25.5},     // SHT31 temperature - C */
		/* 		{"06":56.6},     // SHT31 humidity - percent */
		/* 		{"07":228.7},    // Light - Lux */
		/* 		{"08":28.7},     // Barometric pressure - kPa */
		/* 		{"09":48.7},     // VOC's - ppb */
		/* 		{"10":88.5},     // ECO2 - ppm */
		/* 		{"11":48.7},     // Noise - dbA */
		/* 		{"12":18.7},     // PM-1 - ug/m3 */
		/* 		{"13":18.7},     // PM-2.5 - ug/m3 */
		/* 		{"14":18.7},     // PM-10 - ug/m3 */
		/* 		{"15":1},        // Auxiliary I2C bus - bool */
		/* 		{"16":8}         // Wifi connection time - seconds */
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

	testBase->sdDetect();
	if (testBase->st.cardPresent && testBase->sdInit()) {
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
