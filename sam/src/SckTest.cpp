#include "SckBase.h"

#ifdef TESTING
#include "SckTest.h"

extern SckSerial serESP;

void SckTest::test_full()
{
    testBase->led.update(testBase->led.WHITE, testBase->led.PULSE_STATIC);

    // Enable sensors for test
    testBase->enableSensor(SENSOR_TEMPERATURE);
    testBase->enableSensor(SENSOR_HUMIDITY);
    testBase->enableSensor(SENSOR_LIGHT);
    testBase->enableSensor(SENSOR_AS7331_UVA);
    testBase->enableSensor(SENSOR_LPS33_PRESSURE);
    testBase->enableSensor(SENSOR_NOISE_DBA);
    testBase->enableSensor(SENSOR_SEN5X_PM_1);

    // Make sure al results are 0
    for (uint8_t i=0; i<TEST_COUNT; i++) {
        test_report.tests[i] = 0;
    }

    delay(2000);
    testBase->st.onShell = true;

    testBase->ESPcontrol(testBase->ESP_REBOOT);
    SerialUSB.println("Waiting for ESP first boot...");
    SerialUSB.println("May take some time due to flash formating");
    SerialUSB.println("Please don't disconnect the SCK");
    uint32_t startWaiting = millis();
    while (testBase->st.espBooting) {
        if (millis() % 1000 == 0) SerialUSB.print('.'); delay(1);
        if (millis() - startWaiting > 120000) {
            SerialUSB.println("ESP is taking too long to wakeup");
            SerialUSB.println("Please reflash ESP chip and try again");
        }
        testBase->inputUpdate();
    }

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

    testBase->config.outLevel = OUT_SILENT;

    // Start WiFi
    if (!connect_ESP()) errors++;


    // Test battery
    if (test_battery() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_battery();
    }

    // Test SDcard
    if (test_sdcard() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_sdcard();
    }

    // Test Flash memory
    if (test_flash() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_flash();
    }

    // Test SHT temp and hum
    if (test_SHT() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_SHT();
    }

    // Test Light
    if (test_Light() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_Light();
    }

    // Test UVA
    if (test_UVA() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_UVA();
    }

    // Test Pressure
    if (test_Pressure() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_Pressure();
    }

    // Test Noise
    if (test_Noise() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_Noise();
    }

    // Test SEN5X PM sensor
    if (test_SEN5X() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_SEN5X();
    }

    // Test auxiliary I2C bus
    if (test_auxWire() > 0) {
        SerialUSB.println("Retrying...");
        delay(500);
        title = false;
        errors += test_auxWire();
    }

    // Publish result
    if (!publishResult()) {
        SerialUSB.println("Retrying...");
        delay(500);
        if (!publishResult()) errors++;
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
    while(true) {
        delay(100);
    }
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
                testBase->led.update(testBase->led.BLUE, testBase->led.PULSE_ERROR);
                SerialUSB.println("Changing Led to blue..\r\nButton and led test finished OK");
                butLedState = TEST_FINISHED;
                break;
            default: break;
        }
    }
}

uint8_t SckTest::test_battery()
{
    if (title) SerialUSB.println("\r\nTesting battery");

    uint8_t error = 0;

    testBase->charger.chargeState(0);

    // One pause to check for battery
    uint32_t startPoint = millis();
    delay(1000);
    while (millis() - startPoint < 10000) {
        testBase->updatePower();
        delay(500);
        if (testBase->battery.present) break;
    }

    if (!testBase->battery.present) {
        SerialUSB.println("ERROR no battery detected!!");
        return 1; // No point on doing tests if no battery present
    } else {
        // Turn on charger and wait to be sure charging has started
        testBase->charger.chargeState(1);
        startPoint = millis();
        while (millis() - startPoint < 2000) {
            testBase->updatePower();
        }
    }

    if (!testBase->getReading(&testBase->sensors[SENSOR_BATT_VOLTAGE]) || testBase->sensors[SENSOR_BATT_VOLTAGE].reading.toFloat() <= 0) {
        SerialUSB.println("ERROR reading battery voltage!");
        error++;
    } else {
        test_report.tests[TEST_BATT_VOLT] = testBase->sensors[SENSOR_BATT_VOLTAGE].reading.toFloat();
        SerialUSB.print("Battery voltage: ");
        SerialUSB.print(test_report.tests[TEST_BATT_VOLT]);
        SerialUSB.println(" V");
    }

    if (testBase->charger.getChargeStatus() != testBase->charger.CHRG_FAST_CHARGING) {
        SerialUSB.println("ERROR battery is not charging!!");
        error++;
    } else {
        test_report.tests[TEST_BATT_CHG] = 1;
        SerialUSB.print("Charger status: ");
        SerialUSB.println(testBase->charger.chargeStatusTitles[testBase->charger.getChargeStatus()]);
        SerialUSB.println("Battery test finished OK");
    }

    title = true;
    return error;
}

uint8_t SckTest::test_sdcard()
{
    uint8_t error = 0;

    if (title) SerialUSB.println("\r\nTesting SDcard...");
    testBase->sdDetect();
    if (!testBase->st.cardPresent) {
        SerialUSB.println("ERROR No SD card detected!!!");
        error = 1;
    }

    digitalWrite(pinCS_FLASH, HIGH);    // disables Flash
    digitalWrite(pinCS_SDCARD, LOW);

    if (!testBase->sdInit()) {
        SerialUSB.println(F("ERROR Cant't start Sdcard!!!"));
        error = 1;
    }


    // Create a file write to it, close it, reopen read from it and delete it
    File testFile;

    if (testBase->sd.exists("TEST.TXT")) testBase->sd.remove("TEST.TXT");
    testFile = testBase->sd.open("TEST.TXT", FILE_WRITE);
    testFile.println("testing");
    testFile.close();

    delay(100);

    testFile = testBase->sd.open("TEST.TXT", FILE_READ);
    char testString[8];
    testFile.read(testString, 9);
    testFile.close();

    testBase->sd.remove("TEST.TXT");

    String strTest = String(testString);
    if (!strTest.startsWith("testing")) {
        SerialUSB.println("ERROR writing/reading sdcard!!!");
        error = 1;
    } else {
        test_report.tests[TEST_SD] = 1;
        SerialUSB.println("SDcard test finished OK");
    }

    title = true;
    return error;
}

uint8_t SckTest::test_flash()
{
    uint8_t error = 0;

    if (title) SerialUSB.println("\r\nTesting Flash memory...");

    uint32_t fCapacity = testBase->readingsList.getFlashCapacity();
    if (fCapacity > 0) {
        SerialUSB.print("Found flash chip with ");
        SerialUSB.print(fCapacity);
        SerialUSB.println(" bytes of size.");
    } else {
        SerialUSB.println("ERROR recognizing flash chip!!!");
        error = 1;
    }

    if (!testBase->readingsList.testFlash()) {
        SerialUSB.println("ERROR writing/reading flash chip!!!");
        error = 1;
    }

    test_report.tests[TEST_FLASH] = 1;
    SerialUSB.println("Flash memory test finished OK");
    title = true;
    return error;
}

uint8_t SckTest::test_SHT()
{
    if (title) SerialUSB.println("\r\nTesting SHT31 sensor...");

    uint8_t error = 0;

    if (!testBase->sensors[SENSOR_TEMPERATURE].enabled || !testBase->getReading(&testBase->sensors[SENSOR_TEMPERATURE])) {
        SerialUSB.println("ERROR reading SHT31 temperature sensor");
        error ++;
    } else {
        test_report.tests[TEST_TEMP] = testBase->sensors[SENSOR_TEMPERATURE].reading.toFloat();
        sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_TEMPERATURE].title, test_report.tests[TEST_TEMP], testBase->sensors[SENSOR_TEMPERATURE].unit);
        SerialUSB.println(testBase->outBuff);
    }

    if (!testBase->sensors[SENSOR_HUMIDITY].enabled || !testBase->getReading(&testBase->sensors[SENSOR_HUMIDITY])) {
        SerialUSB.println("ERROR reading SHT31 humidity sensor");
        error ++;
    } else {
        test_report.tests[TEST_HUM] = testBase->sensors[SENSOR_HUMIDITY].reading.toFloat();
        sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_HUMIDITY].title, test_report.tests[TEST_HUM], testBase->sensors[SENSOR_HUMIDITY].unit);
        SerialUSB.println(testBase->outBuff);
    }

    if (error == 0) SerialUSB.println("SHT31 sensors test finished OK");
    title = true;
    return error;
}

uint8_t SckTest::test_Light()
{
    uint8_t error = 0;

    if (title) SerialUSB.println("\r\nTesting Light sensor...");

    if (!testBase->sensors[SENSOR_LIGHT].enabled || !testBase->getReading(&testBase->sensors[SENSOR_LIGHT])) {
        SerialUSB.println("ERROR reading Light sensor");
        error = 1;
    } else {
        test_report.tests[TEST_LIGHT] = testBase->sensors[SENSOR_LIGHT].reading.toFloat();
        sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_LIGHT].title, test_report.tests[TEST_LIGHT], testBase->sensors[SENSOR_LIGHT].unit);
        SerialUSB.println(testBase->outBuff);
        SerialUSB.println("Light sensor test finished OK");
    }

    title = true;
    return error;
}

uint8_t SckTest::test_UVA()
{
    uint8_t error = 0;

    if (title) SerialUSB.println("\r\nTesting UVA sensor...");

    if (!testBase->sensors[SENSOR_AS7331_UVA].enabled || !testBase->getReading(&testBase->sensors[SENSOR_AS7331_UVA])) {
        SerialUSB.println("ERROR reading UVA sensor");
        error = 1;
    } else {
        test_report.tests[SENSOR_AS7331_UVA] = testBase->sensors[SENSOR_AS7331_UVA].reading.toFloat();
        sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_AS7331_UVA].title, test_report.tests[SENSOR_AS7331_UVA], testBase->sensors[SENSOR_AS7331_UVA].unit);
        SerialUSB.println(testBase->outBuff);
        SerialUSB.println("UVA sensor test finished OK");
    }

    title = true;
    return error;
}
uint8_t SckTest::test_Pressure()
{
    uint8_t error = 0;

    if (title) SerialUSB.println("\r\nTesting Pressure sensor...");

    if (!testBase->getReading(&testBase->sensors[SENSOR_LPS33_PRESSURE])) {
        SerialUSB.println("ERROR reading Barometric Pressure sensor");
        error = 1;
    } else {
        test_report.tests[TEST_PRESS] = testBase->sensors[SENSOR_LPS33_PRESSURE].reading.toFloat();
        sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_LPS33_PRESSURE].title, test_report.tests[TEST_PRESS], testBase->sensors[SENSOR_LPS33_PRESSURE].unit);
        SerialUSB.println(testBase->outBuff);
        SerialUSB.println("Pressure sensor test finished OK");
    }

    title = true;
    return error;
}

uint8_t SckTest::test_Noise()
{
    uint8_t error = 0;

    if (title) SerialUSB.println("\r\nTesting Noise sensor...");

    if (!testBase->sensors[SENSOR_NOISE_DBA].enabled || !testBase->getReading(&testBase->sensors[SENSOR_NOISE_DBA])) {
        SerialUSB.println("ERROR reading Noise sensor");
        error = 1;
    } else {
        test_report.tests[TEST_NOISE] = testBase->sensors[SENSOR_NOISE_DBA].reading.toFloat();
        sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_NOISE_DBA].title, test_report.tests[TEST_NOISE], testBase->sensors[SENSOR_NOISE_DBA].unit);
        SerialUSB.println(testBase->outBuff);
        SerialUSB.println("Noise sensor test finished OK");
    }

    title = true;
    return error;
}

uint8_t SckTest::test_SEN5X()
{
    uint8_t error = 0;
    testBase->urban.sck_sen5x.monitor = true;

    if (title) SerialUSB.println("\r\nTesting SEN5X PM sensor...");

    if (!testBase->sensors[SENSOR_SEN5X_PM_1].enabled || !testBase->getReading(&testBase->sensors[SENSOR_SEN5X_PM_1])) {
        SerialUSB.println("ERROR reading SEN5X PM sensor");
        error = 1;
    } else {
        test_report.tests[TEST_SEN5X] = testBase->sensors[SENSOR_SEN5X_PM_1].reading.toFloat();
        sprintf(testBase->outBuff, "%s: %.2f %s", testBase->sensors[SENSOR_SEN5X_PM_1].title, test_report.tests[TEST_SEN5X], testBase->sensors[SENSOR_SEN5X_PM_1].unit);
        SerialUSB.println(testBase->outBuff);
        SerialUSB.println("SEN5X PM sensor test finished OK");
    }

    title = true;
    return error;
}

uint8_t SckTest::test_auxWire()
{
    uint8_t error = 0;

    if (title) SerialUSB.println("\r\nTesting auxiliary I2C bus...");

    // Check if a external SHT was detected a get reading
    if (!testBase->sensors[SENSOR_SHT35_TEMP].enabled) {
        SerialUSB.println("ERROR No external SHT35 sensor found on Auxiliary I2C bus!!!");
        error = 1;

    } else if (!testBase->getReading(&testBase->sensors[SENSOR_SHT35_TEMP])) {
        SerialUSB.println("ERROR reading external SHT35 sensor");
        error = 1;

    } else {
        test_report.tests[TEST_AUXWIRE] = 1;
        sprintf(testBase->outBuff, "%s: %s %s", testBase->sensors[SENSOR_SHT35_TEMP].title, testBase->sensors[SENSOR_SHT35_TEMP].reading.c_str(), testBase->sensors[SENSOR_SHT35_TEMP].unit);
        SerialUSB.println(testBase->outBuff);
        SerialUSB.println("Auxiliary I2C bus test finished OK");
    }

    title = true;
    return error;
}

bool SckTest::connect_ESP()
{
    strncpy(testBase->config.credentials.ssid, TEST_WIFI_SSID, 64);
    strncpy(testBase->config.credentials.pass, TEST_WIFI_PASSWD, 64);
    testBase->config.credentials.set = true;
    testBase->config.mode = MODE_NET;
    strncpy(testBase->config.token.token, "123456", 7);
    testBase->config.token.set = true;
    testBase->config.outLevel = OUT_SILENT;
    testBase->saveConfig();

    SerialUSB.println("\r\nTesting ESP and WIFI connectivity...");

    testBase->led.update(testBase->led.BLUE, testBase->led.PULSE_ERROR);
    uint32_t started = millis();
    while (!testBase->st.wifiStat.ok) {

        testBase->update();
        testBase->inputUpdate();
        delay(100);
        if (millis() - started > 80000) {
            SerialUSB.println("ERROR timeout on wifi connection");
            return false;
        }
    }

    // Clean any ESP msg
    uint32_t start = millis();
    while (millis() - start < 1000) {
        testBase->update();
        testBase->inputUpdate();
    }

    SerialUSB.println("Wifi connection OK");
    test_report.tests[TEST_WIFI_TIME] = (millis() - started) / 1000;
    return true;
}

bool SckTest::publishResult()
{
    SerialUSB.println();

    /* {"time":"2018-07-17T06:55:06Z",              // time */
    /* "id":"45f90530-504e4b4b-372e314a-ff031e17",  // SAM id */
    /* "mac":"AB:45:2D:33:98",                      // ESP MAC address */
    /* "errors":3,                                  // Number of errors */
    /* "tests": */
    /*          [ */
    /*      {"0":3.5},      // battery - voltage */
    /*      {"1":1},        // battery charging - bool */
    /*      {"2":1},        // SD card - bool */
    /*      {"3":1},        // flash memory - bool */
    /*      {"4":1},        // user (button) - bool */
    /*      {"5":25.5},     // SHT31 temperature - C */
    /*      {"6":56.6},     // SHT31 humidity - percent */
    /*      {"7":228.7},    // Light - Lux */
    /*      {"8":2.7},      // UVA - uW/cm2 */
    /*      {"9":28.7},     // Barometric pressure - kPa */
    /*      {"10":48.7},     // Noise - dbA */
    /*      {"11":18.7},     // PM-1 - ug/m3 */
    /*      {"12":1},        // Auxiliary I2C bus - bool */
    /*      {"13":8}         // Wifi connection time - seconds */
    /*          ] */
    /* } */

    // Get time if needed
    if (!testBase->st.timeStat.ok) {
        testBase->ESPsend(ESPMES_GET_TIME, "");
        while (!testBase->st.timeStat.ok) {
            testBase->update();
            testBase->inputUpdate();
        }
    }

    test_report.time = testBase->ISOtimeBuff;
    test_report.mac = testBase->config.mac.address;

    // build json
    char *buffer = testBase->serESPBuffPtr;
    String id = String(String(test_report.id[0], HEX) + "-" + String(test_report.id[1], HEX) + "-" + String(test_report.id[2], HEX) + "-" + String(test_report.id[3], HEX));
	sprintf(buffer, "{\"time\":\"%s\",\"id\":\"%s\",\"mac\":\"%s\",\"errors\":%u,\"tests\":[", test_report.time.c_str(), id.c_str(), test_report.mac.c_str(), errors);

    for (uint8_t i=0; i<TEST_COUNT; i++) {
        if (i > 0) sprintf(buffer, "%s,", buffer);
        sprintf(buffer, "%s{\"%u\":%0.2f}", buffer, i, test_report.tests[i]);
    }
    sprintf(buffer, "%s]}", buffer);

    // Clean any ESP msg
    uint32_t start = millis();
    while (millis() - start < 1000) {
        testBase->update();
        testBase->inputUpdate();
    }

    // Send message
    uint32_t started = millis();
    testBase->st.publishStat.reset();
    testBase->ESPsend(ESPMES_MQTT_INVENTORY);
    while (!testBase->st.publishStat.ok) {
        if (testBase->st.publishStat.error || millis() - started > 15000) {
            SerialUSB.println("ERROR on publishing test results");
            return false;
        }
        testBase->update();
        testBase->inputUpdate();
    }

    SerialUSB.println("Test results sent to platform");

    return true;
}

#endif
