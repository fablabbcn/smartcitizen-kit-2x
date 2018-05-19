#include "SckESP.h"

// SAM communication
RH_Serial driver(Serial);
RHReliableDatagram manager(driver, ESP_ADDRESS);

// Telnet debug
RemoteDebug Debug;

// Web Server
ESP8266WebServer webServer(80);

// MQTT client
WiFiClient wclient;
PubSubClient MQTTclient(wclient);

// DNS for captive portal
DNSServer dnsServer;

void SckESP::setup()
{

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
		startAP();
	}

	if (telnetDebug) {
		Debug.begin(hostname);
		Debug.setResetCmdEnabled(true);
		Debug.showColors(true);
		Debug.setSerialEnabled(false);
	}
}
void SckESP::update()
{

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
			case WL_DISCONNECTED: 
					       if (config.credentials.set) ledBlink(LED_SLOW);
					       else ledBlink(LED_FAST);
					       break;
			default: ledBlink(LED_FAST); sendMessage(SAMMES_WIFI_UNKNOWN_ERROR); break;
		}
	}

	SAMbusUpdate();

	if (telnetDebug) Debug.handle();
}
void SckESP::tryConnection()
{

	if (WiFi.status() != WL_CONNECTED) {

		debugOUT(String F("Trying connection to wifi: ") + String(config.credentials.ssid) + " - " + String(config.credentials.pass));

		WiFi.mode(WIFI_STA);
		// Support for open networks
		String tp = String(config.credentials.pass);
		if (tp.length() == 0) WiFi.begin(config.credentials.ssid);
		else WiFi.begin(config.credentials.ssid, config.credentials.pass);

	} else {
		debugOUT(String F("Already connected to wifi: ") + String(WiFi.SSID()));
	}
}

// **** Input/Output
void SckESP::debugOUT(String strOut)
{

	if (telnetDebug) {
		strOut += "\r\n";
		DEBUG(strOut.c_str());
	}

	if (serialDebug) sendMessage(SAMMES_DEBUG, strOut.c_str());
}
void SckESP::SAMbusUpdate()
{

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
bool SckESP::sendMessage(SAMMessage wichMessage)
{

	// This function is used when &netBuff[1] is already filled with the content

	sprintf(netBuff, "%u", wichMessage);
	return sendMessage();
}
bool SckESP::sendMessage(SAMMessage wichMessage, const char *content)
{

	sprintf(netBuff, "%u%s", wichMessage, content);
	return sendMessage();
}
bool SckESP::sendMessage()
{

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
void SckESP::receiveMessage(ESPMessage wichMessage)
{

	switch(wichMessage)
{

	case ESPMES_SET_CONFIG:
	{

			// TODO put this in a reusable function
			StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(netBuff);
			uint8_t intMode = json["mo"];
			SCKmodes wichMode = static_cast<SCKmodes>(intMode);
			config.mode = wichMode;
			config.publishInterval = json["pi"];
			config.credentials.set = json["cs"];
			strcpy(config.credentials.ssid, json["ss"]);
			strcpy(config.credentials.pass, json["pa"]);
			config.token.set = json["ts"];
			strcpy(config.token.token, json["to"]);

			saveConfig(config);
			break;

	}
	case ESPMES_GET_NETINFO:

		sendNetinfo();
		break;

	case ESPMES_GET_TIME:

		sendTime();
		break;

	case ESPMES_MQTT_HELLO:

		if (mqttHellow()) sendMessage(SAMMES_MQTT_HELLO_OK, "");
		break;

	case ESPMES_MQTT_PUBLISH:
	{

			debugOUT("Receiving new readings...");

			// Parse input
			StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(netBuff);

			// Iterate over all sensors
			uint8_t count = 0;
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {

				SensorType wichSensor = static_cast<SensorType>(i);

				// Check if sensor exists in received readings (we asume that missing sensors are disabled)
				if (sensors[wichSensor].enabled && sensors[wichSensor].id > 0) {

					sensors[wichSensor].lastReadingTime = json["t"];
					float temp = json["sensors"][count];
					count++;
					sensors[wichSensor].reading = String(temp);
					sensors[wichSensor].enabled = true;

				} else {
					sensors[wichSensor].enabled = false;
				}
			}

			if (mqttPublish()) sendMessage(SAMMES_MQTT_PUBLISH_OK, "");
			break;

	}
	case ESPMES_START_AP:

		startAP();
		break;

	case ESPMES_STOP_AP:

		stopAP();
		break;

	default: break;
}
}

// **** MQTT
bool SckESP::mqttConnect()
{

	if (MQTTclient.connected()) return true;

	debugOUT(F("Connecting to MQTT server..."));

	MQTTclient.setServer(MQTT_SERVER_NAME, 1883);

	if (MQTTclient.connect(config.token.token)) {
		debugOUT(F("Established MQTT connection..."));
		return true;
	} else {
		debugOUT(F("ERROR: MQTT connection failed!!!"));
		debugOUT(String(MQTTclient.state()));
		return false;
	}
}
bool SckESP::mqttHellow()
{

	debugOUT(F("Trying MQTT Hello..."));

	if (mqttConnect()) {

		// prepare the topic title
		char myTopic[24];
		sprintf(myTopic, "device/sck/%s/hello", config.token.token);

		// Prepare the payload
		char myPayload[14];
		sprintf(myPayload, "%s:Hello", config.token.token);

		if (MQTTclient.publish(myTopic, myPayload)) {
			debugOUT(F("MQTT Hello published OK !!!"));
			return true;
		}
		debugOUT(F("MQTT Hello ERROR !!!"));
		}
	return false;
}
bool SckESP::mqttPublish()
{

	debugOUT(F("Trying MQTT publish..."));

	if (mqttConnect()) {

		// /* Example
		// {	"data":[
		// 		{"recorded_at":"2017-03-24T13:35:14Z",
		// 			"sensors":[
		// 				{"id":29,"value":48.45},
		// 				{"id":13,"value":66},
		// 				{"id":12,"value":28},
		// 				{"id":10,"value":4.45}
		// 			]
		// 		}
		// 	]
		// }
		// 	*/

		// Prepare the payload
		char myPayload[1024];

		// Put time of the first sensor
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);
			if (sensors[wichSensor].enabled && sensors[wichSensor].id != 0) {
				sprintf(myPayload, "{\"data\":[{\"recorded_at\":\"%s\",\"sensors\":[", epoch2iso(sensors[wichSensor].lastReadingTime).c_str());
			}
		}

		// Put the readings
		bool putComma = false;
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			SensorType wichSensor = static_cast<SensorType>(i);

			// Only send enabled sensors
			if (sensors[wichSensor].enabled && sensors[wichSensor].id != 0) {
				if (putComma) sprintf(myPayload, "%s,", myPayload);
				else putComma = true;
				sprintf(myPayload, "%s{\"id\":%i,\"value\":%s}", myPayload, sensors[wichSensor].id, sensors[wichSensor].reading.c_str());
			}
		}
		sprintf(myPayload, "%s]}]}", myPayload);

		debugOUT(String(myPayload));

		// prepare the topic title
		char pubTopic[27];
		sprintf(pubTopic, "device/sck/%s/readings", config.token.token);

		debugOUT(String(pubTopic));

		if (MQTTclient.publish(pubTopic, myPayload)) {
			debugOUT(F("MQTT readings published OK !!!"));
			return true;
		}
	}
	debugOUT(F("MQTT publish ERROR !!!"));
	return false;
}


// **** Notifications
bool SckESP::sendNetinfo()
{

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
bool SckESP::sendTime()
{

	// Update time
	debugOUT(F("Trying NTP Sync..."));
	Udp.begin(8888);
	setSyncProvider(ntpProvider);
	setSyncInterval(300);
}

// **** APmode and WebServer
void SckESP::startAP()
{

	debugOUT(String F("Starting Ap with ssid: ") + String(hostname));

	// IP for DNS
	IPAddress myIP(192, 168, 1, 1);

	// Start Soft AP
	WiFi.disconnect(true);
	WiFi.mode(WIFI_AP);
	delay(100);
	WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP((const char *)hostname);
	delay(500);

	// DNS stuff (captive portal)
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", myIP);

	/* startWebServer(); */
}
void SckESP::stopAP()
{
	dnsServer.stop();
	WiFi.softAPdisconnect(true);

	// TODO stop webserver?
	tryConnection();
}

// **** Configuration
bool SckESP::saveConfig(Configuration newConfig)
{

	if ((config.credentials.ssid != newConfig.credentials.ssid) || !newConfig.credentials.set) WiFi.disconnect();

	config = newConfig;

	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["mo"] = (uint8_t)config.mode;
	json["pi"] = config.publishInterval;
	json["cs"] = (uint8_t)config.credentials.set;
	json["ss"] = config.credentials.ssid;
	json["pa"] = config.credentials.pass;
	json["ts"] = (uint8_t)config.token.set;
	json["to"] = config.token.token;

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
bool SckESP::loadConfig()
{

	if (SPIFFS.exists(configFileName)) {

		File configFile = SPIFFS.open(configFileName, "r");

		StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
		JsonObject &json = jsonBuffer.parseObject(configFile);

		if (json.success()) {

			uint8_t intMode = json["mo"];
			SCKmodes wichMode = static_cast<SCKmodes>(intMode);
			config.mode = wichMode;

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
void SckESP::ledSet(uint8_t value)
{
	blink.detach();
	ledValue = abs(value - 1);
	digitalWrite(pinLED, ledValue);
}
void SckESP::ledBlink(float rate)
{

	blink.attach_ms(rate, ledToggle);
}
void SckESP::_ledToggle()
{
	ledValue = abs(ledValue - 1);
	digitalWrite(pinLED, ledValue);
}

// **** Time
time_t SckESP::getNtpTime()
{
	IPAddress ntpServerIP;

	while (Udp.parsePacket() > 0) ; // discard any previously received packets
	WiFi.hostByName(NTP_SERVER_NAME, ntpServerIP);

	sendNTPpacket(ntpServerIP);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 200) {
		int size = Udp.parsePacket();
		if (size >= 48) {
			Udp.read(packetBuffer, 48);  // read packet into the buffer
			unsigned long secsSince1900;
			// convert four bytes starting at location 40 to a long integer
			secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
			secsSince1900 |= (unsigned long)packetBuffer[43];

			debugOUT(F("Time updated!!!"));
			String epochStr = String(secsSince1900 - 2208988800UL);
			sendMessage(SAMMES_TIME, epochStr.c_str());
			return secsSince1900 - 2208988800UL;
		}
	}
	debugOUT(F("No NTP Response!!!"));
	return 0;
}
void SckESP::sendNTPpacket(IPAddress &address)
{
	memset(packetBuffer, 0, 48);

	// Initialize values needed to form NTP request
	packetBuffer[0] = 0b11100011;
	packetBuffer[1] = 0;
	packetBuffer[2] = 6;
	packetBuffer[3] = 0xEC;

	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	Udp.beginPacket(address, 123);
	Udp.write(packetBuffer, 48);
	Udp.endPacket();
}
String SckESP::ISOtime()
{
	// Return string.format("%04d-%02d-%02dT%02d:%02d:%02dZ", tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
	if (timeStatus() == timeSet) {
		String isoTime = String(year()) + "-" +
			leadingZeros(String(month()), 2) + "-" +
			leadingZeros(String(day()), 2) + "T" +
			leadingZeros(String(hour()), 2) + ":" +
			leadingZeros(String(minute()), 2) + ":" +
			leadingZeros(String(second()), 2) + "Z";
		return isoTime;
	} else {
		return "0";
	}
}
String SckESP::leadingZeros(String original, int decimalNumber)
{
	for (uint8_t i=0; i < (decimalNumber - original.length()); ++i)	{
		original = "0" + original;
	}
	return original;
}
String SckESP::epoch2iso(uint32_t toConvert)
{

	time_t tc = toConvert;

	String isoTime = String(year(tc)) + "-" +
		leadingZeros(String(month(tc)), 2) + "-" +
		leadingZeros(String(day(tc)), 2) + "T" +
		leadingZeros(String(hour(tc)), 2) + ":" +
		leadingZeros(String(minute(tc)), 2) + ":" +
		leadingZeros(String(second(tc)), 2) + "Z";

	return isoTime;
}
