#include "SckESP.h"

// SAM communication
RH_Serial driver(Serial);
RHReliableDatagram manager(driver, ESP_ADDRESS);

// Telnet debug
RemoteDebug Debug;

// Web Server
AsyncWebServer webServer(80);

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

	if (config.debug_telnet) {
		Debug.begin(hostname);
		Debug.setResetCmdEnabled(true);
		Debug.showColors(true);
		Debug.showTime(true);
		Debug.setSerialEnabled(false);
	}
	if (!sendStartInfo()) bootedPending = true;

	// Date for Web server
	sprintf(last_modified, "%s %s GMT", __DATE__, __TIME__);

	// MQTT pubSubClient settings
	MQTTclient.setKeepAlive(MQTT_KEEP_ALIVE);
	MQTTclient.setBufferSize(MQTT_BUFF_SIZE);
}
void SckESP::update()
{
	if (bootedPending) {
		if (sendStartInfo()) bootedPending = false;
		return;
	}
	if (sendConfigPending) {
		if (sendTime()) {
			if (sendConfig()) sendConfigPending = false;
		}
	}

	if (WiFi.getMode() == WIFI_AP) {
		dnsServer.processNextRequest();
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

	if(shouldReboot) ESP.restart();

	if (config.debug_telnet) Debug.handle();
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
	if (config.debug_telnet) {
		strOut += "\r\n";
		Debug.printf(strOut.c_str());
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

			// Empty netBuff before starting to store the new message
			memset(netBuff, 0, sizeof(netBuff));

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
			StaticJsonDocument<JSON_BUFFER_SIZE> jsonBuffer;
			deserializeJson(jsonBuffer, netBuff);
			JsonObject json = jsonBuffer.as<JsonObject>();
			config.credentials.set = json["cs"];
			strcpy(config.credentials.ssid, json["ss"]);
			strcpy(config.credentials.pass, json["pa"]);
			config.token.set = json["ts"];
			strcpy(config.token.token, json["to"]);
			strcpy(config.mqtt.server, json["ms"]);
			config.mqtt.port = json["mp"];
			strcpy(config.ntp.server, json["ns"]);
			config.ntp.port = json["np"];
			SAMversion = json["ver"].as<String>();
			SAMbuildDate = json["bd"].as<String>();
			uint8_t action = json["ac"];
			ESPMessage wichAction = static_cast<ESPMessage>(action);
			config.debug_telnet = json["tn"];

			// Do we need to update ESP firmware?
			VersionInt ESPversionInt = parseVersionStr(ESPversion);
			VersionInt SAMversionInt = parseVersionStr(SAMversion);

			if ((SAMversionInt.mayor != ESPversionInt.mayor) || (SAMversionInt.minor != ESPversionInt.minor)) updateNeeded = true;
			else updateNeeded= false;

			saveConfig(config);

			if (wichAction == ESPMES_START_AP) startAP();
			else if (wichAction == ESPMES_CONNECT) tryConnection();

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
				if (mqttPublishRaw()) {
					sendMessage(SAMMES_MQTT_PUBLISH_OK, "");
				} else sendMessage(SAMMES_MQTT_PUBLISH_ERROR, "");
				break;
		}
		case ESPMES_MQTT_INVENTORY:
		{
				debugOUT("Receiving MQTT inventory...");
				if (mqttInventory()) {
					sendMessage(SAMMES_MQTT_PUBLISH_OK, "");
				} else sendMessage(SAMMES_MQTT_PUBLISH_ERROR, "");
				break;
		}
		case ESPMES_MQTT_INFO:
		{
				debugOUT("Receiving new info...");
				if (mqttInfo()) {
					sendMessage(SAMMES_MQTT_INFO_OK, "");
				} else sendMessage(SAMMES_MQTT_INFO_ERROR, "");
				break;
		}
		case ESPMES_MQTT_CUSTOM:
		{
				debugOUT("Receiving MQQT custom publish request...");
				if (mqttCustom()) {
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

	MQTTclient.setServer(config.mqtt.server, config.mqtt.port);

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

		char pubPayload[MQTT_BUFF_SIZE];
		

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

		char thisTime[21];
		snprintf(thisTime, 21, &netBuff[3]);
		sprintf(pubPayload, "%s%s%s", "{\"data\":[{\"recorded_at\":\"", thisTime, "\",\"sensors\":[{\"id\":");

		for (uint16_t i=24; i<strlen(netBuff); i++) {
			
			char thisChar[2];
			snprintf(thisChar, 2, &netBuff[i]);

			if (netBuff[i] == ':') sprintf(pubPayload, "%s%s", pubPayload, ",\"value\":");
			else if (netBuff[i] == ',') sprintf(pubPayload, "%s%s", pubPayload, "},{\"id\":");
			else sprintf(pubPayload, "%s%s", pubPayload, thisChar);
		}

		sprintf(pubPayload, "%s%s", pubPayload, "]}]}");

		debugOUT(String(pubPayload));

		if (MQTTclient.publish(pubTopic, pubPayload)) {
			debugOUT(F("MQTT readings published OK !!!"));
			return true;
		}
	}
	debugOUT(F("MQTT publish ERROR !!!"));
	return false;
}
bool SckESP::mqttPublishRaw()
{
	debugOUT(F("Trying MQTT publish..."));

	if (mqttConnect()) {

		// Prepare the topic title
		char pubTopic[32];
		sprintf(pubTopic, "device/sck/%s/readings/raw", config.token.token);

		debugOUT(String(pubTopic));
		debugOUT(String(netBuff));

		// /* Example
		// {	t:2017-03-24T13:35:14Z,
		// 		29:48.45,
		// 		13:66,
		// 		12:28,
		// 		10:4.45
		// }

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
bool SckESP::mqttInfo()
{
	debugOUT(F("Trying MQTT info..."));

	if (mqttConnect()) {

		// Prepare the topic title
		char pubTopic[23];
		sprintf(pubTopic, "device/sck/%s/info", config.token.token);

		debugOUT(String(pubTopic));
		debugOUT(String(netBuff));

		if (MQTTclient.publish(pubTopic, netBuff)) {
			debugOUT(F("MQTT info published OK !!!"));
			return true;
		}
	}
	debugOUT(F("MQTT info ERROR !!!"));
	return false;
}
bool SckESP::mqttCustom()
{
	debugOUT(F("Trying custom MQTT..."));
	StaticJsonDocument<JSON_BUFFER_SIZE> jsonBuffer;
	deserializeJson(jsonBuffer, netBuff);
	JsonObject json = jsonBuffer.as<JsonObject>();

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
	StaticJsonDocument<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject jsonSend = jsonBuffer.to<JsonObject>();
	jsonSend["hn"] = hostname;
	ipAddr = WiFi.localIP().toString();
	jsonSend["ip"] = ipAddr;

	sprintf(netBuff, "%c", SAMMES_NETINFO);
	serializeJson(jsonSend, &netBuff[1], NETBUFF_SIZE);

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
bool SckESP::sendStartInfo()
{
	StaticJsonDocument<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject jsonSend = jsonBuffer.to<JsonObject>();
	jsonSend["mac"] = macAddr;
	jsonSend["ver"] = ESPversion;
	jsonSend["bd"] = ESPbuildDate;

	sprintf(netBuff, "%c", SAMMES_BOOTED);
	serializeJson(jsonSend, &netBuff[1], NETBUFF_SIZE);

	return sendMessage();
}
bool SckESP::sendConfig()
{
	StaticJsonDocument<240> jsonBuffer;
	JsonObject jsonConf = jsonBuffer.to<JsonObject>();

	if (config.credentials.set) {
		jsonConf["ss"] = config.credentials.ssid;
		jsonConf["pa"] = config.credentials.pass;
	}

	if (sendMode == SAM_MODE_SD) jsonConf["mo"] = "sd";
	else if (sendMode == SAM_MODE_NET) jsonConf["mo"] = "net";

	if (config.token.set) jsonConf["to"] = config.token.token;

	if (sendPubInt > 0) jsonConf["pi"] = sendPubInt;

	if (jsonConf.size() <= 0) return false;

	sprintf(netBuff, "%c", SAMMES_SET_CONFIG);
	serializeJson(jsonConf, &netBuff[1], NETBUFF_SIZE);
	if (sendMessage()) {
		debugOUT(F("Sent configuration to SAM!!"));
		return true;
	}

	debugOUT(F("ERROR Failed to send config to SAM!!!"));
	return false;
}

// **** APmode and WebServer
void SckESP::startAP()
{
	if (apStarted) return;

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

	apStarted = true;
}
void SckESP::stopAP()
{
	dnsServer.stop();
	WiFi.softAPdisconnect(true);
	apStarted = false;
	tryConnection();
}
void SckESP::startWebServer()
{
	webServer.rewrite("/", "/index.html");

	// For IOS
	webServer.on("/hotspot-detect.html", HTTP_GET, extRoot);

	// Handle root
	webServer.on("/index.html", HTTP_GET, extRoot);

	// Android captive portal
	webServer.on("/generate_204", HTTP_GET, extRoot);

	// Microsoft captive portal.
	webServer.on("/fwlink", HTTP_GET, extRoot);
	webServer.on("/ncsi.txt", HTTP_GET, extRoot);

	// Handle set
	// /set?ssid=value1&password=value2&token=value3&epoch=value&pubint=60&mode=value
	//	ssid
	//	password
	//	token
	//	epoch
	//	pubint
	//	mode
	webServer.on("/set", HTTP_GET, extSet);

	// Handle status request
	webServer.on("/status", HTTP_GET, extStatus);

	// Handle token request
	webServer.on("/token", HTTP_GET, [&] (AsyncWebServerRequest *request) {
		// {"token":"123123"}
		String json = "{\"token\":\"" + String(config.token.token) + "\"}";
		request->send(200, "text/plain", json);
	});

	// Handle ping request
	webServer.on("/ping", HTTP_GET, [] (AsyncWebServerRequest *request) {
		request->send(200, "text/plain", "ping...");
	});

	// Handle APlist request
	webServer.on("/aplist", HTTP_GET, [&] (AsyncWebServerRequest *request) {

		String json = "{\"nets\":[";

		// int netNum = WiFi.scanNetworks();
		for (int i=0; i<netNumber; i++) {
			json += "{\"ssid\":\"" + String(WiFi.SSID(i));
			json += "\",\"ch\":" + String(WiFi.channel(i));
			json += ",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
			if (i < (netNumber - 1)) json += ",";
		}

		json += "]}";
		request->send(200, "text/json", json);
		json = String();
	});

	webServer.on("/update", HTTP_GET, [&](AsyncWebServerRequest *request){
			request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
		});
	webServer.on("/update", HTTP_POST, [&](AsyncWebServerRequest *request){
		shouldReboot = !Update.hasError();
		if (shouldReboot) OTAstatus = "Succeeded!";
		else OTAstatus = "ERROR...";
		AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", OTAstatus);
		response->addHeader("Connection", "close");
		request->send(response);
	},[&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
		if(!index){
			OTAstatus = "Running...";
			Update.runAsync(true);
			if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
				OTAstatus = "ERROR...";
			}
		}
		if(!Update.hasError()){
			if(Update.write(data, len) != len){
				OTAstatus = "ERROR...";
			}
		}
		if(final){
			if(Update.end(true)){
				OTAstatus = "Succeeded!";
			} else {
				OTAstatus = "ERROR...";
			}
		}
	});

	// Handle not found
	webServer.onNotFound([](AsyncWebServerRequest *request){ request->send(404); });

	webServer.begin();
}
void SckESP::webSet(AsyncWebServerRequest *request)
{
	// If we found ssid AND pass
	if (request->hasParam("ssid"))  {

		String tssid = request->arg("ssid");
		String tpass = "";
		if (request->hasParam("password")) tpass = request->arg("password");

		// If ssid is no zero chars
		if (tssid.length() > 0) {
			config.credentials.set = true;
			tssid.toCharArray(config.credentials.ssid, 64);
			tpass.toCharArray(config.credentials.pass, 64);
			sendConfigPending = true;
		} else {
			config.credentials.set = false;
			strncpy(config.credentials.ssid, "", 64);
			strncpy(config.credentials.pass, "", 64);
		}
	}

	// If we found the mode
	if (request->hasParam("mode")) {
		String stringMode = request->arg("mode");
		if (stringMode.equals("sdcard")) {
			sendMode = SAM_MODE_SD;
			sendConfigPending = true;
		} else if (stringMode.equals("network")) {
			sendMode = SAM_MODE_NET;
			sendConfigPending = true;
		}
	}

	// If we found the token
	if (request->hasParam("token")) {
		String stringToken = request->arg("token");
		if (stringToken.length() == 6) {
			config.token.set = true;
			stringToken.toCharArray(config.token.token, 8);
			sendConfigPending = true;
		} else {
			config.token.set = false;
			strncpy(config.token.token, "", 64);
		}
	}

	// If we found new time
	if (request->hasParam("epoch")) {

		String tepoch = request->arg("epoch");
		uint32_t iepoch = tepoch.toInt();
		const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

		if (iepoch >= DEFAULT_TIME) {
			setTime(iepoch);
			sendConfigPending = true;
		}
	}

	// Publish interval (seconds)
	if (request->hasParam("pubint")) {

		String tinterval = request->arg("pubint");
		uint32_t intTinterval = tinterval.toInt();

		if (intTinterval > 0) {
			sendPubInt = intTinterval;
			sendConfigPending = true;
		}
	}
}
void SckESP::webRoot(AsyncWebServerRequest *request)
{
    // Check if the client already has the same version and respond with a 304 (Not modified)
    if (request->header("If-Modified-Since").equals(last_modified)) {
        request->send(304);

    } else {
	
        // Dump the byte array in PROGMEM with a 200 HTTP code (OK)
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);

        // Tell the browswer the contemnt is Gzipped
        response->addHeader("Content-Encoding", "gzip");

        // And set the last-modified datetime so we can check if we need to send it again next time or not
        response->addHeader("Last-Modified", last_modified);

        request->send(response);

    }
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
void SckESP::webStatus(AsyncWebServerRequest *request)
{
	String json;

	// Hostname
	json += "{\"hostname\":\"" + String(hostname) + "\",";

	// MAC address
	String tmac = WiFi.softAPmacAddress();
	json += "\"mac\":\"" + tmac + "\",";

	// ESP firmware version
	json += "\"ESPversion\":\"" + ESPversion.substring(0, ESPversion.indexOf("-")) + "\",";

	// ESP firmware commit
	json += "\"ESPcommit\":\"" + ESPversion.substring(ESPversion.indexOf("-")+1, ESPversion.length()) + "\",";

	// ESP build date
	json += "\"ESPbuilddate\":\"" + ESPbuildDate + "\",";

	// SAM firmware version
	json += "\"SAMversion\":\"" + SAMversion.substring(0, SAMversion.indexOf("-")) + "\",";

	// SAM firmware commit
	json += "\"SAMcommit\":\"" + SAMversion.substring(SAMversion.indexOf("-")+1, SAMversion.length()) + "\",";

	// SAM build date
	json += "\"SAMbuilddate\":\"" + SAMbuildDate + "\",";

	// ESP update needed
	json += "\"updateNeeded\":\"" +  String(updateNeeded ? "true" : "false") + "\"";

	json += "}";
	request->send(200, "text/json", json);

	json = String();
}
void SckESP::scanAP()
{
	debugOUT(F("Scaning Wifi networks..."));
	ledBlink(50);
	
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(100);
	WiFi.scanNetworks(false); 

	// For some reason it runs in async mode so we wait for it to finish
	netNumber = WiFi.scanComplete();
	while (netNumber == WIFI_SCAN_RUNNING) {
		netNumber = WiFi.scanComplete();
		delay(50);
	}

	if (netNumber == WIFI_SCAN_FAILED) {
		debugOUT("Error on WiFi scanning... retrying");
		scanAP();
	} else {
		debugOUT(String(netNumber) + F(" networks found"));
	}
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

	StaticJsonDocument<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject json = jsonBuffer.to<JsonObject>();
	json["cs"] = (uint8_t)config.credentials.set;
	json["ss"] = config.credentials.ssid;
	json["pa"] = config.credentials.pass;
	json["ts"] = (uint8_t)config.token.set;
	json["to"] = config.token.token;
	json["tn"] = config.debug_telnet;
	json["ms"] = config.mqtt.server;
	json["mp"] = config.mqtt.port;
	json["ns"] = config.ntp.server;
	json["np"] = config.ntp.port;
		

	File configFile = SPIFFS.open(configFileName, "w");
	if (configFile) {
		serializeJson(json, configFile);
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

		StaticJsonDocument<JSON_BUFFER_SIZE> jsonBuffer;
		deserializeJson(jsonBuffer, configFile);
		JsonObject json = jsonBuffer.as<JsonObject>();

		if (json) {
			
			if (json.containsKey("cs")) config.credentials.set = json["cs"];
			if (json.containsKey("ss")) strcpy(config.credentials.ssid, json["ss"]);
			if (json.containsKey("pa")) strcpy(config.credentials.pass, json["pa"]);

			if (json.containsKey("ts")) config.token.set = json["ts"];
			if (json.containsKey("to")) strcpy(config.token.token, json["to"]);

			if (json.containsKey("ms")) strcpy(config.mqtt.server, json["ms"]);
			if (json.containsKey("mp")) config.mqtt.port = json["mp"];
			if (json.containsKey("ns")) strcpy(config.ntp.server, json["ms"]);
			if (json.containsKey("np")) config.ntp.port = json["mp"];

			if (json.containsKey("tn")) config.debug_telnet = json["tn"];
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
	WiFi.hostByName(config.ntp.server, ntpServerIP);

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
			sprintf(last_modified, "%s %s GMT", __DATE__, __TIME__);
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

	Udp.beginPacket(address, config.ntp.port);
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

