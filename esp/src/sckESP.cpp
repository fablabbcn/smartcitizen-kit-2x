#include "sckESP.h"

// Structs for SAM <<>> ESP communication
EasyTransfer BUS_in, BUS_out;

// Wifi client
WiFiClient wclient;

// MQTT client
PubSubClient MQTTclient(wclient);

// Web Server
ESP8266WebServer webServer(80);

// DNS for captive portal
DNSServer dnsServer;

// 	-----------------
//	|	General 	|
//	-----------------
//
void SckESP::setup() {

	// LED outputs
	pinMode(LED_LEFT, OUTPUT);
	pinMode(LED_RIGHT, OUTPUT);
	digitalWrite(LED_LEFT, HIGH);
	digitalWrite(LED_RIGHT, HIGH);

	// Filesystem (Still recognized less size than real)
	SPIFFS.begin();

	// SAM <<>> ESP comunication
	BUS_in.begin(details(msgIn), &Serial);
	BUS_out.begin(details(msgOut), &Serial);

	Serial.begin(115200);
	Serial.setDebugOutput(false);

	ledBlink(ledRight, 350); 			// Heartbeat

	// Create hostname
	String macAddr = WiFi.softAPmacAddress();
	macAddr = macAddr.substring(macAddr.length() - 5);
	macAddr.replace(":", "");

	strncpy(hostname, "Smartcitizen", 20);
	strncat(hostname, macAddr.c_str(), 4);

	// Starts ESP status values in NULL
	for (uint8_t i=0; i<ESP_STATUS_TYPES_COUNT; i++) espStatus.value[i] = ESP_NULL_EVENT;

	// Wifi settings
	WiFi.hostname(hostname);
	WiFi.persistent(false);		 		// Only write to flash credentials when changed (for protecting flash from wearing out)
	readNetwork();
	loadToken();

	scanAP();

	if (countSavedNetworks() <= 0) {
		espStatus.wifi = ESP_WIFI_NOT_CONFIGURED;
		ledBlink(ledRight, 100);
		startAP();
	} else {
		tryConnection();
	}
}
void SckESP::update() {

	if (BUS_in.receiveData()) processMsg();

	actualWIFIStatus = WiFi.status();

	if (espStatus.web == ESP_WEB_ON_EVENT)	{
		dnsServer.processNextRequest();
		webServer.handleClient();
	}
	
	// Only take actions when status has changed
	if (actualWIFIStatus != prevWIFIStatus) {

		// WL_CONNECTED after successful connection is established
		if (actualWIFIStatus == WL_CONNECTED) {
			debugOUT(String F("Conected to wifi: ") + String(config.ssid) + " - " + String(config.pass));
			espStatus.wifi = ESP_WIFI_CONNECTED_EVENT;
			ledSet(ledRight, 1);
			// setGoodNet();
			stopAP();

			startWebServer();	// CHECK if it doesnt mess with NTP

			// mDNS
			if (!MDNS.begin(hostname)) {
				debugOUT(F("ERROR: mDNS service failed to start!!"));
			} else {
				debugOUT(F("mDNS service started!!"));
				MDNS.addService("http", "tcp", 80);
			}


		// WL_DISCONNECTED if module is not configured in station mode
		} else if (actualWIFIStatus == WL_DISCONNECTED) {

			// tryConnection();
		
		// (NOT) WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
		} else if (actualWIFIStatus != WL_IDLE_STATUS) {
			
			debugOUT(String F("FAILED Wifi conection: ") + String(config.ssid) + " - " + String(config.pass));
			espStatus.wifi == ESP_WIFI_ERROR_EVENT;
			startAP();

			// WL_NO_SSID_AVAILin case configured SSID cannot be reached
			if (actualWIFIStatus == WL_NO_SSID_AVAIL) {
				
				debugOUT(String F("Can't find ssid!!"));
				espStatus.wifi = ESP_WIFI_ERROR_AP_EVENT;
			

			// WL_CONNECT_FAILED if password is incorrect
			} else if (actualWIFIStatus == WL_CONNECT_FAILED) {
			
				debugOUT(String F("Wrong password!!"));
				espStatus.wifi = ESP_WIFI_ERROR_PASS_EVENT;
			}

			// If you have configured at least one net AND it appears in the scan 
			if (countSavedNetworks() > 0 && connectionRetrys < 1) {
			
				connectionRetrys++;
				tryConnection();
			
			} else {
			
				// NO network available: Enter Ap mode
				ledBlink(ledRight, 100);
				startAP();
			}
		}
		prevWIFIStatus = WiFi.status();
	}
}


// 	---------------------
//	|	Input-Output 	|
//	---------------------
//
bool SckESP::processMsg() {

	// debugOUT(F("Processing command from SAM..."));
	// debugOUT(String F("Epoch time: ") + String(msgIn.time));
	if (!String(msgIn.com).startsWith("34")) debugOUT(String F("Command from SAM: ") + String(msgIn.com));
	// debugOUT(String F("Parameters: ") + String(msgIn.param));

	switch(msgIn.com) {
		case ESP_SET_WIFI_COM: {
			// Parse input
			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);
			strcpy(config.ssid, jsonNet["ssid"]);
			strcpy(config.pass, jsonNet["pass"]);
			if (addNetwork()) {
				readNetwork();
				sendNetwork(ESP_SET_WIFI_COM);
				if (WiFi.status() != WL_CONNECTED) tryConnection();
			} 
			break;

		} case ESP_SET_TOKEN_COM: {

	 		strncpy(config.token, msgIn.param, 8);
	 		espStatus.token = ESP_TOKEN_OK;
	 		if (saveToken()) {
	 			loadToken();
	 			sendToken();
	 		}

	 		break;

	 	} case ESP_GET_WIFI_COM: {
			debugOUT(F("Cheking for saved networks..."));
			sendNetwork(ESP_GET_WIFI_COM);
			break;

		} case ESP_CLEAR_WIFI_COM: {
			debugOUT(F("Clearing network configuration..."));
			clearNetworks();

			// ACK
			msgOut.com = ESP_CLEAR_WIFI_COM;
			clearParam();
			SAMsendMsg();

			break;

		} case ESP_GET_NET_INFO_COM: {

			msgOut.com = ESP_GET_NET_INFO_COM;
			
			String tip = WiFi.localIP().toString();
			String tmac = WiFi.softAPmacAddress();

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonIP = jsonBuffer.createObject();
			jsonIP["hn"] = hostname;
			jsonIP["ip"] = tip;
			jsonIP["mac"] = tmac;
			// clearParam();
			jsonIP.printTo(msgOut.param, 240);
			SAMsendMsg();
			break;

		} case ESP_WIFI_CONNECT_COM: {

	 		break;

	 	} case ESP_GET_TOKEN_COM: {
			msgOut.com = ESP_GET_TOKEN_COM;
			clearParam();
			strncpy(msgOut.param, config.token, 8);
			SAMsendMsg();
	 		break;

	 	} case ESP_CLEAR_TOKEN_COM: {

	 		// ACK
			msgOut.com = ESP_CLEAR_TOKEN_COM;
			clearParam();
			SAMsendMsg();

			clearToken();
			break;

	 	} case ESP_GET_CONF_COM: {

	 		msgOut.com = ESP_GET_CONF_COM;
	 		
 			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonConf = jsonBuffer.createObject();
			jsonConf["mo"] = config.persistentMode;
			jsonConf["ri"] = config.publishInterval;
			jsonConf["ss"] = config.ssid;
			jsonConf["pa"] = config.pass;
			jsonConf["to"] = config.token;

			clearParam();
			jsonConf.printTo(msgOut.param, 240);
			SAMsendMsg();
			espStatus.conf = ESP_CONF_NOT_CHANGED_EVENT;
			break;

	 	} case ESP_START_AP_COM: {

			// ACK
			msgOut.com = ESP_START_AP_COM;
			clearParam();
			SAMsendMsg();

	 		startAP();
	 		break;

	 	} case ESP_STOP_AP_COM: {
	 		stopAP();
	 		break;

	 	} case ESP_START_WEB_COM: {
	 		startWebServer();
	 		break;

	 	} case ESP_STOP_WEB_COM: {
	 		stopWebserver();
	 		break;

	 	} case ESP_DEEP_SLEEP_COM: {

	 		ESP.deepSleep(0);			// Microseconds
	 		break;

	 	} case ESP_GET_APCOUNT_COM: {
	 		
	 		scanAP();
			String sn = String(netNumber);
			clearParam();
			sn.toCharArray(msgOut.param, 240);
			msgOut.com = ESP_GET_APCOUNT_COM;
	 		SAMsendMsg();
	 		break;

	 	} case ESP_GET_APLIST_COM: {
	 		msgOut.com = ESP_GET_APLIST_COM;
	 		
	 		scanAP();

			for (int i=0; i<netNumber; i++) {
				StaticJsonBuffer<240> jsonBuffer;
				JsonObject& jsonNet = jsonBuffer.createObject();
				jsonNet["n"] = i;
				jsonNet["s"] = WiFi.SSID(i);
				jsonNet["r"] = WiFi.RSSI(i);
				clearParam();
				jsonNet.printTo(msgOut.param, 240);
				SAMsendMsg();
			}
	 		break;

	 	} case ESP_GET_TIME_COM: {

			// Update time
			if (espStatus.time != ESP_TIME_UPDATED_EVENT) {
				debugOUT(F("Trying NTP Sync..."));
				Udp.begin(8888);
				setSyncProvider(ntpProvider);
				setSyncInterval(300);
			}

			// Send time
			debugOUT(F("Sending time to SAM..."));
			String epochSTR = String(now());
			clearParam();
			epochSTR.toCharArray(msgOut.param, 240);
			msgOut.com = ESP_GET_TIME_COM;
			SAMsendMsg();
	 		break;

		} case ESP_SET_TIME_COM: {

			String tepoch = msgIn.param;
			uint32_t iepoch = tepoch.toInt();
			const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

			if (iepoch >= DEFAULT_TIME) { 

				setTime(iepoch);
       			espStatus.time = ESP_TIME_UPDATED_EVENT;
       			debugOUT(F("Time updated from SAM!!!"));

       			msgOut.com = ESP_SET_TIME_COM;
       			SAMsendMsg();
			}
			break;


		} case ESP_SYNC_HTTP_TIME_COM: {

			debugOUT("received request for sync HTTP time...");

			// ACK response
			msgOut.com = ESP_SYNC_HTTP_TIME_COM;
			clearParam();
			SAMsendMsg();
			getHttpTime();
			break;

	 	} case ESP_MQTT_HELLOW_COM: {

	 		// ACK response
			msgOut.com = ESP_MQTT_HELLOW_COM;
			clearParam();
			SAMsendMsg();

	 		if (mqttHellow()) espStatus.mqtt = ESP_MQTT_HELLO_OK_EVENT;
	 		else espStatus.mqtt = ESP_MQTT_ERROR_EVENT;
			sendStatus();

	 		break;

	 	} case ESP_MQTT_PUBLISH_COM: {

	 		debugOUT("Receiving new readings...");

	 		// ACK response
			msgOut.com = ESP_MQTT_PUBLISH_COM;
			clearParam();
			SAMsendMsg();

			// Set MQTT status to null
			espStatus.mqtt = ESP_NULL_EVENT;
			
			// Parse input
			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonSensors = jsonBuffer.parseObject(msgIn.param);

			// Iterate over all sensors
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {

				SensorType wichSensor = static_cast<SensorType>(i);

				// Check if sensor exists in received readings (we asume that missing sensors are disabled)
				if (jsonSensors.containsKey(String(i))) {

					sensors[wichSensor].lastReadingTime = jsonSensors["t"];
					sensors[wichSensor].reading = jsonSensors[String(i)];
					sensors[wichSensor].enabled = true;

				} else {
					sensors[wichSensor].enabled = false;
				}
			}

			if (mqttPublish()) espStatus.mqtt = ESP_MQTT_PUBLISH_OK_EVENT;
			else espStatus.mqtt = ESP_MQTT_ERROR_EVENT;
	 		break;

	 	} case ESP_MQTT_SUBSCRIBE_COM: {

	 		// ACK response
			msgOut.com = ESP_MQTT_SUBSCRIBE_COM;
			clearParam();
			SAMsendMsg();

			mqttConfigSub(true);
	 		break;

	 	} case ESP_MQTT_UNSUBSCRIBE_COM: {

	 		// ACK response
			msgOut.com = ESP_MQTT_UNSUBSCRIBE_COM;
			clearParam();
			SAMsendMsg();

			mqttConfigSub(false);
	 		break;

	 	} case ESP_MQTT_CLEAR_STATUS: {

	 		debugOUT(F("Clearing MQTT status!!!"));

	 		espStatus.mqtt = ESP_NULL_EVENT;

	 		// ACK response
			msgOut.com = ESP_MQTT_CLEAR_STATUS;
			clearParam();
			SAMsendMsg();
	 		break;

	 	} case ESP_LED_OFF: {

			// ACK response
			msgOut.com = ESP_LED_OFF;
			clearParam();
			SAMsendMsg();

	 		ledSet(ledLeft, 0);
	 		ledSet(ledRight, 0);
	 		break;

	 	} case ESP_LED_ON: {
	 		ledSet(ledLeft, 1);
	 		ledSet(ledRight, 1);
	 		break;

	 	} case ESP_GET_STATUS_COM: {
	 		// debugOUT("~");
	 		sendStatus();
	 		break;

	 	} case ESP_SERIAL_DEBUG_TOGGLE: {
	 		if (serialDebug) {
	 			debugOUT(F("Turning debug output OFF"));
	 			serialDebug = false;
	 		} else {
	 			serialDebug = true;
	 			debugOUT(F("Turning debug output ON"));
	 		} 
	 		break;

	 	} case ESP_GET_FREE_HEAP_COM: {

	 		int f = ESP.getFreeHeap();
			String free = String(f);
			clearParam();
			free.toCharArray(msgOut.param, 240);
			msgOut.com = ESP_GET_FREE_HEAP_COM;
	 		SAMsendMsg();
	 		break;

		} case ESP_GET_VERSION_COM: {
			msgOut.com = ESP_GET_VERSION_COM;

			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonVer = jsonBuffer.createObject();
			jsonVer["ver"] 	= ESPversion;
			jsonVer["date"]	= ESPbuildDate;
			clearParam();
			jsonVer.printTo(msgOut.param, 240);
			SAMsendMsg();
			break;
	 	
	 	} case ESP_CONSOLE_COM: {

			consoleBuffer += String(msgIn.param);
	 		break;

	 	} case ESP_CONSOLE_PUBLISH: {

	 		debugOUT("Publishing response to web: " + String(consoleBuffer.length()) + " chars");

	 		// Ack
	 		msgOut.com = ESP_CONSOLE_PUBLISH;
			clearParam();
			SAMsendMsg();

	 		webServer.send(200, "text/plain", consoleBuffer);
	 		consoleBuffer = "";
	 		break;
	 	}
	}
	// Clear msg
	msgIn.com = 0;
	strncpy(msgIn.param, "", 240);
}
void SckESP::debugOUT(String strOut) {

	if (serialDebug) { 
		msgOut.com = ESP_DEBUG_EVENT;
		strOut.toCharArray(msgOut.param, 240);
		SAMsendMsg();
	}
}
void SckESP::SAMsendMsg() {

	if (sizeof(msgOut.param) > 240) {
		// TODO handle multiple packages in msgout
		debugOUT(F("WARNING: Serial package is too big!!!"));
	}

	BUS_out.sendData();
}
void SckESP::sendStatus() {

	clearParam();

	espStatusTypes statusToSend = ESP_STATUS_TYPES_COUNT;

	for (uint8_t i=0; i<ESP_STATUS_TYPES_COUNT; i++) {

		statusToSend = static_cast<espStatusTypes>(i);
		msgOut.param[i] = espStatus.value[statusToSend];

	}

	msgOut.com = ESP_GET_STATUS_COM;
	SAMsendMsg();
}
void SckESP::clearParam() {

	memset(msgOut.param, 0, sizeof(msgOut.param));
}
void SckESP::sendNetwork(EspCommand comm) {

	// Prepare json for sending
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["s"] = config.ssid;
	jsonNet["p"] = config.pass;
	msgOut.com = comm;
	clearParam();
	jsonNet.printTo(msgOut.param, 240);
	SAMsendMsg();
}

// 	---------------------
//	|	Configuration 	|
//	---------------------
//
bool SckESP::saveConf() {
}
bool SckESP::loadConf() {
}


// 	-------------
//	|	WiFi 	|
//	-------------
//
void SckESP::tryConnection() {

	if (WiFi.status() != WL_CONNECTED) {

		debugOUT(String F("Trying connection to wifi: ") + String(config.ssid) + " - " + String(config.pass));

		String tp = String(config.pass);

		if (tp.length() == 0) WiFi.begin(config.ssid);
		else WiFi.begin(config.ssid, config.pass);

	} else {
		debugOUT(String F("Already connected to wifi: ") + String(WiFi.SSID()));
	}
}

// 	-----------------------------
//	|	APmode and WebServer 	|
//	-----------------------------
//
void SckESP::startAP(){

	debugOUT(String F("Starting Ap with ssid: ") + String(hostname));

	// IP for DNS
	IPAddress myIP(192, 168, 1, 1);

	// Start Soft AP
	WiFi.mode(WIFI_AP);
	WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP((const char *)hostname);
	delay(500);
	espStatus.ap = ESP_AP_ON_EVENT;

	// DNS stuff (captive portal)
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", myIP);

	delay(100);
	startWebServer();
}
void SckESP::stopAP() {

	dnsServer.stop();
	WiFi.softAPdisconnect(true);
	espStatus.ap = ESP_AP_OFF_EVENT;
}
void SckESP::startWebServer() {

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
	webServer.on("/console", [&](){

		debugOUT(F("Received web console request."));
		
		clearParam();
		strncpy(msgOut.param, webServer.arg(0).c_str(), 240); 
		msgOut.com = ESP_CONSOLE_COM;
		SAMsendMsg();

	});

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
	espStatus.web = ESP_WEB_ON_EVENT;
	debugOUT("Started Webserver!!!");

	// SSDP description
	SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName("SmartCitizen Kit");
    SSDP.setModelName("1.5");
    SSDP.setModelURL("http://www.smartcitizen.me");
	SSDP.begin();
	
}
void SckESP::stopWebserver() {

	espStatus.web = ESP_WEB_OFF_EVENT;
	webServer.stop();
}
void SckESP::webSet() {

	debugOUT(F("Received web configuration request."));

	String json = "{";

	// If we found ssid AND pass
	if (webServer.hasArg("ssid"))  {

		String tssid = webServer.arg("ssid");
		String tpass = "";

		if (webServer.hasArg("password")) {
			tpass = webServer.arg("password");
		}
			
		// If ssid is no zero chars
		if (tssid.length() > 0) {

			tssid.toCharArray(config.ssid, 64);
			tpass.toCharArray(config.pass, 64);

			if (addNetwork()) {
				espStatus.conf = ESP_CONF_CHANGED_EVENT;
				json += "\"ssid\":\"true\",";
				debugOUT(F("Wifi credentials updated from apmode web!!!"));
			}
		} else {
			debugOUT(F("Invalid Wifi credentials received from apmode web!!!"));
			json += "\"ssid\":\"false\",";
		}
	} else {
		json += "\"ssid\":\"false\",";
	}

	// If we found the mode
	if (webServer.hasArg("mode")) {
		String stringMode = webServer.arg("mode");
		if (stringMode.equals("sdcard")) {

			config.persistentMode = MODE_SD;
			espStatus.conf = ESP_CONF_CHANGED_EVENT;
			json += "\"mode\":\"true\",";
			debugOUT(F("Mode set to sdcard from apmode web!!!"));

		} else if (stringMode.equals("network")) {

			config.persistentMode = MODE_NET;
			espStatus.conf = ESP_CONF_CHANGED_EVENT;
			json += "\"mode\":\"true\",";
			debugOUT(F("Mode set to network from apmode web!!!"));

		} else {

			json += "\"mode\":\"false\",";
			debugOUT(F("Invalid mode from apmode web!!!"));
		}
		json += "\"mode\":\"false\",";
	}

	// If we found the token
	if (webServer.hasArg("token")) {
		String stringToken = webServer.arg("token");
		if (stringToken.length() == 6) {
			stringToken.toCharArray(config.token, 8);
			saveToken();
			espStatus.conf = ESP_CONF_CHANGED_EVENT;
			json += "\"token\":\"true\",";
			debugOUT(F("Token updated from apmode web!!!"));
		} else {
			debugOUT(F("Invalid Token received from apmode web!!!"));
			json += "\"token\":\"false\",";
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
       		espStatus.conf = ESP_CONF_CHANGED_EVENT;
      		debugOUT(F("Time updated from apmode web!!!"));

      		debugOUT(F("Sending time to SAM..."));
			String epochSTR = String(now());
			clearParam();
			epochSTR.toCharArray(msgOut.param, 240);
			msgOut.com = ESP_GET_TIME_COM;
			SAMsendMsg();

			json += "\"time\":\"true\",";
		} else {
			debugOUT(F("Invalid time received from apmode web!!!"));
			json += "\"time\":\"false\",";
		}

	} else {
		json += "\"time\":\"false\",";
	}

	// Publish interval (seconds)
	if (webServer.hasArg("pubint")) {
		
		String tinterval = webServer.arg("pubint");
		uint32_t intTinterval = tinterval.toInt();

		if (intTinterval < max_publish_interval && intTinterval > minimal_publish_interval) {
			
			espStatus.conf = ESP_CONF_CHANGED_EVENT;
			config.publishInterval = intTinterval;

			debugOUT(F("Publish interval changed from apmode web!!!"));

			json += "\"pubint\":\"true\"";

		} else {
			debugOUT(F("Invalid publish interval from apmode web!!!"));
			json += "\"pubint\":\"false\"";
		}
				
	} else {
		json += "\"pubint\":\"false\"";
	}
	
	json += "}";
	webServer.send(200, "text/json", json);

	if (WiFi.status() != WL_CONNECTED) tryConnection();
}
void SckESP::webStatus() {

	debugOUT(F("Received web status info request."));

	// Token
	String json = "{\"token\":\"" + String(config.token) + "\",";
	
	// Wifi config
	json += "\"ssid\":\"" + String(config.ssid) + "\",";
	json += "\"password\":\"" + String(config.pass) + "\",";

	// Mode
	if (config.persistentMode == MODE_SD) json += "\"mode\":\"sdcard\",";
	else if (config.persistentMode == MODE_NET) json += "\"mode\":\"network\",";

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
	json += "\"apstatus\":\"" + String(espStatus.eventTitle[espStatus.ap]) + "\",";

	// Wifi status
	json += "\"wifi\":\"" + String(espStatus.eventTitle[espStatus.wifi]) + "\",";

	// Net status
	json += "\"net\":\"" + String(espStatus.eventTitle[espStatus.net]) + "\",";

	// MQTT status
	json += "\"mqtt\":\"" + String(espStatus.eventTitle[espStatus.mqtt]) + "\",";

	// Last publish time
	json += "\"last_publish\":\"" + epochSTR + "\"";


	json += "}";
	webServer.send(200, "text/json", json);
	json = String();
}
bool SckESP::flashReadFile(String path){

	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
 	   return false;
	}
	
	debugOUT("Received file request: " + path);
	
	// send index file in case no file is requested
	if (path.endsWith("/")) path += "index.html";

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
		size_t sent = webServer.streamFile(file, contentType);
		file.close();

		return true;
	}
	webServer.send(404, "text/plain", "FileNotFound");
	return false;
}
bool SckESP::captivePortal() {
  if (!isIp(webServer.hostHeader()) && webServer.hostHeader() != (String(hostname)+".local")) {
    webServer.sendHeader("Location", String("http://") + toStringIp(webServer.client().localIP()), true);
    webServer.send (302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    webServer.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}
bool SckESP::isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) return false;
  }
  return true;
}
String SckESP::toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}


// 	---------------------
//	|	Credentials 	|
//	---------------------
//
bool SckESP::addNetwork() {

	// Prepare json for saving
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["ssid"] = config.ssid;
	jsonNet["pass"] = config.pass;

	File credFile = SPIFFS.open(CREDENTIALS_FILE, "w");
	
	if (credFile) {
		jsonNet.printTo(credFile);
		credFile.write('\n');
		credFile.close();
		debugOUT(String F("Saved network: ") + String(config.ssid) + " - " + String(config.pass));
		return true;
	}
	debugOUT(F("Error saving network!!!"));
	return false;
}
void SckESP::clearNetworks() {

	if (SPIFFS.exists(CREDENTIALS_FILE)) {
		SPIFFS.remove(CREDENTIALS_FILE);
		msgOut.com = ESP_CLEAR_WIFI_COM;
		SAMsendMsg();
	}

	espStatus.wifi = ESP_WIFI_NOT_CONFIGURED;
}
bool SckESP::readNetwork() {

	// If no index specified, load last network
	// if (index < 0) index = (countSavedNetworks() - 1);
	
	if (SPIFFS.exists(CREDENTIALS_FILE)) {
		File credFile = SPIFFS.open(CREDENTIALS_FILE, "r");

		// Go to line Index
		// int countedLines = 0;
		// while (countedLines < index) {
		// 	if (credFile.find(10)) countedLines++;
		// }

		// Read line
		String jbuffer;
		while (credFile.available()) {
			char buff = credFile.read();
			jbuffer += buff;
			if (buff == 10) break;
		}

		credFile.close();

		// Parse json credentials
		StaticJsonBuffer<240> jsonBuffer;
		JsonObject& jsonNet = jsonBuffer.parseObject(jbuffer);
		strcpy(config.ssid, jsonNet["ssid"]);
		strcpy(config.pass, jsonNet["pass"]);
		debugOUT(String(config.ssid) + F(" - ") + String(config.pass));
		return true;
	}
	
	return false;
}
int SckESP::countSavedNetworks() {

	if (SPIFFS.exists(CREDENTIALS_FILE)) {
		File credFile = SPIFFS.open(CREDENTIALS_FILE, "r");
		if (credFile) {
			int countedLines = 0;
			while (credFile.available()) {
				if (credFile.find(10)) countedLines++;
			}
			credFile.close();
			debugOUT(String F("Counted ") + String(countedLines) + F(" networks saved."));
			return countedLines;
		}
	}
	
	espStatus.wifi = ESP_WIFI_NOT_CONFIGURED;
	return -1;
}
void SckESP::scanAP() {

	debugOUT(F("Scaning Wifi networks..."));

	netNumber = WiFi.scanNetworks();
	// Wait for scan...
	while (WiFi.scanComplete() == -2) {
		;
	}

	debugOUT(String(netNumber) + F(" networks found"));
}


// 	-------------
//	|	Token 	|
//	-------------
//
bool SckESP::saveToken() {

	// Open token flash file
	File tokenFile = SPIFFS.open(TOKEN_FILE, "w");

	// Check if file opened OK
	if (!tokenFile) {
		return false;
		debugOUT(F("Failed to save token!!!"));
	}
		
	// Write the token
	tokenFile.println(config.token);
	tokenFile.close();

	debugOUT(F("Token saved!!!"));
	return true;
}
bool SckESP::loadToken() {

	// Check if file exists
	if (!SPIFFS.exists(TOKEN_FILE)) {
		debugOUT(F("Failed to load token!!!"));
		espStatus.token = ESP_TOKEN_ERROR;
		return false;
	}

	// Open token flash file
	File tokenFile = SPIFFS.open(TOKEN_FILE, "r");

	// Read line
	String buffer;
	while (tokenFile.available()) {
		char buff = tokenFile.read();
		if (buff == 10) break;
		buffer += buff;
	}

	buffer.replace("\r", "");
	buffer.replace("\n", "");

	tokenFile.close();
	buffer.toCharArray(config.token, buffer.length()+1);

	espStatus.token = ESP_TOKEN_OK;

	debugOUT(String F("Loaded token: ") + String(config.token));
	return true;
}
void SckESP::clearToken() {

	if (SPIFFS.exists(TOKEN_FILE)) {
		SPIFFS.remove(TOKEN_FILE);
	}

	espStatus.token = ESP_TOKEN_ERROR;
}
void SckESP::sendToken() {
	strncpy(msgOut.param, config.token, 8);
	msgOut.com = ESP_SET_TOKEN_COM;
	SAMsendMsg();
}


// 	-------------
//	|	MQTT 	|
//	-------------
//
bool SckESP::mqttConnect() {

	if (MQTTclient.connected()) return true;

	debugOUT(F("Connecting to MQTT server..."));

	MQTTclient.setServer(MQTT_SERVER_NAME, 1883);

	if (MQTTclient.connect(config.token)) {
		debugOUT(F("Established MQTT connection..."));
		return true;
	} else {
		debugOUT(F("ERROR: MQTT connection failed!!!"));
		debugOUT(String(MQTTclient.state()));
		espStatus.mqtt = ESP_MQTT_ERROR_EVENT;
		return false;
	}
}
bool SckESP::mqttHellow() {

	debugOUT(F("Trying MQTT Hello..."));

	if (mqttConnect()) {

		// prepare the topic title
   		char myTopic[24];
		sprintf(myTopic, "device/sck/%s/hello", config.token);

   		// Prepare the payload
   		char myPayload[14];
   		sprintf(myPayload, "%s:Hello", config.token);

		if (MQTTclient.publish(myTopic, myPayload)) {
			debugOUT(F("MQTT Hello published OK !!!"));
			return true;
		}
		debugOUT(F("MQTT Hello ERROR !!!"));
	}
	return false;
}
bool SckESP::mqttPublish() {
	
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
    	char myPayload[512];

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
				sprintf(myPayload, "%s{\"id\":%i,\"value\":%s}", myPayload, sensors[wichSensor].id, String(sensors[wichSensor].reading,2).c_str());
			}
    	}
    	sprintf(myPayload, "%s]}]}", myPayload);

    	debugOUT(String(myPayload));

    	// prepare the topic title
	   	char pubTopic[27];
		sprintf(pubTopic, "device/sck/%s/readings", config.token);

		debugOUT(String(pubTopic));

		if (MQTTclient.publish(pubTopic, myPayload)) {
			debugOUT(F("MQTT readings published OK !!!"));
			return true;
		}
	}
	debugOUT(F("MQTT publish ERROR !!!"));
	return false;
}
bool SckESP::mqttConfigSub(bool activate) {



	// if (activate) {

	// 	debugOUT("Subscribing to MQQT config");

	// 	// We need to be disconnected to subscribe...
	// 	if (mqtt.connected()) mqtt.disconnect();
		
	// 	// Create the topic title
	// 	char newTopic[25];
	// 	sprintf(newTopic, "device/sck/%s/config", config.token);

	// 	debugOUT(String("Subscribing to: ") + String(newTopic));

	// 	// Change the topic to the new one and subscribe
	// 	// mqttConfig.topic = newTopic;
	// 	// mqtt.subscribe(&mqttConfig);

	// 	// Turn on subscribed flag
	// 	configSubscribed = true;
	// 	espStatus.mqtt = ESP_MQTT_CONFIG_SUB_EVENT;

	// 	// Reconnect to mqtt broker
	// 	if (mqttConnect()) {

	// 		// Turn on the left led
	// 		ledSet(ledLeft, 1);
	// 	}

	// } else {

	// 	debugOUT("Unsubscribing to MQQT config");

	// 	// unsubscribe
	// 	// mqtt.unsubscribe(&mqttConfig);

	// 	// Turn off subscribed flag
	// 	configSubscribed = false;
	// 	espStatus.mqtt = ESP_MQTT_CONFIG_UNSUB_EVENT;

	// 	// Turn off the left led
	// 	ledSet(ledLeft, 0);
	// }
}

// 	------------
// 	|	Time   |
// 	------------
//
time_t SckESP::getNtpTime() {
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
      
      espStatus.time = ESP_TIME_UPDATED_EVENT;
      debugOUT(F("Time updated!!!"));

      return secsSince1900 - 2208988800UL;
    }
  }
  espStatus.time = ESP_TIME_FAIL_EVENT;
  debugOUT(F("No NTP Response!!!"));
  getHttpTime();
  return 0;
}
void SckESP::sendNTPpacket(IPAddress &address) {
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
String SckESP::ISOtime() {
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
String leadingZeros(String original, int decimalNumber) {
	for (uint8_t i=0; i < (decimalNumber - original.length()); ++i)	{
		original = "0" + original;
	}
	return original;
}
String SckESP::epoch2iso(uint32_t toConvert) {

	time_t tc = toConvert;

	String isoTime = String(year(tc)) + "-" +
	leadingZeros(String(month(tc)), 2) + "-" + 
	leadingZeros(String(day(tc)), 2) + "T" +
	leadingZeros(String(hour(tc)), 2) + ":" + 
	leadingZeros(String(minute(tc)), 2) + ":" + 
	leadingZeros(String(second(tc)), 2) + "Z";
	
	return isoTime;
}
bool SckESP::getHttpTime() {

	debugOUT(F("Trying to get HTTP time..."));

	String UTCtime;

	if (wclient.connect(HTTP_TIME_SERVER_NAME, 80)) {

		debugOUT(F("Connected to smartcitizen time server!!!"));

		wclient.print(String("GET /datetime") + " HTTP/1.1\r\n" +
             "Host: " + HTTP_TIME_SERVER_NAME + "\r\n" +
             "Connection: close\r\n" +
             "\r\n"
            );

		while (wclient.connected()) {
			if (wclient.available()) {
				String line = wclient.readStringUntil('\n');
				if (line.startsWith("UTC:") && line.endsWith("#")) {
					UTCtime = line;

					// Clean String
					line.replace("UTC:", "");
					line.replace("#", ",");

					// Parse data
					// UTC:2017,4,12,15,26,20#
					// UTC:yr,mnth,day,hr,min,sec#
					uint16_t numbers[6];
					for (uint8_t i=0; i<6; i++) {
						uint8_t nextComma = line.indexOf(",");
						String numm = line.substring(0, nextComma);
						line.remove(0, nextComma+1);
						numbers[i] = numm.toInt();
					}

					// setTime(int hr,int min,int sec,int day, int month, int yr);
					setTime(numbers[3], numbers[4], numbers[5], numbers[2], numbers[1], numbers[0]);

					if (year() > 2010) {
						debugOUT(F("Time updated!!!"));
						espStatus.time = ESP_TIME_UPDATED_EVENT;
						return true;
					} else {
						debugOUT(F("Error in HTTP time reception!!!"));
					}
				}
			}
		}
		wclient.stop();

		if (UTCtime.length() > 0) {
			// Proces String
			debugOUT(UTCtime);
		} else {
			debugOUT(F("Failed to get HTTP time!!!"));
		}

	} else {
		debugOUT(F("Failed to connect to Smartcitizen time server!!!"));
	}
	espStatus.time = ESP_TIME_FAIL_EVENT;
	return false;
}


// 	------------
// 	|	Leds   |
// 	------------
//
void SckESP::ledSet(uint8_t wichLed, uint8_t value) {
	if(wichLed == ledLeft) Lblink.detach();
	else Rblink.detach();
	ledValue[wichLed] = abs(value - 1);
	digitalWrite(ledPin[wichLed], ledValue[wichLed]);
}
void SckESP::ledToggle(uint8_t wichLed) {
	ledValue[wichLed] = abs(ledValue[wichLed] - 1);
	digitalWrite(ledPin[wichLed], ledValue[wichLed]);
}
void SckESP::ledBlink(uint8_t wichLed, float rate) {
	if (wichLed == ledLeft) Lblink.attach_ms(rate, LedToggleLeft);
	else if (wichLed == ledRight) Rblink.attach_ms(rate, LedToggleRight);
}
