#include "SckBase.h"
#include "Commands.h"

// Hardware Auxiliary I2C bus
TwoWire auxWire(&sercom1, pinAUX_WIRE_SDA, pinAUX_WIRE_SCL);
void SERCOM1_Handler(void) {
	auxWire.onService();
}

// ESP communication
SckSerial serESP(SerialESP);

// Auxiliary I2C devices
AuxBoards auxBoards;

// Eeprom flash emulation to store persistent variables
FlashStorage(eepromConfig, Configuration);

void SckBase::setup()
{
	// Led
	led.setup();

	// ESP Configuration
	pinMode(pinPOWER_ESP, OUTPUT);
	pinMode(pinESP_CH_PD, OUTPUT);
	pinMode(pinESP_GPIO0, OUTPUT);
	serESP.begin();
	serESPBuffPtr = serESP.buff;
	ESPcontrol(ESP_OFF);

	// Internal I2C bus setup
	Wire.begin();

	// Button interrupt and wakeup
	pinMode(pinBUTTON, INPUT_PULLUP);
	attachInterrupt(pinBUTTON, ISR_button, CHANGE);
	EExt_Interrupts in = g_APinDescription[pinBUTTON].ulExtInt;
	configGCLK6();
	EIC->WAKEUP.reg |= (1 << in);

	// RTC setup
	rtc.begin();
	uint32_t now = rtc.getEpoch();
	if (rtc.isConfigured() && (now > 1514764800)) st.timeStat.setOk();	// If greater than 01/01/2018
	else {
		rtc.setTime(0, 0, 0);
		rtc.setDate(1, 1, 15);
	}
	espStarted = now;

	// Flash storage
	sckOut("Starting flash memory...");
	wichGroupPublishing.group = -1;  	// No group is being published yet
	int8_t rcode = readingsList.setup();
	if (rcode == 1) sckOut("Found problems on flash memory, it was formated...");
	else if (rcode == -1) {
        led.update(led.WHITE, led.PULSE_ERROR);
		while (true) {
			sckOut("Error starting flash memory!!!");
			delay(1000);
		}
	}

/* #define autoTest  // Uncomment for doing Gases autotest, you also need to uncomment  TODO complete this */

#ifdef autoTest
    // TODO verify led blinking...
    ESPcontrol(ESP_OFF);
    sckOut("Starting Gases Pro Board automated test...", PRIO_HIGH);
    led.off();
    led.update(led.BLUE, led.PULSE_STATIC);
    String testResult = auxBoards.control(SENSOR_GASESBOARD_SLOT_1W, "autotest");
    SerialUSB.println(testResult);
    if (testResult.startsWith("1")) {
        sckOut("Test finished OK!!");
        led.update(led.GREEN, led.PULSE_STATIC);
    } else {
        sckOut("ERROR Test failed, please check your connections");
        led.update(led.RED, led.PULSE_STATIC);
    }
    while(true);
#endif

    // Power management configuration
    charger.setup(this);
    battery.setup();

    // Configuration
    loadConfig();
    if (st.mode == MODE_NOT_CONFIGURED) writeHeader = true;

    // Urban board
    urbanStart();

    // Auxiliary I2C bus
    pinMode(pinPOWER_AUX_WIRE, OUTPUT);
    digitalWrite(pinPOWER_AUX_WIRE, LOW);   // LOW -> ON , HIGH -> OFF
    pinPeripheral(pinAUX_WIRE_SDA, PIO_SERCOM);
    pinPeripheral(pinAUX_WIRE_SCL, PIO_SERCOM);
    auxWire.begin();
    delay(3000);                // Give some time for external boards to boot


    // SDcard and flash select pins
    pinMode(pinCS_SDCARD, OUTPUT);
    pinMode(pinCS_FLASH, OUTPUT);
    digitalWrite(pinCS_SDCARD, HIGH);
    digitalWrite(pinCS_FLASH, HIGH);
    pinMode(pinCARD_DETECT, INPUT_PULLUP);

    // SD card
    sckOut("Setting up SDcard interrupt");
    attachInterrupt(pinCARD_DETECT, ISR_sdDetect, CHANGE);
    if (sdDetect()) sdInit();


    // Detect and enable auxiliary boards
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {
        OneSensor *wichSensor = &sensors[sensors.sensorsPriorized(i)];
        if (wichSensor->location == BOARD_AUX) {
            if (config.sensors[wichSensor->type].enabled) {
                enableSensor(wichSensor->type);
            } else {
                wichSensor->enabled = false;
#ifdef WITH_SENSOR_GROVE_OLED
                wichSensor->oled_display = false;
#endif
            }
        }
    }

    sckOut("Turning on WiFi...");
    ESPcontrol(ESP_ON);

    // Update battery parcent for power management stuff
    battery.percent(&charger);

    // After sanity reset go directly to sleep
    if (rtc.getHours() == wakeUP_H && rtc.getMinutes() == wakeUP_M) lastUserEvent = 0;
}
void SckBase::update()
{
    if (millis() - reviewStateMillis > 250) {
        reviewStateMillis = millis();
        reviewState();
    }

    if (millis() - updatePowerMillis > 1000) {
        updatePowerMillis = millis();
        updatePower();
    }

    if (butState != butOldState) {
        buttonEvent();
        butOldState = butState;
        while(!butState) buttonStillDown();
    }

    if (millis() - generalUpdateTimer > 500) {

        // Avoid ESP hangs
        if (st.espBooting) {
            if (rtc.getEpoch() - espStarted > 3) ESPcontrol(ESP_REBOOT);
        }

        if (pendingSyncConfig) {
            if (millis() - sendConfigTimer > 1000) {
                sendConfigTimer = millis();
                if (sendConfigCounter > 3) {
                    ESPcontrol(ESP_REBOOT);
                    sendConfigCounter = 0;
                } else if (st.espON) {
                    if (!st.espBooting) sendConfig();
                    sendConfigCounter++;
                } else {
                    ESPcontrol(ESP_ON);
                }
            }
        }

        if (sdInitPending) sdInit();

        // SD card debug check file size and backup big files.
        if (config.debug.sdcard) {
            // Just do this every hour
            if (rtc.getEpoch() % 3600 == 0) {
                sckFile.open(SD_DEBUG_NAME, FILE_WRITE);
                if (sckFile) {

                    uint32_t debugSize = sckFile.size();

                    // If file is bigger than 50mb rename the file.
                    if (debugSize >= 52428800) sckFile.rename("DEBUG01.TXT");
                    sckFile.close();

                } else {
                    st.cardPresent = false;
                    st.cardPresentErrorPrinted = false;
                }

            }
        }

#ifdef WITH_GPS
        // If we have a GPS update it and get time if needed
        if (sensors[SENSOR_GPS_FIX_QUALITY].enabled){
            auxBoards.updateGPS();
            if (!st.timeStat.ok) getReading(&sensors[SENSOR_GPS_FIX_QUALITY]);
        }
#endif

#ifdef WITH_SENSOR_GROVE_OLED
        // If we have a screen update it
        if (sensors[SENSOR_GROVE_OLED].enabled) auxBoards.updateDisplay(this);
#endif
    }
}

// **** Mode Control
void SckBase::reviewState()
{
    /* struct SckState { */
    /* bool onSetup --  in from enterSetup() and out from saveConfig()*/
    /* bool espON */
    /* bool wifiSet */
    /* bool tokenSet */
    /* bool helloPending */
    /* SCKmodes mode */
    /* bool cardPresent */
    /* bool sleeping */
    /* }; */

    /* state can be changed by: */
    /* loadConfig() */
    /* receiveMessage() */
    /* sdDetect() */
    /* buttonEvent(); */

	// update ESP serial communication error status
	if (serESP.error) {
        st.error = ERROR_ESP;
        if (st.mode == MODE_SD) led.update(led.PINK, led.PULSE_ERROR);
        else if (st.onSetup) led.update(led.RED, led.PULSE_ERROR);
        else led.update(led.BLUE, led.PULSE_ERROR);
    }

	if (st.onShell) {

        led.update(led.YELLOW, led.PULSE_STATIC);

    } else if (st.onSetup) {

    } else if (sckOFF) {

    } else if (st.mode == MODE_NOT_CONFIGURED) {

        if (!st.onSetup) enterSetup();

    } else if (st.mode == MODE_NET || st.mode == MODE_SD) {

        if (!st.timeStat.ok) {

            // This error needs user intervention
            if (!st.wifiSet) {
                if (!st.wifiStat.error) {
                    sckOut("Time not synced and wifi is not configured!!!", PRIO_ERROR);
                    ESPcontrol(ESP_OFF);
                    if (st.mode == MODE_SD) led.update(led.PINK, led.PULSE_ERROR);
                    else led.update(led.BLUE, led.PULSE_ERROR);
                    st.error = ERROR_NO_WIFI_CONFIG;
                    st.wifiStat.error = true;
                }
                return;
            }

            if (!st.wifiStat.ok) {

                if (st.wifiStat.retry()) {          // After triggering this we have 60 seconds until error is declared, unless the ESP sends an error msg

                    sckOut("Time not synced, connecting to WiFi...");
                    if (!st.espON) ESPcontrol(ESP_ON);  // Make sure the ESP is on

                } else if (st.wifiStat.error) {         // If error is declared something went wrong

                    // This error needs user intervention so feedback should be urgent
                    if (st.error != ERROR_TIME) sckOut("Without time we can not take readings!!!", PRIO_ERROR);
                    if (st.mode == MODE_SD) led.update(led.PINK, led.PULSE_ERROR);
                    else led.update(led.BLUE, led.PULSE_ERROR);
                    st.error = ERROR_TIME;
                }

            } else if (st.timeStat.retry()) {

				if (ESPsend(ESPMES_GET_TIME, "")) sckOut("Asking time to ESP...");

            } else if (st.timeStat.error) {

                if (st.error != ERROR_TIME) sckOut("Getting time from the network!!!", PRIO_ERROR);

                ESPcontrol(ESP_REBOOT);             // This also resets st.wifiStat
                if (st.mode == MODE_SD) led.update(led.PINK, led.PULSE_ERROR);
                else led.update(led.BLUE, led.PULSE_ERROR);
                led.update(led.BLUE, led.PULSE_ERROR);
                st.error = ERROR_TIME;
                st.timeStat.reset();
            }

            return;
        }

        if (st.mode == MODE_NET) {

            // If We need conection now
            if (st.helloPending || timeToPublish || !infoPublished) {

                if (!st.tokenSet) {

                    if (!st.tokenError) {
                        sckOut("Token is not configured!!!", PRIO_ERROR);
                        ESPcontrol(ESP_OFF);
                        led.update(led.BLUE, led.PULSE_WARNING);
                        st.error = ERROR_NO_TOKEN_CONFIG;
                        st.tokenError = true;
                    }

                } else if (!st.wifiStat.ok) {

                    // After triggering this we have 60 seconds until error is declared, unless the ESP sends an error msg
                    if (st.wifiStat.retry()) {

                        sckOut("Connecting to Wifi...");
                        if (!st.espON) ESPcontrol(ESP_ON);  // Make sure the ESP is on

                    } else if (st.wifiStat.error) {         // If error is declared something went wrong


                        uint32_t now = rtc.getEpoch();

                        // If error just happened don't go to sleep yet
                        if (st.lastWiFiError == 0) {

                            ESPcontrol(ESP_OFF);                // Save battery
                            infoPublished = true;               // We will try on next boot, publishing info is not high priority
                            st.lastWiFiError = now;             // Start counting time
                            st.wifiErrorCounter++;              // Count errors

                            if (st.helloPending) {
                                sckOut("ERROR: Couldn't send hello to platform! ");
                                sckOut("No readings will be taken!!");
                                led.update(led.BLUE, led.PULSE_ERROR);
                            } else {
                                sckOut("ERROR Can't publish without wifi!!!");  // User feedback
                                led.update(led.BLUE, led.PULSE_WARNING);
                            }

                            // Retry WiFi to be sure that is not working
                        } else if ( (now - st.lastWiFiError) > config.offline.retry ||      // Enough time has passed to try again
                                st.wifiErrorCounter < 2 ||              // Try 2 times before assuming WiFi is no present
                                millis() - lastUserEvent < 1000             // User event in the last second, this shouldn't enter more than once because wifi error declaration takes a lot more than one second
                            ) {

                            // Reset and try again
                            st.lastWiFiError = 0;
                            st.wifiStat.reset();
                            sckOut("Retrying WiFi...");             // User feedback

                            // Asume WiFi is down or not reachable
                        } else {

                            if (st.espON) ESPcontrol(ESP_OFF);              // Save battery
                            timeToPublish = false;
                            lastPublishTime = rtc.getEpoch();       // Wait for another period before retry
                            sleepLoop();
                        }
                    }

                } else {

                    led.update(led.BLUE, led.PULSE_SOFT);
                    st.wifiErrorCounter = 0;
                    st.error = ERROR_NONE;

                    if (st.helloPending) {

                        if (st.helloStat.retry()) {

							if (ESPsend(ESPMES_MQTT_HELLO, ""))	sckOut("Hello sent!");

                        } else if (st.helloStat.error) {

                            sckOut("Sending hello!!!", PRIO_ERROR);

                            ESPcontrol(ESP_REBOOT);             // Try reseting ESP
                            led.update(led.BLUE, led.PULSE_ERROR);  // This error is an exception (normally it would be Soft error) because in this case the user is probably doing the onboarding process
                            st.error = ERROR_MQTT;

                            st.helloStat.reset();
                        }

                    } else if (!infoPublished) {

                        if (st.infoStat.retry()) {

                            if (publishInfo()) sckOut("Info sent!");

                        } else if (st.infoStat.error){

                            sckOut("Sending kit info to platform!!!", PRIO_ERROR);
                            infoPublished = true;       // We will try on next reset
                            st.infoStat.reset();
                            st.error = ERROR_MQTT;

                        }

                    } else if (timeToPublish) {

                        if (st.publishStat.ok) {

                            lastPublishTime = rtc.getEpoch();
                            st.publishStat.reset();         // Restart publish error counter

                            // Mark reading as published
                            uint8_t readingNum = readingsList.setPublished(wichGroupPublishing, readingsList.PUB_NET);
                            wichGroupPublishing.group = -1;
                            timeToPublish = false;
                            sprintf(outBuff, "Network publish OK!! (%u readings)", readingNum);
                            sckOut();

                            if (readingsList.availableReadings[readingsList.PUB_NET]) {

                                if (st.publishStat.retry()) netPublish();

                            } else {

                                // Turn off WiFi if is not going to be used soon
                                if (config.publishInterval >= 60) {
                                    ESPcontrol(ESP_OFF);
                                    st.wifiStat.reset();
                                }
                            }

                        } else if (st.publishStat.error) {

                            sckOut("Publish failed, will retry on next interval!!!", PRIO_ERROR);

                            // Forget the index of the group we tried to publish
                            wichGroupPublishing.group = -1;

                            led.update(led.BLUE, led.PULSE_WARNING);
                            st.error = ERROR_MQTT;

                            ESPcontrol(ESP_OFF);
                            timeToPublish = false;
                            lastPublishTime = rtc.getEpoch();
                            st.publishStat.reset();         // Restart publish error counter

                        } else {
                            if (st.publishStat.retry()) netPublish();
                        }
                    }
                }
            } else {

            updateSensors();
            sleepLoop();
        }

        } else if  (st.mode == MODE_SD) {

        updateSensors();

        if (st.espON && !pendingSyncConfig) ESPcontrol(ESP_OFF);

        if (!st.cardPresent && !st.cardPresentErrorPrinted) {
            if (sdDetect()) sdInit();   // Retry sd card init
        }

        if (!st.cardPresent) {

            if (!st.cardPresentErrorPrinted) {
                // Saving error on sdcard here does not makes sense, but other error log outputs (mqtt?) would be implemented
                sckOut("Can't find SD card!!!", PRIO_ERROR);
                led.update(led.PINK, led.PULSE_WARNING);
                st.error = ERROR_SD;
                st.cardPresentErrorPrinted = true;
            }
            return;

        } else {

            st.error = ERROR_NONE;
            led.update(led.PINK, led.PULSE_SOFT);
            sleepLoop();
        }
    }
    }
}
void SckBase::enterSetup()
{
    sckOut("Entering setup mode", PRIO_LOW);
    st.onSetup = true;

    // Update led
    led.update(led.RED, led.PULSE_SOFT);
    st.error = ERROR_NONE;

    // Clear errors from other modes
    st.tokenError = false;
    st.cardPresentErrorPrinted = false;

    // Reboot ESP to have a clean start
    ESPcontrol(ESP_REBOOT);
}

// **** Input
void SckBase::inputUpdate()
{

    if (SerialUSB.available()) {

        char buff = SerialUSB.read();
        uint16_t blen = serialBuff.length();

        // New line
        if (buff == 13 || buff == 10) {

            SerialUSB.println();                // Newline

            serialBuff.replace("\n", "");       // Clean input
            serialBuff.replace("\r", "");
            serialBuff.trim();

            commands.in(this, serialBuff);      // Process input
            if (blen > 0) previousCommand = serialBuff;
            serialBuff = "";
            prompt();

            // Backspace
        } else if (buff == 127) {

            if (blen > 0) SerialUSB.print("\b \b");
            serialBuff.remove(blen-1);

            // Up arrow (previous command)
        } else if (buff == 27) {

            delayMicroseconds(200);
            SerialUSB.read();               // drop next char (always 91)
            delayMicroseconds(200);
            char tmp = SerialUSB.read();
            if (tmp != 65) tmp = SerialUSB.read(); // detect up arrow
            else {
                for (uint8_t i=0; i<blen; i++) SerialUSB.print("\b \b");    // clean previous command
                SerialUSB.print(previousCommand);
                serialBuff = previousCommand;
            }

            // Normal char
        } else {

            serialBuff += buff;
            SerialUSB.print(buff);              // Echo

        }
    }

    ESPbusUpdate();
}

// **** Output
void SckBase::sckOut(const char *strOut, PrioLevels priority, bool newLine)
{
    if (strncmp(strOut, outBuff, strlen(strOut)) == 0 && (config.outLevel + priority > 1)) {
        outRepetitions++;
        if (outRepetitions >= 10) {
            sckOut("Last message repeated 10 times");
            outRepetitions = 0;
        }
        return;
    }
    outRepetitions = 0;

    if (priority == PRIO_ERROR) snprintf(outBuff, 240, "ERROR: %s", strOut);
    else strncpy(outBuff, strOut, 240);

    sckOut(priority, newLine);
}
void SckBase::sckOut(PrioLevels priority, bool newLine)
{
    // Output via USB console
    if (charger.onUSB) {
        if (config.outLevel + priority > 1) {
            if (newLine) SerialUSB.println(outBuff);
            else SerialUSB.print(outBuff);
        }
    } else  {
        digitalWrite(pinLED_USB, HIGH);
    }

    // Debug output to sdcard
    if (config.debug.sdcard) {
        sckFile.open(SD_DEBUG_NAME, FILE_WRITE);
        if (sckFile) {
            ISOtime();
            sckFile.print(ISOtimeBuff);
            sckFile.print("-->");
            sckFile.println(outBuff);
            sckFile.close();
        } else st.cardPresent = false;
    }

    // Append to Error log on sdcard
    if (priority == PRIO_ERROR) {
        sckFile.open(SD_ERROR_NAME, FILE_WRITE);
        if (sckFile) {
            ISOtime();
            sckFile.print(ISOtimeBuff);
            sckFile.print("-->");
            sckFile.println(outBuff);
            sckFile.close();
        } else st.cardPresent = false;
    }

#ifdef WITH_SENSOR_GROVE_OLED
    // Debug output to oled display
    if (config.debug.oled) {
        if (sensors[SENSOR_GROVE_OLED].enabled) auxBoards.print(outBuff);
    }
#endif
}
void SckBase::prompt()
{
    sprintf(outBuff, "%s", "SCK > ");
    sckOut(PRIO_MED, false);
}
#ifdef WITH_SENSOR_GROVE_OLED
void SckBase::plot(String value, const char *title, const char *unit)
{
    auxBoards.plot(value, title, unit);
}
#endif

// **** Config
void SckBase::loadConfig()
{
	sckOut("Loading configuration from eeprom...");

	Configuration savedConf = eepromConfig.read();

	if (savedConf.valid) config = savedConf;
	else {
		sckOut("Can't find valid configuration!!! loading defaults...");
		saveConfig(true);
	}

	// Load saved intervals
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {
		OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
		wichSensor->everyNint = config.sensors[wichSensor->type].everyNint;
	}

	st.wifiSet = config.credentials.set;
	st.tokenSet = config.token.set;
	st.tokenError = false;
	st.mode = config.mode;
	readingsList.debug = config.debug.flash;
	serESP.debug = config.debug.serial;
    led.brightness = config.ledBrightness;

	snprintf(hostname, sizeof(hostname), "%s", "Smartcitizen");
	memcpy(&hostname[12], &config.mac.address[12], 2);
	memcpy(&hostname[14], &config.mac.address[15], 2);
	hostname[16] = '\0';

#ifdef WITH_URBAN

#ifdef WITH_CCS811
	// CSS vocs sensor baseline loading
	if (config.extra.ccsBaselineValid && I2Cdetect(&Wire, urban.sck_ccs811.address)) {
		sprintf(outBuff, "Updating CCS sensor baseline: %u", config.extra.ccsBaseline);
		sckOut();
		urban.sck_ccs811.setBaseline(config.extra.ccsBaseline);
	}
#endif

#ifdef WITH_PMS
	// PMS sensor warmUpperiod and powerSave config
	urban.sck_pms.warmUpPeriod = config.extra.pmWarmUpPeriod;
	urban.sck_pms.powerSave = config.extra.pmPowerSave;
#endif
#endif
}
void SckBase::saveConfig(bool defaults)
{
	// Load defaults
	if (defaults) {
		Configuration defaultConfig;

        config = defaultConfig;

        for (uint8_t i=0; i<SENSOR_COUNT; i++) {

            SensorType wichSensorType = static_cast<SensorType>(i);

            config.sensors[wichSensorType].enabled = sensors[wichSensorType].defaultEnabled;
#ifdef WITH_SENSOR_GROVE_OLED
            config.sensors[wichSensorType].oled_display = true;
#endif
            config.sensors[wichSensorType].everyNint = sensors[wichSensorType].defaultEveryNint;
        }
        pendingSyncConfig = true;
    }

    // Sensor enabled/disabled state is only saved if it is setted in config.sensors the runtime state (sensors) is not saved.
    // This means that if you want to make sensor state persistent you have to change explicitly config.sensors

	// Save to eeprom
	eepromConfig.write(config);
	sckOut("Saved configuration on eeprom!!", PRIO_LOW);
	lastUserEvent = millis();

    // Update state
    st.mode = config.mode;
    st.wifiSet = config.credentials.set;
    st.tokenSet = config.token.set;
    st.tokenError = false;
    st.wifiStat.reset();

    uint32_t now = rtc.getEpoch();
    lastPublishTime = now - config.publishInterval;
    lastSensorUpdate = now - config.readInterval;

    readingsList.flashUpdate();     // Scan flash memory and update indexes

    if (st.wifiSet || st.tokenSet) pendingSyncConfig = true;

    // Decide if new mode its valid
    if (st.mode == MODE_NET) {

        if (st.wifiSet && st.tokenSet) {

            infoPublished = false;
            st.helloPending = true;
            st.onSetup = false;
            led.update(led.BLUE, led.PULSE_SOFT);
            st.error = ERROR_NONE;
            ESPcontrol(ESP_REBOOT);

        } else {

            if (!st.wifiSet) sckOut("Wifi not configured: can't set Network Mode!!!", PRIO_ERROR);
            if (!st.tokenSet) sckOut("Token not configured: can't set Network Mode!!!", PRIO_ERROR);
            ESPcontrol(ESP_OFF);
            led.update(led.BLUE, led.PULSE_ERROR);
            st.error = ERROR_NO_WIFI_CONFIG;
        }

    } else if (st.mode == MODE_SD) {

		st.helloPending = false;
		st.onSetup = false;
		led.update(led.PINK, led.PULSE_SOFT);
		ESPsend(ESPMES_STOP_AP, "");
		st.error = ERROR_NONE;

    }

    if (pendingSyncConfig && !st.espON) ESPcontrol(ESP_ON);
}
Configuration SckBase::getConfig()
{

    return config;
}
bool SckBase::sendConfig()
{
	if (!st.espON) {
		ESPcontrol(ESP_ON);
		return false;
	}
	if (st.espBooting) return false;

	StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
	JsonObject json = jsonBuffer.to<JsonObject>();

	json["cs"] = (uint8_t)config.credentials.set;
	json["ss"] = config.credentials.ssid;
	json["pa"] = config.credentials.pass;
	json["ts"] = (uint8_t)config.token.set;
	json["to"] = config.token.token;
	json["ms"] = config.mqtt.server;
	json["mp"] = config.mqtt.port;
	json["ns"] = config.ntp.server;
	json["np"] = config.ntp.port;
	json["ver"] = SAMversion;
	json["bd"] = SAMbuildDate;
	json["sd"] = (uint8_t)config.debug.serial;
    json["lb"] = config.ledBrightness;

	if (!st.onSetup && ((st.mode == MODE_NET && st.wifiSet && st.tokenSet) || (st.mode == MODE_SD && st.wifiSet))) json["ac"] = (uint8_t)ESPMES_CONNECT;
	else json["ac"] = (uint8_t)ESPMES_START_AP;

	serializeJson(json, serESP.buff, NETBUFF_SIZE);

	if (ESPsend(ESPMES_SET_CONFIG)) {
		pendingSyncConfig = false;
		sendConfigCounter = 0;
		sckOut("Synced config with ESP!!", PRIO_LOW);
		return true;
	}

	return false;
}
bool SckBase::createInfo(char* buffer, bool pretty)
{
    /* { */
    /* 	"time":"2018-07-17T06:55:06Z", */
    /* 	"hw_ver":"2.0", */
    /* 	"id":"6C4C1AF4504E4B4B372E314AFF031619", */
    /* 	"sam_ver":"0.3.0-ce87e64", */
    /* 	"sam_bd":"2018-07-17T06:55:06Z", */
    /* 	"mac":"AB:45:2D:33:98", */
    /* 	"esp_ver":"0.3.0-ce87e64", */
    /* 	"esp_bd":"2018-07-17T06:55:06Z" */
    /*  "rcause":"POR" */
    /* } */

    ISOtime();
    getUniqueID();

    StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
    JsonObject json = jsonBuffer.to<JsonObject>();

    json["time"] = ISOtimeBuff;
    json["hw_ver"] = hardwareVer.c_str();
    json["id"] = uniqueID_str;
    json["sam_ver"] = SAMversion.c_str();
    json["sam_bd"] = SAMbuildDate.c_str();
    json["mac"] = config.mac.address;
    json["esp_ver"] = ESPversion.c_str();
    json["esp_bd"] = ESPbuildDate.c_str();

    uint8_t resetCause = PM->RCAUSE.reg;
    switch(resetCause){
        case 1:
            json["rcause"] = "POR";         // Power On Reset
            break;
        case 2:
            json["rcause"] = "BOD12";       // Brown Out 12 Detector Reset
            break;
        case 4:
            json["rcause"] = "BOD33";       // Brown Out 33 Detector Reset
            break;
        case 16:
            json["rcause"] = "EXT";         // External Reset");
            break;
        case 32:
            json["rcause"] = "WDT";         // Watchdog Reset");
            break;
        case 64:
            json["rcause"] = "SYST";        // System Reset Request");
            break;
    }

    if (pretty) serializeJsonPretty(json, buffer, NETBUFF_SIZE);
    else serializeJson(json, buffer, NETBUFF_SIZE);
}
bool SckBase::publishInfo()
{
    // Info file
    if (!espInfoUpdated) ESPsend(ESPMES_GET_NETINFO, "");
    else {

        if (!st.espON) {
            ESPcontrol(ESP_ON);
            return false;
        }

        if (!createInfo(serESP.buff)) return false;
        if (ESPsend(ESPMES_MQTT_INFO)) return true;
    }
    return false;
}

// **** ESP
void SckBase::ESPcontrol(ESPcontrols controlCommand)
{
	switch(controlCommand){
		case ESP_OFF:
		{
				sckOut("ESP off...", PRIO_LOW);
				st.espON = false;
				st.espBooting = false;
				digitalWrite(pinESP_CH_PD, LOW);
				digitalWrite(pinPOWER_ESP, HIGH);
				digitalWrite(pinESP_GPIO0, LOW);
				sprintf(outBuff, "Esp was on for %lu seconds", (rtc.getEpoch() - espStarted));
				espStarted = 0;
				break;
		}
		case ESP_FLASH:
		{
				led.update(led.WHITE, led.PULSE_STATIC);

				SerialESP.begin(espFlashSpeed);
				delay(100);

				digitalWrite(pinESP_CH_PD, LOW);
				digitalWrite(pinPOWER_ESP, HIGH);
				digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
				delay(100);

				digitalWrite(pinESP_CH_PD, HIGH);
				digitalWrite(pinPOWER_ESP, LOW);

				uint32_t flashTimeout = millis();
				uint32_t startTimeout = millis();
				while(1) {
					if (SerialUSB.available()) {
						SerialESP.write(SerialUSB.read());
						flashTimeout = millis();
					}
					if (SerialESP.available()) {
						SerialUSB.write(SerialESP.read());
					}
					if (millis() - flashTimeout > 1000) {
						if (millis() - startTimeout > 10000) sck_reset();  // Initial 10 seconds for the flashing to start
					}
				}
				break;
		}
		case ESP_ON:
		{
				if (st.espBooting || st.espON) return;
				digitalWrite(pinESP_CH_PD, HIGH);
				digitalWrite(pinESP_GPIO0, HIGH);		// HIGH for normal mode
				digitalWrite(pinPOWER_ESP, LOW);
				st.wifiStat.reset();
				st.espON = true;
				st.espBooting = true;
				espStarted = rtc.getEpoch();

				// Wait for boot...
				uint32_t startPoint = millis();
				while (st.espBooting) {
					if (millis() - startPoint > 1000) {
						sckOut("ESP not starting!!!", PRIO_HIGH);
						st.error = ERROR_ESP;
						break;
					}
					ESPbusUpdate();
				}
				sckOut("ESP on", PRIO_LOW);
				break;
		}
		case ESP_REBOOT:
		{
				sckOut("Restarting ESP...", PRIO_LOW);
				ESPcontrol(ESP_OFF);
				delay(50);
				ESPcontrol(ESP_ON);
				break;
		}
		case ESP_WAKEUP:
		{
				if (st.espBooting || st.espON) return;
				sckOut("ESP wake up...");
				digitalWrite(pinESP_CH_PD, HIGH);
				st.espON = true;
				espStarted = rtc.getEpoch();
				break;
		}
		case ESP_SLEEP:
		{
				sckOut("ESP deep sleep...", PRIO_LOW);
				ESPsend(ESPMES_LED_OFF, "");
				st.espON = false;
				st.espBooting = false;
				digitalWrite(pinESP_CH_PD, LOW);
				sprintf(outBuff, "Esp was awake for %lu seconds", (rtc.getEpoch() - espStarted));
				sckOut(PRIO_LOW);
				espStarted = 0;
				break;
		}
	}
}
bool SckBase::ESPsend(SCKMessage wichMessage)
{
	if (!st.espON) ESPcontrol(ESP_ON);

	return serESP.send(wichMessage);
}
bool SckBase::ESPsend(SCKMessage wichMessage, const char *content)
{
	// Check if message fits on buffer
	if (strlen(content) > NETBUFF_SIZE) return false;

	// Fills buffer with content
	sprintf(serESP.buff, "%s", content);

	return ESPsend(wichMessage);
}
void SckBase::ESPbusUpdate()
{
	if (!serESP.receive()) return;

	switch(serESP.msg) {
		case SAMMES_SET_CONFIG:
		{
				sckOut("Received new config from ESP");
				StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
				deserializeJson(jsonBuffer, serESP.buff);
				JsonObject json = jsonBuffer.as<JsonObject>();

				if (json.containsKey("mo")) {
					String stringMode = json["mo"];
					if (stringMode.startsWith("net")) config.mode = MODE_NET;
					else if (stringMode.startsWith("sd")) config.mode = MODE_SD;
				} else config.mode = MODE_NOT_CONFIGURED;

				if (json.containsKey("pi")) {
					if (json["pi"] > minimal_publish_interval && json["pi"] < max_publish_interval)	config.publishInterval = json["pi"];
				} else config.publishInterval = default_publish_interval;

				if (json.containsKey("ss")) {
					config.credentials.set = true;
					strcpy(config.credentials.ssid, json["ss"]);
					if (json.containsKey("pa")) strcpy(config.credentials.pass, json["pa"]);
				} else config.credentials.set = false;


				if (json.containsKey("to")) {
					config.token.set = true;
					strcpy(config.token.token, json["to"]);
				} else config.token.set = false;

				st.helloPending = true;
				saveConfig();
				break;

		}
		case SAMMES_NETINFO:
		{
				StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
				deserializeJson(jsonBuffer, serESP.buff);
				JsonObject json = jsonBuffer.as<JsonObject>();

				ipAddress = json["ip"].as<String>();

				sprintf(outBuff, "\r\nHostname: %s\r\nIP address: %s\r\nMAC address: %s", hostname, ipAddress.c_str(), config.mac.address);
				sckOut();
				sprintf(outBuff, "ESP version: %s\r\nESP build date: %s", ESPversion.c_str(), ESPbuildDate.c_str());
				sckOut();

				break;
		}
		case SAMMES_WIFI_CONNECTED:

			st.wifiStat.setOk();
            sensors[SENSOR_RSSI].reading = serESP.buff;
            lastRSSIUpdate = rtc.getEpoch();

			snprintf(outBuff, 240, "Connected to wifi!!- RSSI: %s", serESP.buff);
            sckOut();
			if (!timeSyncAfterBoot) {
				if (ESPsend(ESPMES_GET_TIME, "")) sckOut("Asked new time sync to ESP...");
			}
			break;

		case SAMMES_SSID_ERROR:

			sckOut("Access point not found!!", PRIO_ERROR);
			st.wifiStat.error = true;
			st.error = ERROR_AP;
			break;

		case SAMMES_PASS_ERROR:

			sckOut("Wrong wifi password!!", PRIO_ERROR);
			st.wifiStat.error = true;
			st.error = ERROR_PASS;
			break;

		case SAMMES_WIFI_UNKNOWN_ERROR:

			sckOut("Unknown wifi error!!", PRIO_ERROR);
			st.wifiStat.error = true;
			st.error = ERROR_WIFI_UNKNOWN;
			break;

		case SAMMES_TIME:
		{
				String strTime = String(serESP.buff);
				setTime(strTime);
				saveInfo();
				break;
		}
		case SAMMES_MQTT_HELLO_OK:
		{
				st.helloPending = false;
				st.helloStat.setOk();
				sckOut("Hello OK!!");
				break;
		}
		case SAMMES_MQTT_PUBLISH_OK:

			st.publishStat.setOk();
			break;

		case SAMMES_MQTT_PUBLISH_ERROR:

			sckOut("MQTT publish failed!!", PRIO_ERROR);
			st.publishStat.error = true;
			st.error = ERROR_MQTT;
			break;

		case SAMMES_MQTT_INFO_OK:

			st.infoStat.setOk();
			infoPublished = true;
			sckOut("Info publish OK!!");
			break;

		case SAMMES_MQTT_INFO_ERROR:

			st.infoStat.error = true;
			sckOut("Info publish failed!!", PRIO_ERROR);
			st.error = ERROR_MQTT;
			break;

		case SAMMES_MQTT_CUSTOM_OK:

			sckOut("Custom MQTT publish OK!!");
			break;

		case SAMMES_MQTT_CUSTOM_ERROR:

			sckOut("Custom MQTT publish failed!!", PRIO_ERROR);
			st.error = ERROR_MQTT;
			break;

		case SAMMES_BOOTED:
		{
			sckOut("ESP finished booting");

			st.espBooting = false;

			StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
			deserializeJson(jsonBuffer, serESP.buff);
			JsonObject json = jsonBuffer.as<JsonObject>();

			String macAddress = json["mac"].as<String>();
			ESPversion = json["ver"].as<String>();
			ESPbuildDate = json["bd"].as<String>();

			// Udate mac address and hostname if we haven't yet
			if (!config.mac.valid) {
				sckOut("Updated MAC address");
				sprintf(config.mac.address, "%s", macAddress.c_str());
				config.mac.valid = true;
				saveConfig();
				snprintf(hostname, sizeof(hostname), "%s", "Smartcitizen");
				memcpy(&hostname[12], &config.mac.address[12], 2);
				memcpy(&hostname[14], &config.mac.address[15], 2);
				hostname[16] = '\0';
			}

			if (!espInfoUpdated) {
				espInfoUpdated = true;
				saveInfo();
			}

			pendingSyncConfig = true;
			break;
		}
		default: break;
	}
}
void SckBase::mqttCustom(const char *topic, const char *payload)
{
	StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
	JsonObject json = jsonBuffer.to<JsonObject>();

    json["to"] = topic;
    json["pl"] = payload;

	serializeJson(json, serESP.buff, NETBUFF_SIZE);
	if (ESPsend(ESPMES_MQTT_CUSTOM)) sckOut("MQTT message sent to ESP...", PRIO_LOW);
}

// **** SD card
bool SckBase::sdInit()
{
    sdInitPending = false;
    st.cardPresentErrorPrinted = false;

    // Change SPI speed to SD_SCK_MHZ(4) in case of innestability.
    if (sd.begin(pinCS_SDCARD, SD_SCK_MHZ(50))) {

        // Print some info about the card
        uint32_t size = sd.card()->sectorCount();
        if (size == 0) {
            sckOut("Can't determine the card size.");
            st.cardPresent = false;     // If we cant initialize sdcard, don't use it!
            return false;
        }

        uint32_t sizeMB = 0.000512 * size + 0.5;
        sprintf(outBuff, "Card size: %u MB\r\nVolume is FAT %i, Cluster size (bytes): %i", sizeMB, int(sd.vol()->fatType()), sd.vol()->bytesPerCluster());
        sckOut();

        st.cardPresent = true;

        sckOut("Sd card ready to use");

        // Check if there is a info file on sdcard
        if (!sd.exists(SD_INFO_NAME)) infoSaved = false;
        saveInfo();
        return true;
    }
    sckOut("ERROR on Sd card Init!!!");
    st.cardPresent = false;     // If we cant initialize sdcard, don't use it!
    return false;
}
bool SckBase::sdDetect()
{
    uint32_t debounceTime = 100;
    if (millis() - lastUserEvent < debounceTime) return false;

    lastUserEvent = millis();
    st.cardPresent = !digitalRead(pinCARD_DETECT);

    if (!digitalRead(pinCARD_DETECT)) {
        sckOut("Sdcard inserted");
        sdInitPending = true;
        return true;
    } else sckOut("Sdcard removed or not found!!");
    return false;
}
bool SckBase::saveInfo()
{
    // Save info to sdcard
    if (!infoSaved) {
        if (sd.exists(SD_INFO_NAME)) sd.remove(SD_INFO_NAME);
        sckFile.open(SD_INFO_NAME, FILE_WRITE);

        if (!createInfo(outBuff, true)) return false;
        sckFile.write(outBuff, strlen(outBuff));
        sckFile.close();

        // If ESP info or time are not there, save it, but again when the data is ready.
        if (espInfoUpdated && st.timeStat.ok) infoSaved = true;

        sckOut("Saved INFO.TXT file!!");
        return true;
    }
    return false;
}
void SckBase::fileManager(fileCom command, const char* name)
{
    // TODO hacer un comando -wipe que deje la sd limpia
    // TODO revisar como será el volcado de flash a sdcard cuando haya datos viejos

    #define FBUFF_SIZE 32

    switch(command)
    {
        case FILECOM_LS:
            sckOut("Files found (date time size name): ");
            sd.ls(LS_DATE | LS_SIZE);
            break;

        case FILECOM_RM:
            if (!sd.remove(name)) sprintf(outBuff, "ERROR deleting %s (Directories can't be removed')", name);
            else sprintf(outBuff, "Deleting %s", name);
            sckOut();
            break;

        case FILECOM_LESS:

            // Open file
            if (!sckFile.open(name, O_RDONLY)) {
                sprintf(outBuff, "ERROR opening file %s", name);
                sckOut();
                break;
            }

            // Print file name
            sprintf(outBuff, "\r\n%s\r\n==============", name);
            sckOut();

            // Get chunks (a lot faster!)
            char fbuff[FBUFF_SIZE];
            while (sckFile.available() > FBUFF_SIZE) {
                sckFile.read(fbuff, FBUFF_SIZE);
                Serial.write(fbuff, FBUFF_SIZE);
            }

            // Get remaining chars
            while (sckFile.available()) Serial.write(sckFile.read());
            sckFile.close();
            break;

        case FILECOM_ALLCSV:
            sckOut("Showing all CSV files");
            FsFile dir;

            // Open root dir
            if (!dir.open("/")) {
                sckOut("ERROR opening directory");
                break;
            }
            dir.rewindDirectory();

            uint16_t prevHeadChkSum = 0;

            while (sckFile.openNext(&dir, O_RDONLY)) {

                // Check if the file is a CSV
                char fbuff[FBUFF_SIZE];
                sckFile.getName(fbuff, FBUFF_SIZE);
                char * pch;
                pch = strstr(fbuff, ".CSV");


                if (!sckFile.isDir() && pch != NULL) {

                    // Check if the header is the same as the previous one, if not print it with a blank and a separator line on top
                    uint16_t newHeadChkSum = headerChkSum(&sckFile);
                    if (newHeadChkSum == prevHeadChkSum) {
                        // Ignore the first 3 lines (the first one is already ignored)
                        uint8_t nl = 0;
                        while (nl <= 3) if (sckFile.read() == '\n') nl++;
                    } else {
                        sckFile.rewind();
                        Serial.println("\n\n<<<<<----------- NEW FILE ------------>>>>>");
                        prevHeadChkSum = newHeadChkSum;
                    }

                    // Get chunks (a lot faster!)
                    while (sckFile.available() > FBUFF_SIZE) {
                        sckFile.read(fbuff, FBUFF_SIZE);
                        Serial.write(fbuff, FBUFF_SIZE);
                    }

                    // Get remaining chars
                    while (sckFile.available()) Serial.write(sckFile.read());

                }
                sckFile.close();
            }
            break;
    }
}
uint16_t SckBase::headerChkSum(char* fileName)
{
    sckFile.open(fileName, O_RDONLY);
    return headerChkSum(&sckFile);
}
uint16_t SckBase::headerChkSum(FsFile* thisFile)
{
    // Calculate checkSum
    uint16_t csCalc = 0;
    char lastChar = thisFile->read();
    while (lastChar != '\r') {
        csCalc ^= lastChar;
        lastChar = thisFile->read();
    }
    return csCalc;
}
uint16_t SckBase::RAMheaderChkSum()
{
    // Store header in outBuff
    uint16_t buffIdx = sprintf(outBuff, "TIME");
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {
        SensorType wichSensor = sensors.sensorsPriorized(i);
        if (sensors[wichSensor].enabled && sensors[wichSensor].priority != 250) {
            buffIdx += sprintf(&outBuff[buffIdx], ",%s", sensors[wichSensor].shortTitle);
        }
    }

    // Calc checkSum
    uint16_t csCalc = 0;
    for (uint16_t i=0; i<buffIdx; i++){
        csCalc ^= outBuff[i];
    }

    return csCalc;
}
void SckBase::saveHeader(FsFile* thisFile)
{
    // Short title line
    Serial.println(thisFile->print("TIME"));
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {
        SensorType wichSensor = sensors.sensorsPriorized(i);
        if (sensors[wichSensor].enabled && sensors[wichSensor].priority != 250) {
            thisFile->print(",");
            thisFile->print(sensors[wichSensor].shortTitle);
        }
    }

    // Unit line
    thisFile->println("");
    thisFile->print("ISO 8601");
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {
        SensorType wichSensor = sensors.sensorsPriorized(i);
        if (sensors[wichSensor].enabled && sensors[wichSensor].priority != 250) {
            thisFile->print(",");
            if (String(sensors[wichSensor].unit).length() > 0) {
                thisFile->print(sensors[wichSensor].unit);
            }
        }
    }

    // Title line
    thisFile->println("");
    thisFile->print("Time");
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {
        SensorType wichSensor = sensors.sensorsPriorized(i);
        if (sensors[wichSensor].enabled && sensors[wichSensor].priority != 250) {
            thisFile->print(",");
            thisFile->print(sensors[wichSensor].title);
        }
    }

    // Platform id line
    thisFile->println("");
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {
        SensorType wichSensor = sensors.sensorsPriorized(i);
        if (sensors[wichSensor].enabled && sensors[wichSensor].priority != 250) {
            thisFile->print(",");
            thisFile->print(sensors[wichSensor].id);
        }
    }
    thisFile->println("");
}

// **** Power
void SckBase::sck_reset()
{
#ifdef WITH_URBAN

#ifdef WITH_CCS811
    // Save updated CCS sensor baseline
    if (I2Cdetect(&Wire, urban.sck_ccs811.address)) {
        uint16_t savedBaseLine = urban.sck_ccs811.getBaseline();
        if (savedBaseLine != 0) {
            sprintf(outBuff, "Saved CCS baseline on eeprom: %u", savedBaseLine);
            sckOut();
            config.extra.ccsBaseline = savedBaseLine;
            config.extra.ccsBaselineValid = true;
            eepromConfig.write(config);
        }
    }
#endif
#ifdef WITH_SEN5X
    if (I2Cdetect(&Wire, urban.sck_sen5x.address)) {
        urban.sck_sen5x.vocStateToEeprom();
    }
#endif
#endif

    sckOut("Bye!!");
    NVIC_SystemReset();
}
void SckBase::goToSleep(uint32_t sleepPeriod)
{
    led.off();
    if (st.espON) ESPcontrol(ESP_OFF);

    // ESP control pins savings
    digitalWrite(pinESP_CH_PD, LOW);
    digitalWrite(pinESP_GPIO0, LOW);
    digitalWrite(pinESP_RX_WIFI, LOW);
    digitalWrite(pinESP_TX_WIFI, LOW);

    if (sckOFF) {

        sprintf(outBuff, "Sleeping forever!!! (until a button click)");
        sckOut();

#ifdef WITH_URBAN
#ifdef WITH_CCS811
        // Stop CCS811 VOCS sensor
        urban.stop(SENSOR_CCS811_VOCS);
#endif
#endif

        // Detach sdcard interrupt to avoid spurious wakeup
        // There is no need to reattach since after this sleep there is always a reset
        detachInterrupt(pinCARD_DETECT);

        // Turn off USB led
        digitalWrite(pinLED_USB, HIGH);

    } else {

        sprintf(outBuff, "Sleeping for %.2f seconds", (sleepPeriod) / 1000.0);
        sckOut();

        st.sleeping = true;

        // Turn off USB led
        digitalWrite(pinLED_USB, HIGH);

        // Set alarm to wakeup via RTC
        rtc.attachInterrupt(NULL);
        uint32_t now = rtc.getEpoch();
        rtc.setAlarmEpoch(now + sleepPeriod/1000);
        rtc.enableAlarm(rtc.MATCH_YYMMDDHHMMSS);
    }

    // Go to Sleep
    USBDevice.standby();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

#ifdef WITH_URBAN
    // Recover Noise sensor timer
    REG_GCLK_GENCTRL = GCLK_GENCTRL_ID(4);  // Select GCLK4
    while (GCLK->STATUS.bit.SYNCBUSY);
#endif
}
void SckBase::updatePower()
{
    charger.detectUSB(this);

    if (charger.onUSB) {

        // Reset lowBatt counters
        battery.lowBatCounter = 0;
        battery.emergencyLowBatCounter = 0;

        switch(charger.getChargeStatus()) {

            case charger.CHRG_NOT_CHARGING:
                // If voltage is too low we asume we don't have battery.
                if (charger.getBatLowerSysMin()) {
                    if (battery.present) sckOut("Battery removed!!");
                    battery.present = false;
                    led.chargeStatus = led.CHARGE_NULL;
                } else {
                    if (!battery.present) sckOut("Battery connected!!");
                    battery.present = true;
                    if (battery.voltage() < battery.maxVolt) charger.chargeState(true);
                    else led.chargeStatus = led.CHARGE_FINISHED;
                }
                break;
            case charger.CHRG_CHARGE_TERM_DONE:

                // To be sure the batt is present, turn off charging and check voltages on next cycle
                if (charger.chargeState()) charger.chargeState(false);
                break;
            default:
                battery.present = true;
                led.chargeStatus = led.CHARGE_CHARGING;
                break;
        }
    } else {

        battery.present = true;

        // Emergency lowBatt
        if (battery.last_percent < battery.threshold_emergency) {
            if (battery.emergencyLowBatCounter < 5) battery.emergencyLowBatCounter++;
            else {

                for (uint8_t i=0; i<5; i++) {
                    led.off();
                    delay(200);
                    led.update(led.ORANGE, led.PULSE_STATIC, true);
                    delay(200);
                }

                sckOut("Emergency low battery!!", PRIO_ERROR);
                st.error = ERROR_BATT;
#ifdef WITH_SENSOR_GROVE_OLED
                auxBoards.updateDisplay(this, true);        // Force update of screen before going to sleep
#endif
                // Ignore last user event and go to sleep
                lastUserEvent = 0;

                // Wake up every minute to check if USB power is back
                while (!charger.onUSB) {
                    goToSleep(60000);
                    charger.detectUSB(this);  // When USB is detecteed the kit should reset to start on clean state
                    if (millis() - lastUserEvent < (config.sleepTimer * 60000)) break;  // Wakeup on user interaction (will go to sleep again after sone blinks)
                }
            }

            // Low Batt
        } else if (battery.last_percent < battery.threshold_low) {

            battery.emergencyLowBatCounter = 0;
            if (battery.lowBatCounter < 5) battery.lowBatCounter++;
            else led.chargeStatus = led.CHARGE_LOW;

        } else {
            battery.emergencyLowBatCounter = 0;
            battery.lowBatCounter = 0;
            led.chargeStatus = led.CHARGE_NULL;
        }
    }

    // if more than one minute have passed since last reset and we are in the right hour-minute, and the flag for sam¡nity reset is on, then reset
    if (millis() > 70000) {
        if (rtc.getHours() == wakeUP_H && rtc.getMinutes() == wakeUP_M && config.sanityResetFlag) {
            sckOut("Sanity reset, bye!!");
            sck_reset();
        }
    }
}
void SckBase::configGCLK6()
{
    // enable EIC clock
    GCLK->CLKCTRL.bit.CLKEN = 0; //disable GCLK module
    while (GCLK->STATUS.bit.SYNCBUSY);

    GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK6 | GCLK_CLKCTRL_ID( GCM_EIC )) ;  //EIC clock switched on GCLK6
    while (GCLK->STATUS.bit.SYNCBUSY);

    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_ID(6));  //source for GCLK6 is OSCULP32K
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

    GCLK->GENCTRL.bit.RUNSTDBY = 1;  //GCLK6 run standby
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

    /* Errata: Make sure that the Flash does not power all the way down
        * when in sleep mode. */

    NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
}
void SckBase::updateDynamic(uint32_t now)
{
    // Only do the check once a second
    if (millis() - lastSpeedMonitoring < 1000) return;
    lastSpeedMonitoring = millis();

#ifdef WITH_GPS
    if (!getReading(&sensors[SENSOR_GPS_SPEED])) return;
    if (!getReading(&sensors[SENSOR_GPS_FIX_QUALITY])) return;
    if (!getReading(&sensors[SENSOR_GPS_HDOP])) return;

    float speedFloat = sensors[SENSOR_GPS_SPEED].reading.toFloat();
    uint8_t fix = sensors[SENSOR_GPS_FIX_QUALITY].reading.toInt();
    uint16_t hdop = sensors[SENSOR_GPS_HDOP].reading.toInt();

    sprintf(outBuff, "Current speed: %.2f, fix: %i, hdop: %i", speedFloat, fix, hdop);
    sckOut(PRIO_LOW);

    // If high speed is detected, we have a good GPS fix and a decent hdop we enable dynamic interval
    if (speedFloat > speed_threshold && fix > 0 && hdop < DYNAMIC_HDOP_THRESHOLD) {

        // Only trigger dynamic interval after N counts of high speed in a row
        if (dynamicCounter > DYNAMIC_COUNTER_THRESHOLD) {
            if (!st.dynamic) sckOut("Entering dynamic interval mode!", PRIO_LOW);
            st.dynamic = true;
            dynamicLast = now;

        } else {
            dynamicCounter++;
        }

    } else {
        // Reset counter
        dynamicCounter = 0;

        // After detecting low speed wait some time before disabling dynamic interval
        if (st.dynamic && (speedFloat < speed_threshold) && (now - dynamicLast > DYNAMIC_TIMEOUT)) {
            sckOut("Turning dynamic interval off!", PRIO_LOW);
            st.dynamic = false;
        }
    }

    // Debug speed readings to sdcard
    if (config.debug.speed) {
        sckFile.open(SD_SPEED_NAME, FILE_WRITE);
        if (sckFile) {
            ISOtime();
            sckFile.print(ISOtimeBuff);
            sckFile.print(",");
            sckFile.print(speedFloat);
            sckFile.print(",");
            sckFile.print(fix);
            sckFile.print(",");
            sckFile.println(hdop);
            sckFile.close();
        } else st.cardPresent = false;
    }
#endif
}
void SckBase::sleepLoop()
{
    uint16_t sleepPeriod = 3;                                           // Only sleep if
    uint32_t now = rtc.getEpoch();
    while (     (!timeToPublish) &&                                     // No publish pending
        (config.readInterval - (now - lastSensorUpdate) > (uint32_t)(sleepPeriod + 2)) &&       // No readings to take in the near future
        ((now - dynamicLast) > (config.sleepTimer * 60)) &&                         // Dynamic interval wasn't triggered recently
        (dynamicCounter == 0) &&                                    // Last speed was low
        (pendingSensorsLinkedList.size() == 0) &&                           // No sensor to wait to
        (st.timeStat.ok) &&                                         // RTC is synced and working
        ((millis() - lastUserEvent) > (config.sleepTimer * 60000)) &&                   // No recent user interaction (button, sdcard or USB events)
        (config.sleepTimer > 0)) {                                  // sleep is enabled

        led.off();
        goToSleep(sleepPeriod * 1000);

        // Let the led be visible for one instant (and start breathing if we need to read sensors)
        if (st.mode == MODE_NET) {
            led.update(led.BLUE2, led.PULSE_STATIC, true);
            delay(10);
            led.update(led.BLUE, led.PULSE_SOFT, true);
        } else {
            led.update(led.PINK2, led.PULSE_STATIC, true);
            delay(10);
            led.update(led.PINK, led.PULSE_SOFT, true);
        }

        updateSensors();
        updatePower();

#ifdef WITH_SENSOR_GROVE_OLED
        // If we have a screen update it
        if (sensors[SENSOR_GROVE_OLED].enabled) auxBoards.updateDisplay(this, true);
#endif

        now = rtc.getEpoch();
    }
}

// **** Sensors
void SckBase::urbanStart()
{
    analogReadResolution(12);

    // Try to start enabled sensors
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {
        OneSensor *wichSensor = &sensors[static_cast<SensorType>(i)];
        if (wichSensor->location == BOARD_URBAN) {
            if (config.sensors[wichSensor->type].enabled) enableSensor(wichSensor->type);
            else sensors[wichSensor->type].enabled = false;
        }
    }
}
void SckBase::updateSensors()
{
    uint32_t now = rtc.getEpoch();

    if (!rtc.isConfigured() || now < 1514764800) {
        sckOut("RTC ERROR when updating sensors!!!", PRIO_LOW);
        epoch2iso(now, ISOtimeBuff);
        st.timeStat.reset();
    }
    if (!st.timeStat.ok) return;
    if (st.onSetup) return;

#ifdef WITH_GPS
    if (sensors[SENSOR_GPS_SPEED].enabled) updateDynamic(now);
#endif

    bool sensorsReady = false;

    // Main reading loop
    uint32_t timeSinceLastSensorUpdate = now - lastSensorUpdate;
    if ((st.dynamic && (timeSinceLastSensorUpdate >= dynamicInterval)) || timeSinceLastSensorUpdate >= config.readInterval) {

        lastSensorUpdate = now;

        sckOut("\r\n-----------");
        epoch2iso(lastSensorUpdate, ISOtimeBuff);
        sckOut(ISOtimeBuff);

        // Clear pending sensor list (no sensor should take more than the reading interval).
        pendingSensorsLinkedList.clear();

        for (uint8_t i=0; i<SENSOR_COUNT; i++) {

            // Get next sensor based on priority
            OneSensor *wichSensor = &sensors[sensors.sensorsPriorized(i)];

            // Check if it is enabled
            if (wichSensor->enabled && wichSensor->priority != 250) {

                if (    (lastSensorUpdate - wichSensor->lastReadingTime) >= (wichSensor->everyNint * config.readInterval) ||
                    (st.dynamic && ((lastSensorUpdate - wichSensor->lastReadingTime) >= dynamicInterval))) {    // Is time to read it?

                    wichSensor->lastReadingTime = lastSensorUpdate;     // Update sensor reading time

                    if (!getReading(wichSensor)) {
                        sprintf(outBuff, "Adding %s to pending sensor list", wichSensor->title);
                        sckOut(PRIO_LOW);
                        pendingSensorsLinkedList.add(wichSensor->type);     // Read it or save it for later
                    } else {
                        sprintf(outBuff, "%s: %s %s", wichSensor->title, wichSensor->reading.c_str(), wichSensor->unit);
                        sckOut();
                    }
                }
            }
        }

        if (pendingSensorsLinkedList.size() == 0) sensorsReady = true;
        else {
            sprintf(outBuff, "Waiting for %u sensors to finish the reading process", pendingSensorsLinkedList.size());
            sckOut(PRIO_LOW);
        }

    } else if (pendingSensorsLinkedList.size() > 0) {   // If we still have some sensors pending
        for (uint8_t i=0; i<pendingSensorsLinkedList.size(); i++) {

            OneSensor *wichSensor = &sensors[pendingSensorsLinkedList.get(i)];

            if (getReading(wichSensor)) {
                pendingSensorsLinkedList.remove(i);     // Read it or keepit for later
                sprintf(outBuff, "%s: %s %s", wichSensor->title, wichSensor->reading.c_str(), wichSensor->unit);
                sckOut();
            }
        }
        if (pendingSensorsLinkedList.size() == 0) sensorsReady = true;
    }

    if (sensorsReady) {
        sckOut("-----------");

        SckList::GroupIndex justSaved = readingsList.saveGroup();
        if (justSaved.sector >= 0) {
            sprintf(outBuff, "(%s) Readings saved to flash memory.", ISOtimeBuff);
            sckOut();
        }

        if (st.cardPresent) {
            // Publish new groups (that aren't saved to sdcard)
            uint32_t counter = 0;
            SckList::GroupIndex thisGroup = readingsList.readGroup(readingsList.PUB_SD);
            while (thisGroup.group >= 0) {

                if (sdPublish()) {
                    uint8_t readingNum = readingsList.setPublished(thisGroup, readingsList.PUB_SD);
                    sprintf(outBuff, "(%s) %u readings saved to sdcard.", ISOtimeBuff, readingNum);
                    sckOut();
                    counter++;
                    thisGroup = readingsList.readGroup(readingsList.PUB_SD);
                } else break;
            }
        }

        // Don't keep groups as unpublished for other modes.
        if (st.mode == MODE_NET) readingsList.setPublished(justSaved, readingsList.PUB_SD);
        else if (st.mode == MODE_SD) readingsList.setPublished(justSaved, readingsList.PUB_NET);
    }

    // If we have readings pending to be published
    if (readingsList.availableReadings[readingsList.PUB_NET]) {

        // If its time (based on configured publish interval) or WiFi is connected and ready
        if (    now - lastPublishTime >= config.publishInterval ||
            st.wifiStat.ok) {

            // Check if we are in offline programmed hours
            bool offlineHours = false;
            if (config.offline.start >= 0 && config.offline.end >= 0 && millis() - lastUserEvent > (config.sleepTimer * 60000)) {
                int8_t thisHour = rtc.getHours();
                if (thisHour > config.offline.start && thisHour < config.offline.end) {
                    if (st.espON) ESPcontrol(ESP_OFF);
                    led.update(led.BLUE, led.PULSE_WARNING);
                    offlineHours = true;
                }
            }

            if (!offlineHours) {

                if (st.wifiStat.error) {
                    // If WiFi was failing check if enough time has passed to retry
                    if ((now - st.lastWiFiError) > config.offline.retry) {
                        sckOut("After wifi error it's time to publish", PRIO_LOW);
                        timeToPublish = true;
                    }
                } else {
                    timeToPublish = true;
                    sckOut("it's time to publish", PRIO_LOW);
                }
            }
        }

        //  10 minutes after user interaction we publish as soon readings are available (mostly to be responsive during oboarding process)
        if ((millis() - lastUserEvent < 10 * 60000) || config.sleepTimer == 0) {
            sckOut("Recent user interaction, it's time to publish", PRIO_LOW);
            timeToPublish = true;
        }
    }
}
bool SckBase::enableSensor(SensorType wichSensor)
{
    bool result = false;
    switch (sensors[wichSensor].location) {
        case BOARD_BASE:
            {
                switch (wichSensor) {
                    case SENSOR_BATT_VOLTAGE:
                    case SENSOR_BATT_PERCENT: // Allow enabling battery even if its not present so it can be posted to platform (reading will return -1 if the batery is not present)
                    case SENSOR_SDCARD:
                    case SENSOR_RSSI:
                        result = true;
                        break;
                    default: break;
                }
            }
        case BOARD_URBAN: if (urban.start(wichSensor)) result = true; break;
        case BOARD_AUX: {
            if (auxBoards.start(this, wichSensor)) result = true;
            break;
        }
        default: break;
    }

    if (result) {
        sprintf(outBuff, "Enabling %s", sensors[wichSensor].title);
        sckOut();
        sensors[wichSensor].enabled = true;
#ifdef WITH_SENSOR_GROVE_OLED
        sensors[wichSensor].oled_display = config.sensors[wichSensor].oled_display;  // Show detected sensors on oled display if config is true (default).
#endif
        writeHeader = true;
        return true;
    }

    sensors[wichSensor].enabled = false;
#ifdef WITH_SENSOR_GROVE_OLED
    sensors[wichSensor].oled_display = false;
#endif
    // Avoid spamming with mesgs for every supported auxiliary sensor
    if (sensors[wichSensor].location == BOARD_BASE) {
        sprintf(outBuff, "Failed enabling %s", sensors[wichSensor].title);
        sckOut();
    }

    return false;
}
bool SckBase::disableSensor(SensorType wichSensor)
{
    bool result = false;
    switch (sensors[wichSensor].location) {
        case BOARD_BASE:
            {
                switch (wichSensor) {
                    case SENSOR_BATT_PERCENT:
                    case SENSOR_BATT_VOLTAGE:
                    case SENSOR_SDCARD:
                    case SENSOR_RSSI:
                        result = true;
                        break;
                    default: break;
                }
            }
        case BOARD_URBAN: if (urban.stop(wichSensor)) result = true; break;
        case BOARD_AUX: if (auxBoards.stop(wichSensor)) result = true; break;
        default: break;
    }

    if (result) sprintf(outBuff, "Disabling %s", sensors[wichSensor].title);
    else sprintf(outBuff, "Failed stopping %s, still will be disabled", sensors[wichSensor].title);
    sckOut();

    sensors[wichSensor].enabled = false;
    writeHeader = true;

    return result;
}
bool SckBase::getReading(OneSensor *wichSensor)
{
    switch (wichSensor->location) {
        case BOARD_BASE:
            {
                switch (wichSensor->type) {
                    case SENSOR_BATT_PERCENT:
                        if (!battery.present) wichSensor->reading = String("-1");
                        else wichSensor->reading = String(battery.percent(&charger));
                        wichSensor->state = 0;
                        break;
                    case SENSOR_BATT_VOLTAGE:
                        if (!battery.present) wichSensor->reading = String("-1");
                        else wichSensor->reading = String(battery.voltage());
                        wichSensor->state = 0;
                        break;
                    case SENSOR_SDCARD:
                        if (st.cardPresent) wichSensor->reading = String("1");
                        else wichSensor->reading = String("0");
                        wichSensor->state = 0;
                        break;
                    case SENSOR_RSSI:
                        getRSSI(wichSensor);
                        break;
                    default: break;
                }
                break;
            }
        case BOARD_URBAN:
            {
                urban.getReading(this, wichSensor);
                break;
            }
        case BOARD_AUX:
            {
                auxBoards.getReading(this, wichSensor);
                break;
            }
    }

    // Reading is not yet ready
    if (wichSensor->state > 0) return false;

    // Sensor reading ERROR, save null value
    if (wichSensor->state == -1) wichSensor->reading == "null";

#ifdef WITH_URBAN
    // Temperature / Humidity temporary Correction
    // TODO remove this when calibration routine is ready
    if (wichSensor->type == SENSOR_TEMPERATURE) {
        float aux_temp = wichSensor->reading.toFloat();

        // TODO This needs major refactoring...
        // Corrections are linear, with no slope correction, only offset
        float net_usb_t_offset = 0;
        float net_batt_t_offset = 0;
        float sd_usb_t_offset = 0;
        float sd_batt_t_offset = 0;

#ifdef SCK21_AIR
        net_usb_t_offset =  - 1.45; // DONE
        sd_usb_t_offset = -1.6;  // TODO CHECK
        net_batt_t_offset = -1.3;  // DONE
        sd_batt_t_offset = -1.3; // TODO CHECK
#endif

#ifdef SCK22_AIR
        net_usb_t_offset =  - 1.25; // DONE
        sd_usb_t_offset = -1.3; // DONE
        net_batt_t_offset = -1.05; // DONE
        sd_batt_t_offset = -1.0; // DONE
#endif

        // Correct depending on battery/USB and network/sd card status
        if (charger.onUSB) {
            if (st.mode == MODE_NET) wichSensor->reading = String(aux_temp + net_usb_t_offset);
            else wichSensor->reading = String(aux_temp + sd_usb_t_offset);
        } else {
            if (st.mode == MODE_NET) wichSensor->reading = String(aux_temp + net_batt_t_offset);
            else wichSensor->reading = String(aux_temp + sd_batt_t_offset);
        }

    } else if(wichSensor->type == SENSOR_HUMIDITY) {

        float aux_hum = wichSensor->reading.toFloat();
        // Corrections are linear, with no slope correction, only offset
        float net_usb_h_offset = 0;
        float net_batt_h_offset = 0;
        float sd_usb_h_offset = 0;
        float sd_batt_h_offset = 0;

#ifdef SCK21_AIR
        net_usb_h_offset = 5; // DONE
        sd_usb_h_offset = 5; // TODO CHECK
        net_batt_h_offset = 5; // DONE
        sd_batt_h_offset = 5; // TODO CHECK
#endif

#ifdef SCK22_AIR
        net_usb_h_offset = 3.8; // DONE
        sd_usb_h_offset = 4; // DONE
        net_batt_h_offset = 3.6; // DONE
        sd_batt_h_offset = 3.4; // DONE
#endif

        // Correct depending on battery/USB and network/sd card status
        if (charger.onUSB) {
            if (st.mode == MODE_NET) wichSensor->reading = String(aux_hum  + net_usb_h_offset);
            else wichSensor->reading = String(aux_hum + sd_usb_h_offset);
        } else {
            if (st.mode == MODE_NET) wichSensor->reading = String(aux_hum + net_batt_h_offset);
            else wichSensor->reading = String(aux_hum + sd_batt_h_offset);
        }
    }
#endif

    return true;
}

void SckBase::controlSensor(SensorType wichSensorType, String wichCommand)
{
        sprintf(outBuff, "%s: %s", sensors[wichSensorType].title, wichCommand.c_str());
        sckOut();
        switch (sensors[wichSensorType].location) {
            case BOARD_URBAN: urban.control(this, wichSensorType, wichCommand); break;
            case BOARD_AUX: {
                String result = auxBoards.control(wichSensorType, wichCommand);
                result.toCharArray(outBuff, result.length()+1);
                sckOut();
                break;
            }
            default: break;
        }
}
bool SckBase::netPublish()
{
	if (!st.espON) {
		ESPcontrol(ESP_ON);
		return false;
	}

	bool result = false;
	wichGroupPublishing = readingsList.readGroup(readingsList.PUB_NET);
	if (wichGroupPublishing.group >= 0) {
		sprintf(outBuff, "(%s) Sent readings to platform.", ISOtimeBuff);
		sckOut();
		sckOut(serESP.buff, PRIO_LOW);
		result = ESPsend(ESPMES_MQTT_PUBLISH);
	} else {
		timeToPublish = false; 		// There are no more readings available
	}

	// Wait for response or timeout
	uint32_t timeout = millis();
	while (millis() - timeout < 1000) {
		ESPbusUpdate();
		if (st.publishStat.ok || st.publishStat.error) break;
		if (millis() - lastUserEvent < 1000) break;
	}

	return result;
}
bool SckBase::sdPublish()
{
    // Create the name with YY-MM-DD.CSV
    char postfileName[32];
    sprintf(postfileName, "%02d-%02d-%02d.CSV", rtc.getYear(), rtc.getMonth(), rtc.getDay());

    bool exist = sd.exists(postfileName);

    // If the file has a different set of sensors, we need to create a new file, so rename existing one.
    if (exist && headerChkSum(postfileName) != RAMheaderChkSum()) {

        // Rename file as YY-MM-DD_NN.CSV, NN will start in 01 and grow if needed
        char newName[16];
        bool fileExists = true;
        uint8_t fileNumber = 1;
        while (fileExists) {
            sprintf(newName, "%02d-%02d-%02d_%02u.CSV", rtc.getYear(), rtc.getMonth(), rtc.getDay() , fileNumber);
            fileNumber++;
            if (!sd.exists(newName)) fileExists = false;
        }
        sprintf(outBuff, "Sensor config changed, renaming old csv file to %s", newName);
        sckOut();
        sd.rename(postfileName, newName);

        // We need to write header since we are going to start a new file
        exist = false;
    }

    // Try to open file
    if (!sckFile.open(postfileName, FILE_WRITE)) {
        st.cardPresent = false;
        st.cardPresentErrorPrinted = false;
        sckOut("ERROR failed publishing to SD card");
        led.update(led.PINK, led.PULSE_ERROR);
        return false;
    }

    // If file didn't existed  or was renamed we need to save the header
    if (!exist) saveHeader(&sckFile);

    // Save readings and close the file
    sckFile.print(readingsList.flashBuff);
    sckFile.close();

    // Update state
    if (st.mode == MODE_SD) {
        timeToPublish = false;
        lastPublishTime = rtc.getEpoch();
    }
    return true;
}
bool SckBase::getRSSI(OneSensor* wichSensor)
{
    // To be able to use monitor mode on this sensor, the ESP MUST be on and CONNECTED before issuing the monitor command
    // We recommend the following:
    // > shell -on
    // > esp -on
    // and when WiFi is connected:
    // > monitor rssi

    // Check that the reading interval is the same as the publish interval (we don't want to turn on theESP just for this)
    if (wichSensor->everyNint * config.readInterval != config.publishInterval) {
        uint8_t newEveryNint = config.publishInterval / config.readInterval;
        if (newEveryNint < 1) newEveryNint = 1;
        wichSensor->everyNint = newEveryNint;
        saveConfig();
        sprintf(outBuff, "RSSI interval must be the same as publish interval!!\r\ncorrecting it to %i", config.readInterval * newEveryNint);
        sckOut();
    }

    // If less than a publish interval has passed and ESP is not connected, the reading from the last established connection is still the valid one.
    // We don't want to start the ESP only to check the RSSI, we update this reading only when publishing
    uint32_t now = rtc.getEpoch();
    if ((now - lastRSSIUpdate) < (config.publishInterval + 5) && !st.wifiStat.ok) {
        wichSensor->state = 0;
        return true;
    }

    wichSensor->state = -1;

    // We don't want to turn on the ESP just to take a RSSI reading'
    if (!st.espON) return false;

    // Ask ESP for reading
    serESP.send(ESPMES_GET_RSSI);

    // Wait for answer
    uint32_t started = millis();
    while (!serESP.receive()) {
        if (millis() - started > 1000) {
            return false;
        }
    }

    // Check if it is our answer
    if (serESP.msg != SAMMES_RSSI) return false;

    // Wifi.RSSI() seems to return 31 on error
    if (strcmp("31", serESP.buff) == 0) {
        wichSensor->reading = "null";
        return false;
    }

    // All seems OK
    wichSensor->reading = serESP.buff;
    wichSensor->state = 0;
    return true;
}

// **** Time
bool SckBase::setTime(String epoch)
{
    // Keep track of time passed before updating clock
    uint32_t pre = rtc.getEpoch();

    rtc.setEpoch(epoch.toInt());
    int32_t diff = rtc.getEpoch() - epoch.toInt();
    if (abs(diff) < 2) {
        timeSyncAfterBoot = true;
        st.timeStat.setOk();

        // Adjust events after updating clock
        uint32_t now = rtc.getEpoch();
        lastSensorUpdate = now - (pre - lastSensorUpdate);
        lastPublishTime = now - (pre - lastPublishTime);
        espStarted = now - (pre - espStarted);
        for (uint8_t i=0; i<SENSOR_COUNT; i++) {
            if (sensors[static_cast<SensorType>(i)].lastReadingTime != 0) {
                sensors[static_cast<SensorType>(i)].lastReadingTime =  now - (pre - sensors[static_cast<SensorType>(i)].lastReadingTime);
            }
        }

        lastTimeSync = millis();

        ISOtime();
        sprintf(outBuff, "RTC updated: %s", ISOtimeBuff);
        sckOut();
        return true;
    } else {
        sckOut("RTC update failed!!");
    }
    return false;
}
bool SckBase::ISOtime()
{
    if (st.timeStat.ok) {
        epoch2iso(rtc.getEpoch(), ISOtimeBuff);
        return true;
    } else {
        sprintf(ISOtimeBuff, "0");
        return false;
    }
}
void SckBase::epoch2iso(uint32_t toConvert, char* isoTime)
{

    time_t tc = toConvert;
    struct tm* tmp = gmtime(&tc);

    sprintf(isoTime, "20%02d-%02d-%02dT%02d:%02d:%02dZ",
            tmp->tm_year - 100,
            tmp->tm_mon + 1,
            tmp->tm_mday,
            tmp->tm_hour,
            tmp->tm_min,
            tmp->tm_sec);
}


void SckBase::getUniqueID()
{
    volatile uint32_t *ptr1 = (volatile uint32_t *)0x0080A00C;
    uniqueID[0] = *ptr1;

    volatile uint32_t *ptr = (volatile uint32_t *)0x0080A040;
    uniqueID[1] = *ptr;
    ptr++;
    uniqueID[2] = *ptr;
    ptr++;
    uniqueID[3] = *ptr;

    sprintf(uniqueID_str,  "%lX%lX%lX%lX", uniqueID[0], uniqueID[1], uniqueID[2], uniqueID[3]);
}
bool I2Cdetect(TwoWire *_Wire, byte address)
{
    _Wire->beginTransmission(address);
    byte error = _Wire->endTransmission();

    if (error == 0) return true;
    else return false;
}

void Status::setOk()
{
    ok = true;
    error = false;
    retrys = 0;
    _lastTryMillis = 0;
}
bool Status::retry()
{
    if (error) return false;
    if (millis() - _lastTryMillis > _timeout || _lastTryMillis == 0) {
        if (retrys == _maxRetrys) {
            error = true;
            return false;
        }
        _lastTryMillis = millis();
        retrys++;
        return true;
    }

    return false;
}
void Status::reset()
{
    ok = false;
    error = false;
    retrys = 0;
    _lastTryMillis = 0;
}
