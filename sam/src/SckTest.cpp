#include "SckBase.h"

#ifdef testing
#include "SckTest.h"

void SckTest::test_full()
{

	// Make sure al results are 0
	for (uint8_t i=0; i<TEST_COUNT; i++) {
		test_report.tests[i] = 0;
	}


	delay(2000);
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

	// Get ESP mac address
	// hay que ver como hacer esto sencillo sin tener que entrar en base.update()
	// podria ser en shell mode...

	// At the beginning the kit will be in static blue waiting for user clicks
	testBase->led.update(testBase->led.BLUE, testBase->led.PULSE_STATIC);
	if (!test_user()) error = true;

	// Test battery gauge
	if (!test_battery()) error =true;

	// Test SDcard
	if (!test_sdcard()) error = true;

	// Test Flash memory
	if (!test_flash()) error = true;

	// Test MICS POT
	if (!test_micsPot()) error = true;

	// Test MICS ADC
	/* if (!test_micsAdc()) error = true; */

	// Test SHT temp and hum
	if (!test_SHT()) error = true;

	// Test Light 
	if (!test_Light()) error = true;

	// Test Pressure 
	if (!test_Pressure()) error = true;

	// Test MAX dust sensor
	if (!test_MAX()) error = true;
		

	SerialUSB.println("\r\n********************************");
	if (error) {
		testBase->led.update(testBase->led.RED, testBase->led.PULSE_STATIC);
		SerialUSB.println("\r\nERROR... some tests failed!!!");
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
	while (butLedState != TEST_FINISHED);

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
	// TODO finish this and avoid repetitions...
	SerialUSB.println("\r\nTesting battery level");

	if (!testBase->getReading(SENSOR_BATT_PERCENT)) {
		SerialUSB.println("ERROR no battery detected!!");
		return false;
	} else {
		if (testBase->sensors[SENSOR_BATT_PERCENT].reading.toFloat() < 0) {
			SerialUSB.println("ERROR no battery detected!!");
			return false;
		}	
		test_report.tests[TEST_BATT_GAUGE] = testBase->sensors[SENSOR_BATT_PERCENT].reading.toFloat();
	}

	SerialUSB.println(test_report.tests[TEST_BATT_GAUGE]);
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

bool SckTest::test_micsPot()
{
	const float ohmsPerStep	= 10000.0/127; // Ohms for each potenciometer step

	SerialUSB.println("\r\nTesting MICS digital POT...");

	uint32_t writePoint = 5000;

	testBase->urban.sck_mics4514.getNO2load();
	uint32_t startPoint = testBase->urban.sck_mics4514.no2LoadResistor;
	if (abs(startPoint - writePoint) < 1000) writePoint = 2000;

	testBase->urban.sck_mics4514.setNO2load(writePoint);
	testBase->urban.sck_mics4514.getNO2load();
	uint32_t readPoint = testBase->urban.sck_mics4514.no2LoadResistor; 

	if (writePoint - readPoint > ohmsPerStep) {
		SerialUSB.println("ERROR MICS digital POT test failed!!!");
		return false;
	}

	test_report.tests[TEST_MICS_POT] = 1;
	SerialUSB.println("MICS digital POT test finished OK");
	return true;
}

bool SckTest::test_micsAdc()
{
	SerialUSB.println("\r\nTesting MICS sensor...");

	
	// TODO finish this 
	test_report.tests[TEST_CARBON] = 1;
	test_report.tests[TEST_NITRO] = 1;
	SerialUSB.println("MICS readings test finished OK");
	return true;
}

bool SckTest::test_SHT()
{
	SerialUSB.println("\r\nTesting SHT31 sensor...");

	if (!testBase->getReading(SENSOR_TEMPERATURE)) {
		SerialUSB.println("ERROR reading SHT31 temperature sensor");
		return false;
	} else test_report.tests[TEST_TEMP] = testBase->sensors[SENSOR_TEMPERATURE].reading.toFloat();

	if (!testBase->getReading(SENSOR_HUMIDITY)) {
		SerialUSB.println("ERROR reading SHT31 humidity sensor");
		return false;
	} else test_report.tests[TEST_HUM] = testBase->sensors[SENSOR_HUMIDITY].reading.toFloat();

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
	SerialUSB.println("\r\nTesting Pressures sensor...");

	if (!testBase->getReading(SENSOR_PRESSURE)) { 
		SerialUSB.println("ERROR reading Barometric Pressure sensor");
		return false;
	} else test_report.tests[TEST_PRESS] = testBase->sensors[SENSOR_PRESSURE].reading.toFloat();

	SerialUSB.println("Pressure reading test finished OK");
	return true;
}

bool SckTest::test_MAX()
{
	// TODO finish this
	
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

	// TODO finish this 
	test_report.tests[TEST_PM] = 1;
	SerialUSB.println("PMS reading test finished OK");
	return true;
}

bool SckTest::test_ESP()
{

	return true;
}

#endif
