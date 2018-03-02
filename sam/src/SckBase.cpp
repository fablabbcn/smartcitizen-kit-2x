#include "SckBase.h"
#include "Commands.h"

// Software Serial ESP
SoftwareSerial SerialESP(pinESP_RX_WIFI, pinESP_TX_WIFI);

// Hardware Serial ESP handler (only for flashing)
Uart SerialFlashESP (&sercom4, pinESP_RX_WIFI, pinESP_TX_WIFI, SERCOM_RX_PAD_1, UART_TX_PAD_0);
void SERCOM4_Handler() {
  	SerialFlashESP.IrqHandler();
}

// Hardware Auxiliary I2C bus
TwoWire auxWire(&sercom1, pinAUX_WIRE_SDA, pinAUX_WIRE_SCL);
void SERCOM1_Handler(void) {
	auxWire.onService();
}

void SckBase::setup() {

	// Led
	led.setup();

	// Internal I2C bus setup
	Wire.begin();

	// Auxiliary I2C bus
	auxWire.begin();

	// Output
	outputLevel = OUT_VERBOSE;

	// Button
	pinMode(pinBUTTON, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBUTTON, ISR_button, CHANGE);

	// Pause for a moment (for uploading firmware in case of problems)
	delay(4000);
	
	// Power management configuration
	// battSetup();

	// ESP Configuration
	pinMode(pinPOWER_ESP, OUTPUT);
	pinMode(pinESP_CH_PD, OUTPUT);
	pinMode(pinESP_GPIO0, OUTPUT);
	ESPcontrol(ESP_OFF);

	// Peripheral setup
	rtc.begin();

	// SDcard and flash select pins
	pinMode(pinCS_SDCARD, OUTPUT);
	pinMode(pinCS_FLASH, OUTPUT);
	digitalWrite(pinCS_SDCARD, HIGH);
	digitalWrite(pinCS_FLASH, HIGH);
	pinMode(pinCARD_DETECT, INPUT_PULLUP);

	// SD card
	attachInterrupt(pinCARD_DETECT, ISR_sdDetect, CHANGE);
	sdDetect();

	// Flash memory


	// Turn off Aux power TEMP THIS WILL BE REMOVED
	pinMode(31, OUTPUT);
	digitalWrite(31, LOW);

	// Configuration
	
	// Urban board
	urbanPresent = urban.setup();
	if (urbanPresent) {
		
		sckOut("Urban board detected");
	
		// Activate enabled sensors

	}

	// TEMP
	// config.mode = MODE_NET;
	flashSelect();

}
void SckBase::update() {

	// Check Serial port input
	inputUpdate();

	// Sent test text via i2C to pm board (address 8)
	// SerialUSB.println("Sending data to aux i2C");
	// Wire.beginTransmission(8);
	// Wire.write("Hola desde el SCK 2.0!!!");
	// Wire.endTransmission();
}


// Input
void SckBase::inputUpdate() {

	if (onUSB) {
		if (SerialUSB.available()) {

			char buff = SerialUSB.read();
			uint16_t blen = serialBuff.length();

			// New line
			if (buff == 13 || buff == 10) {

				SerialUSB.println();				// Newline

				serialBuff.replace("\n", "");		// Clean input
				serialBuff.replace("\r", "");
				serialBuff.trim();

				commands.in(this, serialBuff);		// Process input
				if (blen > 0) previousCommand = serialBuff;
				serialBuff = "";
				prompt();

			// Backspace
			} else if (buff == 127) {

				if (blen > 0) SerialUSB.print("\b \b");
				serialBuff.remove(blen-1);

			// Up arrow (previous command)
			} else if (buff == 27) {

				SerialUSB.read();				// drop next char (always 91)
				if (SerialUSB.read() == 65) {	// detect up arrow
					for (uint8_t i=0; i<blen; i++) SerialUSB.print("\b \b");	// clean previous command
					SerialUSB.print(previousCommand);
					serialBuff = previousCommand;
				}

			// Normal char
			} else {

				serialBuff += buff;
				SerialUSB.print(buff);				// Echo

			}
		}
	}
}

// Output
void SckBase::sckOut(String strOut, PrioLevels priority, bool newLine) {
	strOut.toCharArray(outBuff, strOut.length()+1);
	sckOut(priority, newLine);
}
void SckBase::sckOut(const char *strOut, PrioLevels priority, bool newLine) {
	strncpy(outBuff, strOut, 240);
	sckOut(priority, newLine);
}
void SckBase::sckOut(PrioLevels priority, bool newLine) {

	// Output via USB console
	if (onUSB) {
		if (outputLevel + priority > 1) {
			if (newLine) SerialUSB.println(outBuff);
			else SerialUSB.print(outBuff);
		}
	}
	
	strncpy(outBuff, "", 240);
}
void SckBase::prompt() {

	sckOut("SCK > ", PRIO_MED, false);
}

// Configuration
// void SckBase::saveSDconfig() {

// 	sckOut("Trying to save configuration to SDcard...");

// 	if (cardPresent) {
// 		// Remove old sdcard config
// 		if (sd.exists(configFileName)) sd.remove(configFileName);

// 		// Save to sdcard
// 		if (openConfigFile()) {
// 			char lineBuff[128];

// 			configFile.println("# -------------------\r\n# General configuration\r\n# -------------------");
// 			configFile.println("\r\n# mode:sdcard or network");
// 			// sprintf(lineBuff, "mode:%s", modeTitles[config.workingMode]);
// 			configFile.println(lineBuff);
// 			configFile.println("\r\n# publishInterval:period in seconds");
// 			sprintf(lineBuff, "publishInterval:%lu", config.publishInterval);
// 			configFile.println(lineBuff);

// 			configFile.println("\r\n# -------------------\r\n# Network configuration\r\n# -------------------");
// 			sprintf(lineBuff, "ssid:%s", config.ssid);
// 			configFile.println(lineBuff);
// 			sprintf(lineBuff, "pass:%s", config.pass);
// 			configFile.println(lineBuff);
// 			sprintf(lineBuff, "token:%s", config.token);
// 			configFile.println(lineBuff);

// 			configFile.println("\r\n# -------------------\r\n# Sensor configuration\r\n# ej. sensor name:reading interval or disabled\r\n# -------------------\r\n");

// 			bool externalTitlePrinted = false;

// 			// Sensors config
// 			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
// 				SensorType wichSensor = static_cast<SensorType>(i);

// 				if (!externalTitlePrinted && sensors[wichSensor].location == BOARD_AUX) {
// 					configFile.println("\r\n# External sensors (Not included in SCK board)"); 
// 					externalTitlePrinted = true;
// 				}

// 				if (sensors[wichSensor].enabled) sprintf(lineBuff, "%s:%lu", sensors[wichSensor].title, sensors[wichSensor].interval);
// 				else sprintf(lineBuff, "%s:disabled", sensors[wichSensor].title);

// 				configFile.println(lineBuff);
// 			}

// 			configFile.close();
// 			sckOut("Saved configuration to sdcard!!!");
// 		}
// 	}
// }

// ESP
void SckBase::ESPcontrol(ESPcontrols controlCommand) {
	switch(controlCommand){
		case ESP_OFF: {
			sckOut("Turning ESP off...");
			digitalWrite(pinESP_CH_PD, LOW);
			digitalWrite(pinPOWER_ESP, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);
			espStarted = 0;
			break;

		} case ESP_FLASH: {

			sckOut("Putting ESP in flash mode...");

			SerialUSB.begin(ESP_FLASH_SPEED);
			SerialFlashESP.begin(ESP_FLASH_SPEED);

			pinPeripheral(pinESP_TX_WIFI, PIO_SERCOM_ALT);	// PB8 - TXwifi (A1-15 zeroUSB)	SERCOM4 PAD[0]
  			pinPeripheral(pinESP_RX_WIFI, PIO_SERCOM_ALT);	// PB9 - RXwifi (A2-16 zeroUSB)	SERCOM4 PAD[1]

			ESPcontrol(ESP_OFF);

			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
			digitalWrite(pinPOWER_ESP, LOW);

			flashingESP = true;

			led.update(led.WHITE, led.PULSE_STATIC);

			uint32_t flashTimeout = millis();
			uint32_t startTimeout = millis();
			while(1) {
				if (SerialUSB.available()) {
					SerialFlashESP.write(SerialUSB.read());
					flashTimeout = millis();	// If something is received restart timer
				}
				if (SerialFlashESP.available()) {
					SerialUSB.write(SerialFlashESP.read());
				} 
				if (millis() - flashTimeout > 1000) {
					if (millis() - startTimeout > 8000) reset();		// Giva an initial 8 seconds for the flashing to start
				}
			}
			break;

		} case ESP_ON: {

			SerialUSB.begin(serialBaudrate);
			SerialESP.begin(serialBaudrate);

			sckOut("Turning ESP on...");
			digitalWrite(pinESP_CH_PD, HIGH);
			digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
			digitalWrite(pinPOWER_ESP, LOW);
			espStarted = rtc.getEpoch();

			break;

		} case ESP_REBOOT: {
			sckOut("Restarting ESP...");
			ESPcontrol(ESP_OFF);
			delay(10);
			ESPcontrol(ESP_ON);
			break;
		}
	}
}

// SD card
bool SckBase::sdDetect() {

	cardPresent = !digitalRead(pinCARD_DETECT);

	if (cardPresent) {
		sckOut("Sdcard inserted!!");
		// sdSelect();
		// if (sd.cardBegin(pinCS_SDCARD)) {
		// 	sckOut(F("Sdcard ready!!"), PRIO_LOW);
		// 	return true;
		// } else {
		// 	sckOut(F("Sdcard not found!!"));
		// }

	} else {
		
		sckOut("Sdcard removed!!");
	}
	return false;
}
void SckBase::sdSelect() {

	digitalWrite(pinCS_FLASH, HIGH);	// disables Flash
	digitalWrite(pinCS_SDCARD, LOW);
	sd.begin(pinCS_SDCARD);
}
bool SckBase::sdOpenFile(SckFile wichFile, uint8_t oflag) {

	if (cardPresent) {
		sdSelect();
		if (oflag == O_CREAT) sd.remove(wichFile.name);	// Delete the file if we need a new one.
		wichFile.file = sd.open(wichFile.name, oflag);
		return true;
	}
	return false;
}

// Flash memory
void SckBase::flashSelect() {

	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);

	sckOut("Starting flash memory...");

	// Get ID
	// SPI.begin();
	flash.begin(MB(8));
	flash.setClock(70000000);

	uint8_t b1, b2;
	uint32_t JEDEC = flash.getJEDECID();
	uint32_t maxPage = flash.getMaxPage();
	uint32_t capacity = flash.getCapacity();
	b1 = (JEDEC >> 16);
	b2 = (JEDEC >> 8);

	sprintf(outBuff, "JEDEC ID: %u\r\nMax page: %u\r\nCapacity: %u", JEDEC, maxPage, capacity);
	sckOut();



	// uint32_t wTime = 0;
	// uint32_t rTime, addr;
	// uint8_t _data, _d;
	// _d = 35;

	// addr = random(0, 0xFFFFF);
  	flash.eraseSector(35);
	flash.writeChar(35, 32);

	// if (flash.writeByte(addr, _d)) wTime = flash.functionRunTime();
	// _data = flash.readByte(addr);
	// rTime = flash.functionRunTime();

	// SerialUSB.print ("\t\t\tByte: \t\t");
	// if (_data == _d) SerialUSB.println("Data I/O Passed!");
	// else SerialUSB.println("Data I/O ERROR!");
  
	// if (rTime != 0) {
	// 	SerialUSB.print(F("\t\tWrite Time: "));
 //    	SerialUSB.print(wTime);
 //    	SerialUSB.println(" us");
 //    	SerialUSB.print(F(",\tRead Time: "));
 //    	SerialUSB.print(rTime);
 //    	SerialUSB.println(" us");
 //  	} else {
 //    	SerialUSB.print(F("\t\tTime: "));
 //    	SerialUSB.print(wTime);
 //    	SerialUSB.println(" us");;
	// }


  // long long _uniqueID = flash.getUniqueID();
  // sprintf(outBuff, "unique ID: %u", _uniqueID);
  // sckOut();

	// SPI.beginTransaction(_settings);

	// SPI.transfer(SPIFLASH_IDREAD);

	// uint16_t jedecid = SPI.transfer(0) << 8;
	// jedecid |= SPI.transfer(0);
}
// bool SckBase::flashOpenFile(SckFile wichFile, uint8_t oflag) {

// 	if (flashBegin()) {
// 		// if (oflag == O_CREAT) flash.remove(wichFile.name);	// Delete the file if we need a new one.
// 		// wichFile.file = flash.open(wichFile.name, oflag);
// 		return true;
// 	}
// 	return false;
// }

// Power
bool SckBase::battSetup() {

	pinMode(pinBATTERY_ALARM, INPUT_PULLUP);
	LowPower.attachInterruptWakeup(pinBATTERY_ALARM, ISR_battery, CHANGE);
	
	if (lipo.begin()) {
		
		lipo.enterConfig();
		lipo.setCapacity(battCapacity);
		lipo.setGPOUTPolarity(LOW);
		lipo.setGPOUTFunction(SOC_INT);
		lipo.setSOCIDelta(1);
		lipo.exitConfig();

		// Force an update
		batteryEvent();
		return true;
	}
	return false;
}
void SckBase::batteryEvent(){

	getReading(SENSOR_BATT_PERCENT);

	if (sensors[SENSOR_BATT_PERCENT].reading.toInt() != 0) {
		batteryPresent = true;
		sprintf(outBuff, "Battery charge %s%%", sensors[SENSOR_BATT_PERCENT].reading.c_str());
	} else {

		// TODO
		// To confirm no battery is present we should check the state of charger here

		batteryPresent = false;
		sprintf(outBuff, "No battery present!!");
	}
	sckOut();
}
void SckBase::batteryReport() {

	sprintf(outBuff, "Charge: %u %%\r\nVoltage: %u V\r\nCharge current: %i mA\r\nCapacity: %u/%u mAh\r\nState of health: %i",
		lipo.soc(),
		lipo.voltage(),
		lipo.current(AVG),
		lipo.capacity(REMAIN),
		lipo.capacity(FULL),
		lipo.soh()
	);
	sckOut();
}
void SckBase::reset() {
	sckOut("Bye!!");
	NVIC_SystemReset();
}

// Sensors
bool SckBase::getReading(SensorType wichSensor, bool wait) {

	sensors[wichSensor].valid = false;
	String result = "none";

	switch (sensors[wichSensor].location) {
		case BOARD_BASE: {
			switch (wichSensor) {
				case SENSOR_BATT_PERCENT: result = String(lipo.soc()); break;
				case SENSOR_BATT_VOLTAGE: result = String(lipo.voltage()); break;
				case SENSOR_BATT_CHARGE_RATE: result = String(lipo.current(AVG)); break;
				case SENSOR_VOLTIN: {

					break;
				} default: break;
			}
			break;
		} case BOARD_URBAN: {
			result = urban.getReading(wichSensor, wait);
			if (result.startsWith("none")) return false;
			break;
		}
		case BOARD_AUX: break;

	}

	sensors[wichSensor].reading = result;
	sensors[wichSensor].lastReadingTime = rtc.getEpoch();
	sensors[wichSensor].valid = true;
	return true;;
}