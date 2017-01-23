#include "sckESP.h"

/* 	---------------------------------------------------------
	|	Structs for SAM <<>> ESP communication				|
	---------------------------------------------------------
*/
EasyTransfer BUS_credentials;


void SckESP::setup() {

	// LED outputs
	pinMode(LED_LEFT, OUTPUT);
	pinMode(LED_RIGHT, OUTPUT);
	digitalWrite(LED_LEFT, HIGH);
	digitalWrite(LED_RIGHT, HIGH);

	// Filesystem (Still recognized less size than real)
	SPIFFS.begin();
	SPIFFS.info(fs_info);

	// SAM <<>> ESP comunication
	BUS_credentials.begin(details(credentials), &Serial);

	Serial.begin(115200);
	Serial.setDebugOutput(false);

};

void SckESP::start() {
	ledBlink(ledLeft, 350);
	ledBlink(ledRight, 350);

};

void SckESP::update() {

	if (WiFi.status() == WL_CONNECTED) {
/*

    WL_CONNECTED after successful connection is established
    WL_NO_SSID_AVAILin case configured SSID cannot be reached
    WL_CONNECT_FAILED if password is incorrect
    WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
    WL_DISCONNECTED if module is not configured in station mode

*/

		ledSet(ledLeft, 1);
		ledSet(ledRight, 0);
	}

	if (BUS_credentials.receiveData()) {
		if (credentials.sender == SENDER_SAM) {
			setWifi();
		}
	}

};

void espIn(String input) {

};

bool SckESP::setWifi(){
	WiFi.begin(credentials.ssid, credentials.password);

	saveCredentials();

};

void SckESP::saveCredentials() {

	// if file !exist create it and open for write
	// else open it for read and writes and verify date of writed credentials before overwiting

	if (!SPIFFS.exists(CREDENTIALS_FILE)) {
		File credFile = SPIFFS.open(CREDENTIALS_FILE, "w");
		if (credFile) {
			credFile.println(credentials.lastUpdated);
			credFile.println(credentials.ssid);
			credFile.println(credentials.password);
			credFile.close();
		} else {
			// handle and report event error!!
		}

	} else {


	}


	File credFile = SPIFFS.open(CREDENTIALS_FILE, "r+");

		/*struct Credentials {
			uint32_t lastUpdated = 0;			// epoch time
			Sender sender = SENDER_NONE;
			char ssid[64] = "ssid";
			char password[64] = "password";
		};*/

		Credentials readedCred;

		String readedLine;
		// String readedLastUpdated;
		// String readedSsid;
		// String readedPassword;

		if (credFile) {
			while (credFile.available()) {
				char buff = credFile.read();
				if (buff == 13 || buff == 10) {
					// asignar los valores
				} else {
					readedLine += buff;
				}
			}
		}
}

bool SckESP::saveConf() {

};

bool SckESP::loadConf() {

};

bool SckESP::mqttStart(String server) {

};

bool SckESP::mqttHellow() {

};

bool SckESP::mqttSend(String payload) {

};


/* 	---------------------------------------------
 	|	SmartCitizen Kit Wifi Leds management   |
 	---------------------------------------------
*/

void SckESP::ledSet(uint8_t wichLed, uint8_t value) {
	if(wichLed == ledLeft) Lblink.detach();
	else Rblink.detach();
	ledValue[wichLed] = value;
	digitalWrite(ledPin[wichLed], ledValue[wichLed]);
}

void SckESP::ledToggle(uint8_t wichLed) {
	ledValue[wichLed] = abs(ledValue[wichLed] - 1);
	digitalWrite(ledPin[wichLed], ledValue[wichLed]);
};

void SckESP::ledBlink(uint8_t wichLed, float rate) {
	if (wichLed == ledLeft) Lblink.attach_ms(rate, LedToggleLeft);
	else if (wichLed == ledRight) Rblink.attach_ms(rate, LedToggleRight);
};

// TODO
/*	[ ] FUNC Save credentials and token to disk
 *	[ ] FUNC factory reset credentials and token
 *	[ ] FUNC Receive credentals from SAM
 *  [ ] System for sync the last credentials SAM <<>> ESP
 *	[ ] Multiple credentials

 *	[ ] Show wifi status in leds
 *	[ ] Send to SAM events in wifi changes

 *	[ ] Sistema de comandos SAM >> ESP
 *  [ ] Respuestas ESP >> SAM
 *	[ ] Save settings on flash
 *	[ ] RTC y SNTP
 *	[ ] Flash solo reconoce 1mb
 *	[ ] MQTT
 *	[ ] Leds con pwm
 *	[ ] ESP sleep
 *	[ ] AP mode (WifiManager)
 *	[ ] OTA
 *  [ ] Crear un event stack con un array de enums que tenga la historia de eventos desde que arranca. y una funcion de getEventStack.
*/