#include "SckESP.h"

// SAM communication
RH_Serial driver(Serial);
RHReliableDatagram manager(driver, ESP_ADDRESS);

// Telnet debug 
RemoteDebug Debug;

// Web Server
ESP8266WebServer webServer(80);

// DNS for captive portal
DNSServer dnsServer;

void SckESP::setup() {

	// LED outputs
	pinMode(pinLED, OUTPUT);
	digitalWrite(pinLED, LOW);

	// SAM communication
	Serial.begin(serialBaudrate);
	Serial.setDebugOutput(false);
	manager.init();
	debugOUT("ESP starting...");

	// Flash filesystem
	SPIFFS.begin();
	// if (SPIFFS.format()) debugOUT("SPIFFS formated!!!");

	// Create hostname
	macAddr = WiFi.softAPmacAddress();
	String tailMacAddr = macAddr.substring(macAddr.length() - 5);
	tailMacAddr.replace(":", "");
	strncpy(hostname, "Smartcitizen", 20);
	strncat(hostname, tailMacAddr.c_str(), 4);

	WiFi.hostname(hostname);
	WiFi.persistent(false);		 		// Only write to flash credentials when changed (for protecting flash from wearing out)

	loadConfig();
	currentWIFIStatus = WiFi.status();

	if (config.credentials.set) {
		ledBlink(LED_SLOW);
		tryConnection();
	} else {
		ledBlink(LED_FAST);
		// Notify sam ??? on no token set also???
		// startAP();	// Or wait for sam order?
	}

	if (telnetDebug) {
		Debug.begin(hostname);
		Debug.setResetCmdEnabled(true);
		Debug.showColors(true);
		Debug.setSerialEnabled(false);
	}
}
void SckESP::update() {

	if (WiFi.getMode() == WIFI_AP) {
		dnsServer.processNextRequest();
		// webServer.handleClient();
	}

	if (WiFi.status() != currentWIFIStatus) {
		currentWIFIStatus = WiFi.status();
		switch (currentWIFIStatus) {
			case WL_CONNECTED: ledSet(1); sendMessage(SAMMES_WIFI_CONNECTED); break;
			case WL_CONNECT_FAILED: ledBlink(LED_FAST); sendMessage(SAMMES_PASS_ERROR); break;
			case WL_NO_SSID_AVAIL: ledBlink(LED_FAST); sendMessage(SAMMES_SSID_ERROR); break;
			default: ledBlink(LED_FAST); sendMessage(SAMMES_WIFI_UNKNOWN_ERROR); break;
		}
	}
	
	SAMbusUpdate();

	if (telnetDebug) Debug.handle();
}
void SckESP::tryConnection() {

	if (WiFi.status() != WL_CONNECTED) {

		debugOUT(String F("Trying connection to wifi: ") + String(config.credentials.ssid) + " - " + String(config.credentials.pass));

		// Support for open networks
		String tp = String(config.credentials.pass);
		if (tp.length() == 0) WiFi.begin(config.credentials.ssid);
		else WiFi.begin(config.credentials.ssid, config.credentials.pass);

	} else {
		debugOUT(String F("Already connected to wifi: ") + String(WiFi.SSID()));
	}
}

// **** Input/Output
void SckESP::debugOUT(String strOut) {

	if (telnetDebug) {
		strOut += "\r\n";
		DEBUG(strOut.c_str());
	}
	
	if (serialDebug) sendMessage(SAMMES_DEBUG, strOut.c_str());
}
void SckESP::SAMbusUpdate() {
	
	if (manager.available()) {

		uint8_t len = NETPACK_TOTAL_SIZE;

		if (manager.recvfromAck(netPack, &len)) {

			// Identify received command
			uint8_t pre = String((char)netPack[1]).toInt();
			ESPMessage wichMessage = static_cast<ESPMessage>(pre);
			// debugOUT("Command: " + String(wichMessage));

			// Get content from first package (1 byte less than the rest)
			memcpy(netBuff, &netPack[2], NETPACK_CONTENT_SIZE - 1);

			// Het the rest of the packages (if they exist)
			for (uint8_t i=0; i<netPack[TOTAL_PARTS]-1; i++) {
				if (manager.recvfromAckTimeout(netPack, &len, 500))	{
					memcpy(&netBuff[(i * NETPACK_CONTENT_SIZE) + (NETPACK_CONTENT_SIZE - 1)], &netPack[1], NETPACK_CONTENT_SIZE);
				}
				else return;
			}
			// debugOUT("Content: " + String(netBuff));

			// Process message
			receiveMessage(wichMessage);
		}
	}
}
bool SckESP::sendMessage(SAMMessage wichMessage) {

	// This function is used when &netBuff[1] is already filled with the content

	sprintf(netBuff, "%u", wichMessage);
	return sendMessage();
}
bool SckESP::sendMessage(SAMMessage wichMessage, const char *content) {

	sprintf(netBuff, "%u%s", wichMessage, content);
	return sendMessage();
}
bool SckESP::sendMessage() {

	// This function is used when netbuff is already filled with command and content

	uint16_t totalSize = strlen(netBuff);
	uint8_t totalParts = (totalSize + NETPACK_CONTENT_SIZE - 1)  / NETPACK_CONTENT_SIZE;

	for (uint8_t i=0; i<totalParts; i++) {
		netPack[TOTAL_PARTS] = totalParts;
		memcpy(&netPack[1], &netBuff[(i * NETPACK_CONTENT_SIZE)], NETPACK_CONTENT_SIZE);
		if (!manager.sendtoWait(netPack, NETPACK_TOTAL_SIZE, SAM_ADDRESS)) return false;
	}
	return true;
}
void SckESP::receiveMessage(ESPMessage wichMessage) {

	switch(wichMessage) {

		case ESPMES_SET_CONFIG: {

			// TODO put this in a reusable function
			StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(netBuff);
			uint8_t intMode = json["mo"];
			SCKmodes wichMode = static_cast<SCKmodes>(intMode);
			config.mode = wichMode;
			uint8_t intMode2 = json["wm"];
			SCKmodes wichMode2 = static_cast<SCKmodes>(intMode2);
			config.workingMode = wichMode2;
			config.publishInterval = json["pi"];
			config.credentials.set = json["cs"];
			strcpy(config.credentials.ssid, json["ss"]);
			strcpy(config.credentials.pass, json["pa"]);
			config.token.set = json["ts"];
			strcpy(config.token.token, json["to"]);

			saveConfig(config);
			break;

		} case ESPMES_GET_NETINFO: sendNetinfo(); break;
	 	default: break;
	}
}

bool SckESP::sendNetinfo() {
		
		StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
		JsonObject& jsonSend = jsonBuffer.createObject();
		jsonSend["hn"] = hostname;
		ipAddr = WiFi.localIP().toString();
		jsonSend["ip"] = ipAddr;
		jsonSend["mac"] = macAddr;

		sprintf(netBuff, "%u", SAMMES_NETINFO);
		jsonSend.printTo(&netBuff[1], jsonSend.measureLength() + 1);

		return sendMessage();
}

// **** APmode and WebServer
void SckESP::startAP(){

	debugOUT(String F("Starting Ap with ssid: ") + String(hostname));

	// IP for DNS
	IPAddress myIP(192, 168, 1, 1);

	// Start Soft AP
	WiFi.mode(WIFI_AP);
	WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP((const char *)hostname);
	delay(500);
	// espStatus.ap = ESP_AP_ON_EVENT;

	// DNS stuff (captive portal)
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", myIP);

	// startWebServer();
}
void SckESP::stopAP() {

	dnsServer.stop();
	WiFi.softAPdisconnect(true);
}

// **** Configuration
bool SckESP::saveConfig(Configuration newConfig) {

	debugOUT("entro");

	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["mo"] = (uint8_t)config.mode;
	json["wm"] = (uint8_t)config.workingMode;
	json["pi"] = config.publishInterval;
	json["cs"] = (uint8_t)config.credentials.set;
	json["ss"] = config.credentials.ssid;
	json["pa"] = config.credentials.pass;
	json["ts"] = (uint8_t)config.token.set;
	json["to"] = config.token.token;

	debugOUT("paso otra vez");

	File configFile = SPIFFS.open(configFileName, "w");
	if (configFile) {
		json.printTo(configFile);
		configFile.write('\n');
		configFile.close();
		debugOUT("saved configuration!!");
		if (config.credentials.set) {
			ledBlink(LED_SLOW);
			tryConnection();
		}
		return true;
	}
	return false;
}
bool SckESP::loadConfig() {

	if (SPIFFS.exists(configFileName)) {

		File configFile = SPIFFS.open(configFileName, "r");

		StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
		JsonObject &json = jsonBuffer.parseObject(configFile);

		if (json.success()) {

			uint8_t intMode = json["mo"];
			SCKmodes wichMode = static_cast<SCKmodes>(intMode);
			config.mode = wichMode;

			uint8_t intMode2 = json["wm"];
			SCKmodes wichMode2 = static_cast<SCKmodes>(intMode2);
			config.workingMode = wichMode2;

			config.publishInterval = json["pi"];

			config.credentials.set = json["cs"];
			strcpy(config.credentials.ssid, json["ss"]);
			strcpy(config.credentials.pass, json["pa"]);

			config.token.set = json["ts"];
			strcpy(config.token.token, json["to"]);
		}
		configFile.close();
		debugOUT("Loaded configuration!!");
		return true;
	} 

	debugOUT("Can't find valid configuration!!! loading defaults...");
	Configuration newConfig;
	config = newConfig;
	saveConfig(config);
	return false;
}

// **** Led
void SckESP::ledSet(uint8_t value) {
	blink.detach();
	ledValue = abs(value - 1);
	digitalWrite(pinLED, ledValue);
}
void SckESP::ledBlink(float rate) {

	blink.attach_ms(rate, ledToggle);
}
void SckESP::_ledToggle() {
	ledValue = abs(ledValue - 1);
	digitalWrite(pinLED, ledValue);
}