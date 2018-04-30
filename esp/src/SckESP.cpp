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

// Event managers
WiFiEventHandler stationConnectedHandler;


void SckESP::setup() {

	// LED outputs
	pinMode(pinLED, OUTPUT);
	digitalWrite(pinLED, LOW);

	// SAM communication
	Serial.begin(serialBaudrate);
	Serial.setDebugOutput(false);
	manager.init();

	// Event handlers
	stationConnectedHandler = WiFi.onStationModeConnected(&onStationConnected);

	SPIFFS.begin();

	ledBlink(350); 			// Heartbeat

	// Create hostname
	String macAddr = WiFi.softAPmacAddress();
	macAddr = macAddr.substring(macAddr.length() - 5);
	macAddr.replace(":", "");
	strncpy(hostname, "Smartcitizen", 20);
	strncat(hostname, macAddr.c_str(), 4);

	WiFi.hostname(hostname);
	WiFi.persistent(false);		 		// Only write to flash credentials when changed (for protecting flash from wearing out)

	// WiFi.begin("IAAC-OFFICE-C", "enteroffice2016");
	// startAP();

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

	inputUpdate();

	if (telnetDebug) Debug.handle();
}

void SckESP::inputUpdate() {
	
	if (manager.available()) {
		uint8_t len = NETPACK_TOTAL_SIZE;
		if (manager.recvfromAck(netPack, &len)) {

			debugOUT("Command: " + String((char)netPack[2]));
			
			memcpy(netBuff, &netPack[3], NETPACK_CONTENT_SIZE);

			for (uint8_t i=1; i<netPack[TOTAL_PARTS]; i++) {
				if (manager.recvfromAckTimeout(netPack, &len, 500))	memcpy(&netBuff[(i * NETPACK_CONTENT_SIZE)], &netPack[2], sizeof(netPack)-2);
				else return;
			}
			debugOUT("Content: " + String(netBuff));

			uint8_t pre = String((char)netPack[2]).toInt();
			ESPMessage wichMessage = static_cast<ESPMessage>(pre);
			receiveMessage(wichMessage);
		}
	}
}
bool SckESP::sendMessage(SAMMessage wichMessage, const char *content) {

	sprintf(netBuff, "%u%s", wichMessage, content);

	uint16_t totalSize = strlen(netBuff);
	uint8_t totalParts = (totalSize + NETPACK_CONTENT_SIZE - 1)  / NETPACK_CONTENT_SIZE;
	netPack[TOTAL_PARTS] = totalParts;

	for (uint8_t i=0; i<totalParts; i++) {
		netPack[PART_NUMBER] = i;
		netPack[2] = 0;				// Clear previous contents
		memcpy(&netPack[2], &netBuff[(i * NETPACK_CONTENT_SIZE)], NETPACK_CONTENT_SIZE);
		if (!manager.sendtoWait(netPack, NETPACK_TOTAL_SIZE, SAM_ADDRESS)) return false;
	}

	return true;
}
void SckESP::receiveMessage(ESPMessage wichMessage) {

	switch(wichMessage) {

		case ESPMES_SET_TOKEN: {
	 		strncpy(config.token, netBuff, 8);
	 		if (saveToken()) {
	 			loadToken();
	 			sendToken();
	 		}

	 		break;
	 	} case ESPMES_GET_TOKEN: { sendToken(); break;
	 	} default: break;
	}
}


void SckESP::WifiConnected() {

	debugOUT("Conected to Wifi!");
	ledSet(1);
	// TODO notify sam
}
// 	---------------------
//	|	Input-Output 	|
//	---------------------

// bool SckESP::processMsg() {

// 	// debugOUT(F("Processing command from SAM..."));
// 	// debugOUT(String F("Epoch time: ") + String(msgIn.time));
// 	if (!String(msgIn.com).startsWith("34")) debugOUT(String F("Command from SAM: ") + String(msgIn.com));
// 	// debugOUT(String F("Parameters: ") + String(msgIn.param));

// 	switch(msgIn.com) {
// 		case ESP_SET_WIFI_COM: {
// 			// Parse input
// 			StaticJsonBuffer<240> jsonBuffer;
// 			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);
// 			strcpy(config.ssid, jsonNet["ssid"]);
// 			strcpy(config.pass, jsonNet["pass"]);
// 			if (addNetwork()) {
// 				readNetwork();
// 				sendNetwork(ESP_SET_WIFI_COM);
// 				if (WiFi.status() != WL_CONNECTED) tryConnection();
// 			} 
// 			break;

// 		} case ESP_SET_TOKEN_COM: {

// 	 		strncpy(config.token, msgIn.param, 8);
// 	 		espStatus.token = ESP_TOKEN_OK;
// 	 		if (saveToken()) {
// 	 			loadToken();
// 	 			sendToken();
// 	 		}

// 	 		break;

// 	 	} case ESP_GET_WIFI_COM: {
// 			debugOUT(F("Cheking for saved networks..."));
// 			sendNetwork(ESP_GET_WIFI_COM);
// 			break;

// 		} case ESP_CLEAR_WIFI_COM: {
// 			debugOUT(F("Clearing network configuration..."));
// 			clearNetworks();

// 			// ACK
// 			msgOut.com = ESP_CLEAR_WIFI_COM;
// 			clearParam();
// 			SAMsendMsg();

// 			break;

// 		} case ESP_GET_NET_INFO_COM: {

// 			msgOut.com = ESP_GET_NET_INFO_COM;
			
// 			String tip = WiFi.localIP().toString();
// 			String tmac = WiFi.softAPmacAddress();

// 			StaticJsonBuffer<240> jsonBuffer;
// 			JsonObject& jsonIP = jsonBuffer.createObject();
// 			jsonIP["hn"] = hostname;
// 			jsonIP["ip"] = tip;
// 			jsonIP["mac"] = tmac;
// 			// clearParam();
// 			jsonIP.printTo(msgOut.param, 240);
// 			SAMsendMsg();
// 			break;

// 		} case ESP_WIFI_CONNECT_COM: {

// 	 		break;

// 	 	} case ESP_GET_TOKEN_COM: {
// 			msgOut.com = ESP_GET_TOKEN_COM;
// 			clearParam();
// 			strncpy(msgOut.param, config.token, 8);
// 			SAMsendMsg();
// 	 		break;

// 	 	} case ESP_CLEAR_TOKEN_COM: {

// 	 		// ACK
// 			msgOut.com = ESP_CLEAR_TOKEN_COM;
// 			clearParam();
// 			SAMsendMsg();

// 			clearToken();
// 			break;

// 	 	} case ESP_GET_CONF_COM: {

// 	 		msgOut.com = ESP_GET_CONF_COM;
	 		
//  			StaticJsonBuffer<240> jsonBuffer;
// 			JsonObject& jsonConf = jsonBuffer.createObject();
// 			jsonConf["mo"] = config.persistentMode;
// 			jsonConf["ri"] = config.publishInterval;
// 			jsonConf["ss"] = config.ssid;
// 			jsonConf["pa"] = config.pass;
// 			jsonConf["to"] = config.token;

// 			clearParam();
// 			jsonConf.printTo(msgOut.param, 240);
// 			SAMsendMsg();
// 			espStatus.conf = ESP_CONF_NOT_CHANGED_EVENT;
// 			break;

// 	 	} case ESP_START_AP_COM: {

// 			// ACK
// 			msgOut.com = ESP_START_AP_COM;
// 			clearParam();
// 			SAMsendMsg();

// 	 		startAP();
// 	 		break;

// 	 	} case ESP_STOP_AP_COM: {
// 	 		stopAP();
// 	 		break;

// 	 	} case ESP_START_WEB_COM: {
// 	 		startWebServer();
// 	 		break;

// 	 	} case ESP_STOP_WEB_COM: {
// 	 		stopWebserver();
// 	 		break;

// 	 	} case ESP_DEEP_SLEEP_COM: {

// 	 		ESP.deepSleep(0);			// Microseconds
// 	 		break;

// 	 	} case ESP_GET_APCOUNT_COM: {
	 		
// 	 		scanAP();
// 			String sn = String(netNumber);
// 			clearParam();
// 			sn.toCharArray(msgOut.param, 240);
// 			msgOut.com = ESP_GET_APCOUNT_COM;
// 	 		SAMsendMsg();
// 	 		break;

// 	 	} case ESP_GET_APLIST_COM: {
// 	 		msgOut.com = ESP_GET_APLIST_COM;
	 		
// 	 		scanAP();

// 			for (int i=0; i<netNumber; i++) {
// 				StaticJsonBuffer<240> jsonBuffer;
// 				JsonObject& jsonNet = jsonBuffer.createObject();
// 				jsonNet["n"] = i;
// 				jsonNet["s"] = WiFi.SSID(i);
// 				jsonNet["r"] = WiFi.RSSI(i);
// 				clearParam();
// 				jsonNet.printTo(msgOut.param, 240);
// 				SAMsendMsg();
// 			}
// 	 		break;

// 	 	} case ESP_GET_TIME_COM: {

// 			// Update time
// 			if (espStatus.time != ESP_TIME_UPDATED_EVENT) {
// 				debugOUT(F("Trying NTP Sync..."));
// 				Udp.begin(8888);
// 				setSyncProvider(ntpProvider);
// 				setSyncInterval(300);
// 			}

// 			// Send time
// 			debugOUT(F("Sending time to SAM..."));
// 			String epochSTR = String(now());
// 			clearParam();
// 			epochSTR.toCharArray(msgOut.param, 240);
// 			msgOut.com = ESP_GET_TIME_COM;
// 			SAMsendMsg();
// 	 		break;

// 		} case ESP_SET_TIME_COM: {

// 			String tepoch = msgIn.param;
// 			uint32_t iepoch = tepoch.toInt();
// 			const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

// 			if (iepoch >= DEFAULT_TIME) { 

// 				setTime(iepoch);
//        			espStatus.time = ESP_TIME_UPDATED_EVENT;
//        			debugOUT(F("Time updated from SAM!!!"));

//        			msgOut.com = ESP_SET_TIME_COM;
//        			SAMsendMsg();
// 			}
// 			break;


// 		} case ESP_SYNC_HTTP_TIME_COM: {

// 			debugOUT("received request for sync HTTP time...");

// 			// ACK response
// 			msgOut.com = ESP_SYNC_HTTP_TIME_COM;
// 			clearParam();
// 			SAMsendMsg();
// 			getHttpTime();
// 			break;

// 	 	} case ESP_MQTT_HELLOW_COM: {

// 	 		// ACK response
// 			msgOut.com = ESP_MQTT_HELLOW_COM;
// 			clearParam();
// 			SAMsendMsg();

// 	 		if (mqttHellow()) espStatus.mqtt = ESP_MQTT_HELLO_OK_EVENT;
// 	 		else espStatus.mqtt = ESP_MQTT_ERROR_EVENT;
// 			sendStatus();

// 	 		break;

// 	 	} case ESP_MQTT_PUBLISH_COM: {

// 	 		debugOUT("Receiving new readings...");

// 	 		// ACK response
// 			msgOut.com = ESP_MQTT_PUBLISH_COM;
// 			clearParam();
// 			SAMsendMsg();

// 			// Set MQTT status to null
// 			espStatus.mqtt = ESP_NULL_EVENT;
			
// 			// Parse input
// 			StaticJsonBuffer<240> jsonBuffer;
// 			JsonObject& jsonSensors = jsonBuffer.parseObject(msgIn.param);

// 			// Iterate over all sensors
// 			for (uint8_t i=0; i<SENSOR_COUNT; i++) {

// 				SensorType wichSensor = static_cast<SensorType>(i);

// 				// Check if sensor exists in received readings (we asume that missing sensors are disabled)
// 				if (jsonSensors.containsKey(String(i))) {

// 					sensors[wichSensor].lastReadingTime = jsonSensors["t"];
// 					sensors[wichSensor].reading = jsonSensors[String(i)];
// 					sensors[wichSensor].enabled = true;

// 				} else {
// 					sensors[wichSensor].enabled = false;
// 				}
// 			}

// 			if (mqttPublish()) espStatus.mqtt = ESP_MQTT_PUBLISH_OK_EVENT;
// 			else espStatus.mqtt = ESP_MQTT_ERROR_EVENT;
// 	 		break;

// 	 	} case ESP_MQTT_SUBSCRIBE_COM: {

// 	 		// ACK response
// 			msgOut.com = ESP_MQTT_SUBSCRIBE_COM;
// 			clearParam();
// 			SAMsendMsg();

// 			mqttConfigSub(true);
// 	 		break;

// 	 	} case ESP_MQTT_UNSUBSCRIBE_COM: {

// 	 		// ACK response
// 			msgOut.com = ESP_MQTT_UNSUBSCRIBE_COM;
// 			clearParam();
// 			SAMsendMsg();

// 			mqttConfigSub(false);
// 	 		break;

// 	 	} case ESP_MQTT_CLEAR_STATUS: {

// 	 		debugOUT(F("Clearing MQTT status!!!"));

// 	 		espStatus.mqtt = ESP_NULL_EVENT;

// 	 		// ACK response
// 			msgOut.com = ESP_MQTT_CLEAR_STATUS;
// 			clearParam();
// 			SAMsendMsg();
// 	 		break;

// 	 	} case ESP_LED_OFF: {

// 			// ACK response
// 			msgOut.com = ESP_LED_OFF;
// 			clearParam();
// 			SAMsendMsg();

// 	 		ledSet(ledLeft, 0);
// 	 		ledSet(ledRight, 0);
// 	 		break;

// 	 	} case ESP_LED_ON: {
// 	 		ledSet(ledLeft, 1);
// 	 		ledSet(ledRight, 1);
// 	 		break;

// 	 	} case ESP_GET_STATUS_COM: {
// 	 		// debugOUT("~");
// 	 		sendStatus();
// 	 		break;

// 	 	} case ESP_SERIAL_DEBUG_TOGGLE: {
// 	 		if (serialDebug) {
// 	 			debugOUT(F("Turning debug output OFF"));
// 	 			serialDebug = false;
// 	 		} else {
// 	 			serialDebug = true;
// 	 			debugOUT(F("Turning debug output ON"));
// 	 		} 
// 	 		break;

// 	 	} case ESP_GET_FREE_HEAP_COM: {

// 	 		int f = ESP.getFreeHeap();
// 			String free = String(f);
// 			clearParam();
// 			free.toCharArray(msgOut.param, 240);
// 			msgOut.com = ESP_GET_FREE_HEAP_COM;
// 	 		SAMsendMsg();
// 	 		break;

// 		} case ESP_GET_VERSION_COM: {
// 			msgOut.com = ESP_GET_VERSION_COM;

// 			StaticJsonBuffer<240> jsonBuffer;
// 			JsonObject& jsonVer = jsonBuffer.createObject();
// 			jsonVer["ver"] 	= ESPversion;
// 			jsonVer["date"]	= ESPbuildDate;
// 			clearParam();
// 			jsonVer.printTo(msgOut.param, 240);
// 			SAMsendMsg();
// 			break;
	 	
// 	 	} case ESP_CONSOLE_COM: {

// 			consoleBuffer += String(msgIn.param);
// 	 		break;

// 	 	} case ESP_CONSOLE_PUBLISH: {

// 	 		debugOUT("Publishing response to web: " + String(consoleBuffer.length()) + " chars");

// 	 		// Ack
// 	 		msgOut.com = ESP_CONSOLE_PUBLISH;
// 			clearParam();
// 			SAMsendMsg();

// 	 		webServer.send(200, "text/plain", consoleBuffer);
// 	 		consoleBuffer = "";
// 	 		break;
// 	 	}
// 	}
// 	// Clear msg
// 	msgIn.com = 0;
// 	strncpy(msgIn.param, "", 240);
// }
void SckESP::debugOUT(String strOut) {

	if (telnetDebug) DEBUG(strOut.c_str());
	
	if (serialDebug) { 
		// msgOut.com = ESP_DEBUG_EVENT;
		// strOut.toCharArray(msgOut.param, 240);
		// SAMsendMsg();
		Serial.println(strOut);
	if (telnetDebug) {
		strOut += "\r\n";
		DEBUG(strOut.c_str());
	}
	
	if (serialDebug) Serial.println(strOut);
}
// void SckESP::clearParam() {

// 	memset(msgOut.param, 0, sizeof(msgOut.param));
// }


// APmode and WebServer
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
// void SckESP::startWebServer() {

// 	// Android captive portal.
// 	webServer.on("/generate_204", [&](){
// 		flashReadFile("/");
// 	});

// 	// Microsoft captive portal.
// 	webServer.on("/fwlink", [&](){
// 		flashReadFile("/");
// 	});

// 	// Handle files from flash
// 	webServer.onNotFound([&](){
// 		flashReadFile(webServer.uri());
// 	});

// 	// Handle set 
// 	// /set?ssid=value1&password=value2&token=value3&epoch=value&pubint=60&mode=value
// 	//	ssid
// 	//	password
// 	//	token
// 	//	epoch
// 	//	pubint
// 	//	mode
// 	webServer.on("/set", extSet);

// 	// Console
// 	// /console?com=get sensors
// 	webServer.on("/console", [&](){

// 		debugOUT(F("Received web console request."));
		
// 		clearParam();
// 		strncpy(msgOut.param, webServer.arg(0).c_str(), 240); 
// 		msgOut.com = ESP_CONSOLE_COM;
// 		SAMsendMsg();

// 	});

// 	// Handle status request
// 	webServer.on("/status", extStatus);

// 	// Handle ping request
// 	webServer.on("/ping", [&](){

// 		debugOUT(F("Received web ping request."));

// 		webServer.send(200, "text/plain", "");
// 	});

// 	// Handle APlist request
// 	webServer.on("/aplist", [&](){

// 		debugOUT(F("Received web Access point request."));

//    		String json = "{\"nets\":[";

//    		// int netNum = WiFi.scanNetworks();
// 		for (int i=0; i<netNumber; i++) {
// 			json += "{\"ssid\":\"" + String(WiFi.SSID(i));
// 			json += "\",\"ch\":" + String(WiFi.channel(i));
// 			json += ",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
// 			if (i < (netNumber - 1)) json += ",";
// 		}

// 		json += "]}";
// 		webServer.send(200, "text/json", json);
// 		json = String();
// 	});

// 	// Handle SSDP
// 	webServer.on("/description.xml", HTTP_GET, [](){
//     	SSDP.schema(webServer.client());
// 	});

// 	webServer.begin();
// 	espStatus.web = ESP_WEB_ON_EVENT;
// 	debugOUT("Started Webserver!!!");

// 	// SSDP description
// 	SSDP.setSchemaURL("description.xml");
//     SSDP.setHTTPPort(80);
//     SSDP.setName("SmartCitizen Kit");
//     SSDP.setModelName("1.5");
//     SSDP.setModelURL("http://www.smartcitizen.me");
// 	SSDP.begin();
// }
// bool SckESP::flashReadFile(String path){

// 	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
//  	   return false;
// 	}
	
// 	debugOUT("Received file request: " + path);
	
// 	// send index file in case no file is requested
// 	if (path.endsWith("/")) path += "index.gz";

// 	// Manage content types
// 	String contentType = "text/html";

// 	if(path.endsWith(".css")) contentType = "text/css";
// 	else if(path.endsWith(".js")) contentType = "application/javascript";
// 	else if(path.endsWith(".png")) contentType = "image/png";
// 	else if(path.endsWith(".gif")) contentType = "image/gif";
// 	else if(path.endsWith(".jpg")) contentType = "image/jpeg";
// 	else if(path.endsWith(".ico")) contentType = "image/x-icon";

// 	if (SPIFFS.exists(path)) {
	  
// 		File file = SPIFFS.open(path, "r");
// 		size_t sent = webServer.streamFile(file, contentType);
// 		file.close();

// 		return true;
// 	}
// 	webServer.send(404, "text/plain", "FileNotFound");
// 	return false;
// }


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