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

	ledBlink(LED_SLOW);

	if (telnetDebug) {
		Debug.begin(hostname);
		Debug.setResetCmdEnabled(true);
		Debug.showColors(true);
		Debug.setSerialEnabled(false);
	}
	sendMessage(SAMMES_DEBUG, "starting...");
	if (!sendMessage(SAMMES_BOOTED)) bootedPending = true;
}
void SckESP::update()
{
	if (bootedPending) {
		if (sendMessage(SAMMES_BOOTED)) bootedPending = false;
	}

	if (WiFi.getMode() == WIFI_AP) {
		dnsServer.processNextRequest();
		webServer.handleClient();
	}

	if (WiFi.status() != currentWIFIStatus) {
		currentWIFIStatus = WiFi.status();
		switch (currentWIFIStatus) {
			case WL_CONNECTED:
			{
				ledSet(1);
				while (!sendMessage(SAMMES_WIFI_CONNECTED)) {
					debugOUT("Failed sending wifi notification to SAM!!!");
					delay(500);
				}
				break;
			}
			case WL_CONNECT_FAILED: ledBlink(LED_FAST); sendMessage(SAMMES_PASS_ERROR); break;
			case WL_NO_SSID_AVAIL: ledBlink(LED_FAST); sendMessage(SAMMES_SSID_ERROR); break;
			case WL_DISCONNECTED:
				if (config.credentials.set && WiFi.getMode() != WIFI_AP) ledBlink(LED_SLOW);
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
		ledBlink(LED_SLOW);
		// Support for open networks
		String tp = String(config.credentials.pass);
		if (tp.length() == 0) WiFi.begin(config.credentials.ssid);
		else WiFi.begin(config.credentials.ssid, config.credentials.pass);

	} else {
		debugOUT(String F("Already connected to wifi: ") + String(WiFi.SSID()));
	}
}
void SckESP::wifiOFF()
{
	WiFi.mode(WIFI_OFF);
}

// **** Input/Output
void SckESP::debugOUT(String strOut)
{
	if (telnetDebug) {
		strOut += "\r\n";
		Debug.println(strOut.c_str());
	}

	if (serialDebug) sendMessage(SAMMES_DEBUG, strOut.c_str());
}
void SckESP::SAMbusUpdate()
{

	if (manager.available()) {

		uint8_t len = NETPACK_TOTAL_SIZE;

		if (manager.recvfromAck(netPack, &len)) {

			// Identify received command
			uint8_t pre = netPack[1];
			ESPMessage wichMessage = static_cast<ESPMessage>(pre);

			// Get content from first package (1 byte less than the rest)
			memcpy(netBuff, &netPack[2], NETPACK_CONTENT_SIZE - 1);

			// Het the rest of the packages (if they exist)
			for (uint8_t i=0; i<netPack[0]-1; i++) {
				if (manager.recvfromAckTimeout(netPack, &len, 500))	{
					memcpy(&netBuff[(i * NETPACK_CONTENT_SIZE) + (NETPACK_CONTENT_SIZE - 1)], &netPack[1], NETPACK_CONTENT_SIZE);
				}
				else return;
			}

			// Process message
			receiveMessage(wichMessage);
		}
	}
}
bool SckESP::sendMessage(SAMMessage wichMessage)
{
	sprintf(netBuff, "%c", wichMessage);
	return sendMessage();
}
bool SckESP::sendMessage(SAMMessage wichMessage, const char *content)
{
	sprintf(netBuff, "%c%s", wichMessage, content);
	return sendMessage();
}
bool SckESP::sendMessage()
{
	uint16_t totalSize = strlen(netBuff);
	uint8_t totalParts = (totalSize + NETPACK_CONTENT_SIZE - 1)  / NETPACK_CONTENT_SIZE;

	for (uint8_t i=0; i<totalParts; i++) {
		netPack[0] = totalParts;
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
				StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(netBuff);
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
				if (mqttPublish()) {
					delay(500);
					sendMessage(SAMMES_MQTT_PUBLISH_OK, "");
				} else sendMessage(SAMMES_MQTT_PUBLISH_ERROR, "");
				break;
		}
		case ESPMES_MQTT_INVENTORY:
		{
				debugOUT("Receiving MQTT inventory...");
				if (mqttInventory()) {
					delay(500);
					sendMessage(SAMMES_MQTT_PUBLISH_OK, "");
				} else sendMessage(SAMMES_MQTT_PUBLISH_ERROR, "");
				break;
		}
		case ESPMES_MQTT_CUSTOM:
		{
				debugOUT("Receiving MQQT custom publish request...");
				if (mqttCustom()) {
					delay(500);
					sendMessage(SAMMES_MQTT_CUSTOM_OK, "");
				} else sendMessage(SAMMES_MQTT_CUSTOM_ERROR, "");
				break;
		}
		case ESPMES_CONNECT:

			tryConnection();
			break;

		case ESPMES_START_AP:

			startAP();
			break;

		case ESPMES_STOP_AP:

			stopAP();
			break;
		case ESPMES_LED_OFF:

			ledSet(0);
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

		// Prepare the topic title
		char pubTopic[27];
		sprintf(pubTopic, "device/sck/%s/readings", config.token.token);

		debugOUT(String(pubTopic));
		debugOUT(String(netBuff));

		if (MQTTclient.publish(pubTopic, netBuff)) {
			debugOUT(F("MQTT readings published OK !!!"));
			return true;
		}
	}
	debugOUT(F("MQTT publish ERROR !!!"));
	return false;
}
bool SckESP::mqttInventory()
{
	debugOUT(F("Trying MQTT inventory..."));
	if (mqttConnect()) {
		if (MQTTclient.publish("device/inventory", netBuff)) {
			debugOUT(F("MQTT inventory published OK!!"));
			return true;
		}
	}
	return false;
}
bool SckESP::mqttCustom()
{
	debugOUT(F("Trying custom MQTT..."));
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(netBuff);

	if (mqttConnect()) {
		if (MQTTclient.publish(json["to"], json["pl"])) {
			debugOUT(F("Custom MQTT published OK!!"));
			return true;
		}
	}
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
	jsonSend["ver"] = ESPversion;
	jsonSend["bd"] = ESPbuildDate;

	sprintf(netBuff, "%c", SAMMES_NETINFO);
	jsonSend.printTo(&netBuff[1], jsonSend.measureLength() + 1);

	return sendMessage();
}
bool SckESP::sendTime()
{
	if (timeStatus() == timeNotSet) {
		debugOUT("Time is not synced!!!");
		setNTPprovider();
	} else {
		String epochSTR = String(now());
		debugOUT("Sending time to SAM: " + epochSTR);
		sendMessage(SAMMES_TIME, epochSTR.c_str());
	}
	return true;
}

// **** APmode and WebServer
void SckESP::startAP()
{
	scanAP();

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

	startWebServer();
	ledBlink(LED_FAST);
}
void SckESP::stopAP()
{
	dnsServer.stop();
	WiFi.softAPdisconnect(true);

	// TODO stop webserver?
	tryConnection();
}
void SckESP::startWebServer()
{
	// Android captive portal.
	webServer.on("/generate_204", [&](){
		flashReadFile("/");
	});

	// Microsoft captive portal.
	webServer.on("/fwlink", [&](){
		flashReadFile("/");
	});

	// Handle files from flash
	webServer.onNotFound([&](){
		flashReadFile(webServer.uri());
	});

	// Handle set
	// /set?ssid=value1&password=value2&token=value3&epoch=value&pubint=60&mode=value
	//	ssid
	//	password
	//	token
	//	epoch
	//	pubint
	//	mode
	webServer.on("/set", extSet);

	// Console
	// /console?com=get sensors
	/* webServer.on("/console", [&](){ */

	/* 	debugOUT(F("Received web console request.")); */

	/* 	clearParam(); */
	/* 	strncpy(msgOut.param, webServer.arg(0).c_str(), 240); */
	/* 	msgOut.com = ESP_CONSOLE_COM; */
	/* 	SAMsendMsg(); */

	/* }); */

	// Handle status request
	webServer.on("/status", extStatus);

	// Handle ping request
	webServer.on("/ping", [&](){

		debugOUT(F("Received web ping request."));

		webServer.send(200, "text/plain", "");
	});

	// Handle APlist request
	webServer.on("/aplist", [&](){

		debugOUT(F("Received web Access point request."));

		String json = "{\"nets\":[";

		// int netNum = WiFi.scanNetworks();
		for (int i=0; i<netNumber; i++) {
			json += "{\"ssid\":\"" + String(WiFi.SSID(i));
			json += "\",\"ch\":" + String(WiFi.channel(i));
			json += ",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
			if (i < (netNumber - 1)) json += ",";
		}

		json += "]}";
		webServer.send(200, "text/json", json);
		json = String();
	});

	// Handle SSDP
	webServer.on("/description.xml", HTTP_GET, [](){
		SSDP.schema(webServer.client());
	});

	webServer.begin();
	debugOUT("Started Webserver!!!");

	// SSDP description
	SSDP.setSchemaURL("description.xml");
	SSDP.setHTTPPort(80);
	SSDP.setName("SmartCitizen Kit");
	SSDP.setModelName("1.5");
	SSDP.setModelURL("http://www.smartcitizen.me");
	SSDP.begin();
}
void SckESP::webSet()
{
	debugOUT(F("Received web configuration request."));

	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonConf = jsonBuffer.createObject();
	String json = "{";

	// If we found ssid AND pass
	if (webServer.hasArg("ssid"))  {

		String tssid = webServer.arg("ssid");
		String tpass = "";
		if (webServer.hasArg("password")) tpass = webServer.arg("password");

		// If ssid is no zero chars
		if (tssid.length() > 0) {
			config.credentials.set = true;
			tssid.toCharArray(config.credentials.ssid, 64);
			tpass.toCharArray(config.credentials.pass, 64);
			jsonConf["ss"] = config.credentials.ssid;
			jsonConf["pa"] = config.credentials.pass;
			debugOUT(F("Wifi credentials updated from apmode web!!!"));
		} else {
			config.credentials.set = false;
			strncpy(config.credentials.ssid, "", 64);
			strncpy(config.credentials.pass, "", 64);
			json += "\"ssid\":\"false\",";
			debugOUT(F("Invalid Wifi credentials received from apmode web!!!"));
		}
	} else {
		json += "\"ssid\":\"false\",";
	}

	// If we found the mode
	if (webServer.hasArg("mode")) {
		String stringMode = webServer.arg("mode");
		if (stringMode.equals("sdcard")) {
			json += "\"mode\":\"true\",";
			jsonConf["mo"] = "sd";
			debugOUT(F("Mode set to sdcard from apmode web!!!"));
		} else if (stringMode.equals("network")) {
			json += "\"mode\":\"true\",";
			jsonConf["mo"] = "net";
			debugOUT(F("Mode set to network from apmode web!!!"));
		} else {
			json += "\"mode\":\"false\",";
			debugOUT(F("Invalid mode from apmode web!!!"));
		}
	} else {
		json += "\"mode\":\"false\",";
	}

	// If we found the token
	if (webServer.hasArg("token")) {
		String stringToken = webServer.arg("token");
		if (stringToken.length() == 6) {
			config.token.set = true;
			stringToken.toCharArray(config.token.token, 8);
			json += "\"token\":\"true\",";
			jsonConf["to"] = config.token.token;
			debugOUT(F("Token updated from apmode web!!!"));
		} else {
			config.token.set = false;
			json += "\"token\":\"false\",";
			strncpy(config.token.token, "", 64);
			debugOUT(F("Invalid Token received from apmode web!!!"));
		}
	} else {
		json += "\"token\":\"false\",";
	}

	// If we found new time
	if (webServer.hasArg("epoch")) {

		String tepoch = webServer.arg("epoch");
		uint32_t iepoch = tepoch.toInt();
		const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

		if (iepoch >= DEFAULT_TIME) {
			setTime(iepoch);
			debugOUT(F("Time updated from apmode web!!!"));
			if (sendMessage(SAMMES_TIME, tepoch.c_str())) debugOUT(F("Time sent to SAM!!!"));
			else debugOUT(F("ERROR failed to send time to SAM"));
		} else {
			debugOUT(F("Invalid time received from apmode web!!!"));
		}
	} else {
		json += "\"time\":\"false\",";
	}

	// Publish interval (seconds)
	if (webServer.hasArg("pubint")) {

		String tinterval = webServer.arg("pubint");
		uint32_t intTinterval = tinterval.toInt();

		if (intTinterval > 0) {
			jsonConf["pi"] = intTinterval;
			json += "\"pubint\":\"true\"";
			debugOUT(F("Publish interval changed from apmode web!!!"));
		} else {
			json += "\"pubint\":\"false\"";
			debugOUT(F("Invalid publish interval from apmode web!!!"));
		}

	} else {
		json += "\"pubint\":\"false\"";
	}
	json += "\"pubint\":\"false\"";

	sprintf(netBuff, "%c", SAMMES_SET_CONFIG);
	jsonConf.printTo(&netBuff[1], jsonConf.measureLength() + 1);
	if (sendMessage()) {
		debugOUT(F("Sent configuration to SAM!!"));
	} else debugOUT(F("ERROR Failed to send config to SAM!!!"));

	webServer.send(200, "text/json", json);
}
bool SckESP::flashReadFile(String path)
{
	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
		return false;
	}

	debugOUT("Received file request: " + path);

	// send index file in case no file is requested
	if (path.endsWith("/")) path += "index.gz";

	// Manage content types
	String contentType = "text/html";

	if(path.endsWith(".css")) contentType = "text/css";
	else if(path.endsWith(".js")) contentType = "application/javascript";
	else if(path.endsWith(".png")) contentType = "image/png";
	else if(path.endsWith(".gif")) contentType = "image/gif";
	else if(path.endsWith(".jpg")) contentType = "image/jpeg";
	else if(path.endsWith(".ico")) contentType = "image/x-icon";

	if (SPIFFS.exists(path)) {

		File file = SPIFFS.open(path, "r");
		webServer.streamFile(file, contentType);
		file.close();

		return true;
	}
	webServer.send(404, "text/plain", "File Not Found, maybe you need to upload ESP filesystem...");
	return false;
}
bool SckESP::captivePortal()
{
	if (!isIp(webServer.hostHeader()) && webServer.hostHeader() != (String(hostname)+".local")) {
		webServer.sendHeader("Location", String("http://") + toStringIp(webServer.client().localIP()), true);
		webServer.send (302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
		webServer.client().stop(); // Stop is needed because we sent no content length
		return true;
	}
	return false;
}
bool SckESP::isIp(String str)
{
	for (uint8_t i=0; i<str.length(); i++) {
		uint8_t c = str.charAt(i);
		if (c != '.' && (c < '0' || c > '9')) return false;
	}
	return true;
}
String SckESP::toStringIp(IPAddress ip)
{
	String res = "";
	for (int i = 0; i < 3; i++) {
		res += String((ip >> (8 * i)) & 0xFF) + ".";
	}
	res += String(((ip >> 8 * 3)) & 0xFF);
	return res;
}
void SckESP::webStatus()
{

	debugOUT(F("Received web status info request."));

	// Token
	String json;
	if (config.token.set) json = "{\"token\":\"" + String(config.token.token) + "\",";
	else json = "{\"token\":\"null\",";

	// Wifi config
	if (config.credentials.set) {
		json += "\"ssid\":\"" + String(config.credentials.ssid) + "\",";
		json += "\"password\":\"" + String(config.credentials.pass) + "\",";
	} else {
		json += "\"ssid\":\"null\",";
		json += "\"password\":\"null\",";
	}

	// mode
	json +=  "\"mode\":\"wip\",";

	// Hostname
	json += "\"hostname\":\"" + String(hostname) + "\",";

	// IP address
	String tip = WiFi.localIP().toString();
	json += "\"ip\":\"" + tip + "\",";

	// MAC address
	String tmac = WiFi.softAPmacAddress();
	json += "\"mac\":\"" + tmac + "\",";

	// Time
	String epochSTR = "0";
	if (timeStatus() == timeSet) epochSTR = String(now());
	json += "\"time\":" + epochSTR + ",";

	// Battery
	String battPresent = "wip";
	json += "\"batt\":\"" + battPresent + "\",";
	uint8_t battPercent = 0;
	json += "\"battPercent\":" + String(battPercent) + ",";
	String charging = "wip";
	json += "\"charging\":\"" + charging + "\",";

	// SDcard
	String sdPresent = "wip";
	json += "\"sdcard\":\"" + sdPresent + "\",";

	// ESP firmware version
	json += "\"ESPversion\":\"" + ESPversion + "\",";

	// ESP build date
	json += "\"ESPbuilddate\":\"" + ESPbuildDate + "\",";

	String wip = "wip";
	// SAM firmware version
	json += "\"SAMversion\":\"" + wip + "\",";

	// SAM build date
	json += "\"SAMbuilddate\":\"" + wip + "\",";

	// Acces Point status
	json += "\"apstatus\":\"to be removed\",";

	// Wifi status
	json += "\"wifi\":\"to be removed\",";

	// MQTT status
	json += "\"mqtt\":\"to be removed\",";

	// Last publish time
	json += "\"last_publish\":\"to be removed\"";

	json += "}";
	webServer.send(200, "text/json", json);
	json = String();
}
void SckESP::scanAP()
{
	debugOUT(F("Scaning Wifi networks..."));

	netNumber = WiFi.scanNetworks();
	// Wait for scan...
	while (WiFi.scanComplete() == -2) {
		;
	}

	debugOUT(String(netNumber) + F(" networks found"));
}

// **** Configuration
bool SckESP::saveConfig(ESP_Configuration newConfig)
{
	config = newConfig;
	return saveConfig();
}
bool SckESP::saveConfig()
{
	debugOUT("Saving config...");
	if ((config.credentials.ssid != config.credentials.ssid) || !config.credentials.set) WiFi.disconnect();

	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
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
		} else {
			startAP();
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
	ESP_Configuration newConfig;
	config = newConfig;
	saveConfig(config);
	return false;
}

// **** Led
void SckESP::ledSet(uint8_t value)
{
	blink.detach();
	if (value) analogWrite(pinLED, 820);
	else digitalWrite(pinLED, HIGH);
}
void SckESP::ledBlink(float rate)
{
	blink.attach_ms(rate, ledToggle);
}
void SckESP::_ledToggle()
{
	ledValue = abs(ledValue - 1);
	/* digitalWrite(pinLED, ledValue); */
	if (ledValue) analogWrite(pinLED, 820);
	else digitalWrite(pinLED, HIGH);
}

// **** Time
void SckESP::setNTPprovider()
{
	// Update time
	debugOUT(F("Trying NTP Sync..."));
	Udp.begin(8888);
	setSyncProvider(ntpProvider);
	setSyncInterval(300);
}
time_t SckESP::getNtpTime()
{
	IPAddress ntpServerIP;

	while (Udp.parsePacket() > 0) ; // discard any previously received packets
	WiFi.hostByName(NTP_SERVER_NAME, ntpServerIP);

	sendNTPpacket(ntpServerIP);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
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

	Udp.beginPacket(address, NTP_SERVER_PORT);
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

