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

	// Wifi settings
	WiFi.hostname(hostname);
	WiFi.persistent(false);		 		// Only write to flash credentials when changed (for protecting flash from wearing out)
	readNetwork();
	readToken();

	scanAP();

	if (countSavedNetworks() <= 0) {
		espStatus.wifi = ESP_WIFI_ERROR_EVENT;
		ledBlink(ledRight, 100);
		startAP();
	} else {
		tryConnection();
	}
};
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
			debugOUT(String F("Conected to wifi: ") + String(credentials.ssid) + " - " + String(credentials.password));
			espStatus.wifi = ESP_WIFI_CONNECTED_EVENT;
			ledSet(ledRight, 1);
			setGoodNet();
			stopAP();

			// Time setup (it doesn't work with access point mode on)
			// Udp.begin(8888);
			// setSyncProvider(ntpProvider);
			// setSyncInterval(300);

			startWebServer();	// CHECK if it doesnt mess with NTP

		// WL_DISCONNECTED if module is not configured in station mode
		} else if (actualWIFIStatus == WL_DISCONNECTED) {

			tryConnection();
		
		// (NOT) WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
		} else if (actualWIFIStatus != WL_IDLE_STATUS) {
			
			debugOUT(String F("FAILED Wifi conection: ") + String(credentials.ssid) + " - " + String(credentials.password));
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
			// TODO hacer un conteo de retrys por red ASI COMO ESTA SE QUEDA EN UN LOOP
			if (countSavedNetworks() > 1 && selectBestNetwork() >= 0 && connectionRetrys < 1) { 
			
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
};


// 	---------------------
//	|	Input-Output 	|
//	---------------------
//
bool SckESP::processMsg() {

	// debugOUT(F("Processing command from SAM..."));
	// debugOUT(String F("Epoch time: ") + String(msgIn.time));
	// debugOUT(String F("Command: ") + String(msgIn.com));
	// debugOUT(String F("Parameters: ") + String(msgIn.param));

	switch(msgIn.com) {
		case ESP_SET_WIFI_COM: {

			// Parse input
			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonNet = jsonBuffer.parseObject(msgIn.param);
			strcpy(credentials.ssid, jsonNet["ssid"]);
			strcpy(credentials.password, jsonNet["pass"]);

			if (addNetwork()) {
				readNetwork();
				sendNetwork(1, ESP_SET_WIFI_COM);
				if (WiFi.status() != WL_CONNECTED) tryConnection();
			} 
			break;

		} case ESP_GET_WIFI_COM: {
			debugOUT(F("Cheking for saved networks..."));
			SAMlistSavedNetworks();
			break;

		} case ESP_GET_BEST_WIFI_COM: {
			debugOUT(F("Looking for best network..."));
			int8_t bestNetworkIndex = selectBestNetwork();
			msgOut.com = ESP_GET_BEST_WIFI_COM;
			if (bestNetworkIndex >= 0) sendNetwork(bestNetworkIndex);
			else {
				clearParam();
				strncpy(msgOut.param, "None found", 64);
				SAMsendMsg();
			}
			break;

		} case ESP_CLEAR_WIFI_COM: {
			debugOUT(F("Clearing network configuration..."));
			clearNetworks();

			// ACK
			msgOut.com = ESP_CLEAR_WIFI_COM;
			clearParam();
			SAMsendMsg();

			break;

		} case ESP_GET_IP_COM: {

			msgOut.com = ESP_GET_IP_COM;
			String tip = WiFi.localIP().toString();
			debugOUT(String F("IP Address: ") + tip);
			clearParam();
			tip.toCharArray(msgOut.param, 240);
			SAMsendMsg();
			break;

		} case ESP_WIFI_CONNECT_COM: {

	 		break;

	 	} case ESP_GET_TOKEN_COM: {
	 		readToken();
			msgOut.com = ESP_GET_TOKEN_COM;
			clearParam();
			strncpy(msgOut.param, token, 8);
			SAMsendMsg();
	 		break;

	 	} case ESP_SET_TOKEN_COM: {
	 		strncpy(token, msgIn.param, 8);
			saveToken();
			readToken();
			clearParam();
			strncpy(msgOut.param, token, 8);
			msgOut.com = ESP_SET_TOKEN_COM;
			SAMsendMsg();
	 		break;

		} case ESP_CLEAR_TOKEN_COM: {

			strncpy(token, "-null-", 8);
			saveToken();

			// ACK
			msgOut.com = ESP_CLEAR_TOKEN_COM;
			clearParam();
			SAMsendMsg();

			break;

	 	} case ESP_GET_CONF_COM: {
	 		msgOut.com = ESP_GET_CONF_COM;
	 		
 			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonConf = jsonBuffer.createObject();
			jsonConf["ri"] = configuration.readInterval;
			clearParam();
			jsonConf.printTo(msgOut.param, 240);
			SAMsendMsg();
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
			debugOUT(F("Trying NTP Sync..."));
			Udp.begin(8888);
			setSyncProvider(ntpProvider);
			setSyncInterval(300);

			// Send time
			debugOUT(F("Sending time to SAM..."));
			String epochSTR = String(now());
			clearParam();
			epochSTR.toCharArray(msgOut.param, 240);
			msgOut.com = ESP_GET_TIME_COM;
			SAMsendMsg();
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

	 		// Parse input
			StaticJsonBuffer<240> jsonBuffer;
			JsonObject& jsonSensors = jsonBuffer.parseObject(msgIn.param);

			// Iterate over all sensors
			for (uint8_t i=0; i<SENSOR_COUNT; i++) {

				SensorType wichSensor = static_cast<SensorType>(i);

				// Check if sensor exists in SAM readings (we asume that missing sensors are disabled)
				if (jsonSensors.containsKey(String(i))) {

					sensors[wichSensor].reading = jsonSensors[String(i)];
					sensors[wichSensor].enabled = true;

				} else {
					sensors[wichSensor].enabled = false;
				}
			}

			// ACK response
			msgOut.com = ESP_MQTT_PUBLISH_COM;
			clearParam();
			SAMsendMsg();

			if (mqttPublish()) espStatus.mqtt = ESP_MQTT_PUBLISH_OK_EVENT;
			else espStatus.mqtt = ESP_MQTT_ERROR_EVENT;
	 		break;

	 	} case ESP_MQTT_CLEAR_STATUS: {

	 		espStatus.mqtt = ESP_NULL;

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
	 		sendStatus();
	 		break;

	 	} case ESP_SERIAL_DEBUG_ON: {
	 		serialDebug = true;
	 		debugOUT(F("Turning debug output ON"));
	 		break;

	 	} case ESP_SERIAL_DEBUG_OFF: {
	 		debugOUT(F("Turning debug output OFF"));
	 		serialDebug = false;
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
	 	}
	}

	// Clear msg
	msgIn.com = 0;
	strncpy(msgIn.param, "", 240);
};
void SckESP::debugOUT(String strOut) {

	if (serialDebug) { 
		msgOut.com = ESP_DEBUG_EVENT;
		strOut.toCharArray(msgOut.param, 240);
		SAMsendMsg();
	}
};
void SckESP::SAMsendMsg() {

	if (sizeof(msgOut.param) > 240) {
		// TODO handle multiple packages in msgout
		debugOUT(F("WARNING: Serial package is too big!!!"));
	}

	BUS_out.sendData();
};
void SckESP::sendStatus() {

	clearParam();

	msgOut.param[0] = espStatus.wifi;
	msgOut.param[1]	= espStatus.net;
	msgOut.param[2]	= espStatus.mqtt;
	msgOut.param[3]	= espStatus.time;
	msgOut.param[4]	= espStatus.ap;
	msgOut.param[5]	= espStatus.web;
	msgOut.param[6]	= espStatus.conf;

	msgOut.com = ESP_GET_STATUS_COM;
	SAMsendMsg();
};
void SckESP::clearParam() {

	memset(msgOut.param, 0, sizeof(msgOut.param));
};
void SckESP::sendNetwork(uint8_t index, EspCommand comm) {

	// Prepare json for sending
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["n"] = index;
	jsonNet["s"] = credentials.ssid;
	jsonNet["p"] = credentials.password;
	msgOut.com = comm;
	clearParam();
	jsonNet.printTo(msgOut.param, 240);
	SAMsendMsg();
};
void SckESP::SAMlistSavedNetworks() {

	int netCount = countSavedNetworks();

	if (netCount > 0) {

		for (int8_t i=0;  i<netCount; i++) {
			readNetwork(i);
			delay(5);
			sendNetwork(i);
		}
	} else {
		StaticJsonBuffer<240> jsonBuffer;
		JsonObject& jsonNet = jsonBuffer.createObject();
		jsonNet["s"] = "none";
		jsonNet["p"] = "";
		msgOut.com = ESP_GET_WIFI_COM;
		clearParam();
		jsonNet.printTo(msgOut.param, 240);
		SAMsendMsg();
		debugOUT(F("No network configured yet!"));
	}
};


// 	---------------------
//	|	Configuration 	|
//	---------------------
//
bool SckESP::saveConf() {
};
bool SckESP::loadConf() {
};


// 	-------------
//	|	WiFi 	|
//	-------------
//
void SckESP::tryConnection() {

	if (WiFi.status() != WL_CONNECTED) {

		debugOUT(String F("Trying connection to wifi: ") + String(credentials.ssid) + " - " + String(credentials.password));

		String tp = String(credentials.password);

		if (tp.length() == 0) WiFi.begin(credentials.ssid);
		else WiFi.begin(credentials.ssid, credentials.password);

	} else {
		debugOUT(String F("Already connected to wifi: ") + String(WiFi.SSID()));
	}
};
void SckESP::wifiDisconnect(){
};


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
	espStatus.ap = ESP_AP_ON_EVENT;

	// DNS stuff (captive portal)
	dnsServer.start(DNS_PORT, "*", myIP);

	delay(100);
	startWebServer();
};
void SckESP::stopAP() {

	dnsServer.stop();
	WiFi.softAPdisconnect(true);
	espStatus.ap = ESP_AP_OFF_EVENT;

	stopWebserver();
};
void SckESP::startWebServer() {

	// Handle root
	webServer.on("/", HTTP_GET, extRoot);
	
	// TODO a veces no va... parece que es porque esta scaning....

	// For captive portal complex address
	webServer.onNotFound(extRoot);

	webServer.on("/", HTTP_POST, extSet);

	// Handle set 
	// /set?ssid=value1&password=value2&token=value3&epoch=value
	webServer.on("/set", extSet);

	// Handle saved configuration list
	webServer.on("/conf", extShow);

	// Handle APlist request
	// webServer.on("/aplist", [](){

 //   		String json = "{\"nets\":[";

 //   		// int netNum = WiFi.scanNetworks();
 //   		int
	// 	for (int i=0; i<netnumber2; i++) {
	// 		json += "{\"ssid\":\"" + String(WiFi.SSID(i));
	// 		json += "\",\"ch\":" + String(WiFi.channel(i));
	// 		json += ",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
	// 		if (i < (netNumber - 1)) json += ",";
	// 	}

	// 	json += "]}";
	// 	webServer.send(200, "text/json", json);
	// 	json = String();
    	
	// });

	// Handle SSDP
	webServer.on("/description.xml", HTTP_GET, [](){
    	SSDP.schema(webServer.client());
	});

	// mDNS
	if (!MDNS.begin(hostname)) {
		debugOUT(F("ERROR: mDNS service failed to start!!"));
	}

	// The rest
	webServer.onNotFound(extRoot);

	webServer.begin();
	espStatus.web = ESP_WEB_ON_EVENT;

	// SSDP description
	SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName("SmartCitizen Kit");
    SSDP.setModelName("1.5");
    SSDP.setModelURL("http://www.smartcitizen.me");
	SSDP.begin();

	// mDNS
	MDNS.addService("http", "tcp", 80);
};
void SckESP::stopWebserver() {

	espStatus.web = ESP_WEB_OFF_EVENT;
	webServer.stop();
};
void SckESP::webRoot() {

	String htmlPage = htmlPage1;
	
	// Scan for wifi network async
	int netNum = WiFi.scanComplete();

	// Create list of scanned wifi netowrks
	String webSelect;
	for (int i=0; i<netNum; i++) {
		String wSSID = WiFi.SSID(i); 
		webSelect += String(F("<option value=\"")) + wSSID + String(F("\">")) + wSSID + String(F("</option>"));
	}

	// Assemble html page
	htmlPage += webSelect;
	htmlPage += htmlPage2;

	webServer.send(200, "text/html", htmlPage);
};
void SckESP::webSet() {

	// uint8_t score = 0;
	
	String response = "<!DOCTYPE html><html><head><meta name=viewport content=width=device-width, initial-scale=1><style>body {color: #434343;font-family: 'Helvetica Neue',Helvetica,Arial,sans-serif;font-size: 22px;line-height: 1.1;padding: 20px;}</style></head><body>";

	// To send a forced status change if anything changed
	bool somethingChanged = false;

	// TODO support open networks (nopassword)
	// If we found ssid AND pass
	if (webServer.hasArg("ssid"))  {

		String tssid = webServer.arg("ssid");
		String tpass = "";

		if (webServer.hasArg("password")) {
			tpass = webServer.arg("password");
		}
			
		// If ssid is no zero chars
		if (tssid.length() > 0) {

			tssid.toCharArray(credentials.ssid, 64);
			tpass.toCharArray(credentials.password, 64);

			if (addNetwork()) somethingChanged = true;

			response += String(F("Success: New net added: ")) + tssid + " - " + tpass + "<br/>";

			// score ++;
			
		} else {
			response += String(F("Error: wrong ssid or password!!<br/>"));
		}
	
	} else {
		response += String(F("Warning: can't find ssid or password!!<br/>"));
	}

	// If we found the token
	if (webServer.hasArg("token")) {
		String ttoken = webServer.arg("token");
		if (ttoken.length() == 6) {
			ttoken.toCharArray(token, 8);
			saveToken();
			somethingChanged = true;
			response += String(F("Success: New token added: ")) + ttoken + "<br/>";

			// score ++;

		} else {
			response += String(F("Error: token must have 6 chars!!<br/>"));
		}
	} else {
		response += String(F("Warning: no token received<br/>"));
	}

	// If we found new time
	if (webServer.hasArg("epoch")) {

		String tepoch = webServer.arg("epoch");
		uint32_t iepoch = tepoch.toInt();
		const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

		if (iepoch >= DEFAULT_TIME) { 
       		setTime(iepoch);
       		espStatus.time = ESP_TIME_UPDATED_EVENT;
       		somethingChanged = true;
      		debugOUT(F("Time updated from apmode web!!!"));

      		debugOUT(F("Sending time to SAM..."));
			String epochSTR = String(now());
			clearParam();
			epochSTR.toCharArray(msgOut.param, 240);
			msgOut.com = ESP_GET_TIME_COM;
			SAMsendMsg();

       		response += String(F("Success: Time synced: ")) + ISOtime() + "<br/>";
		} else {
			response += String(F("Error: received time is not valid!!<br/>"));
		}

	} else {
		response += String(F("Warning: no time received to sync<br/>"));
	}

	// Interval is in seconds
	if (webServer.hasArg("readint")) {
		
		String tinterval = webServer.arg("readint");
		uint32_t intTinterval = tinterval.toInt();

		// 86400 one day in seconds
		// if (intTinterval > minimal_sensor_reading_interval && intTinterval < max_sensor_reading_interval) {

		// 	for (uint8_t i=0, i<SENSOR_COUNT, i++) {
		// 		wichSensor = static_cast<SensorType>(i);
		// 		config.sensor[wichSensor].interval = intTinterval;
		// 	}

		// 	espStatus.conf = ESP_CONF_CHANGED_EVENT;
		// 	somethingChanged = true;
		// 	response += String(F("Success: New reading interval: ")) + String(intTinterval) + String(F(" seconds"));
		// } else {
		// 	response += String(F("Error: received read interval is not valid!!!"));
		// }
		
	} else {
		response += String(F("Warning: no new reading interval received"));
	}
	
	// Custom Making Sense message TEMP
	// if (score == 2) {
	// 	response += "Thank you<br/>Please finish installing the Smart Citizen Kit on the other screen";
	// } else {
	// 	response += "Something went wrong!!<br/>Please click kit's button until is red and try again!";
	// }
	
	response += "</body></html>";

	webServer.send(200, "text/html", response);
	if (WiFi.status() != WL_CONNECTED) tryConnection();

	// Send message to SAM that something changed via webServer
	if (somethingChanged) {
		clearParam();
		msgOut.com = ESP_WEB_CONFIG_SUCCESS;
		SAMsendMsg();
	}
};
void SckESP::webShow() {
	
	readToken();

	String json = "{\"token\":\"" + String(token) + "\",\"nets\":[";

		int netNum = countSavedNetworks();
	for (int i=0; i<netNum; i++) {
		readNetwork(i);
		json += "{\"ssid\":\"" + String(credentials.ssid);
		json += "\",\"password\":\"" + String(credentials.password) + "\"}";
		if (i < (netNum - 1)) json += ",";
	}
	json += "]}";

	webServer.send(200, "text/json", json);

	json = String();
};


// 	---------------------
//	|	Credentials 	|
//	---------------------
//
bool SckESP::addNetwork() {

	// Prepare json for saving
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["ssid"] = credentials.ssid;
	jsonNet["pass"] = credentials.password;

	//If there is no credentials file yet, create it
	if (!SPIFFS.exists(CREDENTIALS_FILE)) {
		debugOUT(F("Cant find credentials file, creting new..."));
		File credFile = SPIFFS.open(CREDENTIALS_FILE, "w");
		credFile.close();
	} else {

		clearDuplicated();

		//If there is more than 8 sets saved, clear the top one.
		int netCount = countSavedNetworks();
		while (netCount >= MAX_SAVED_CREDENTIALS) {
			debugOUT(F("Max wifi allowed, removing oldest one..."));
			removeNetwork(0);
			netCount = countSavedNetworks();
		}	
	}
	
	File credFile = SPIFFS.open(CREDENTIALS_FILE, "a");
	
	if (credFile) {
		jsonNet.printTo(credFile);
		credFile.write('\n');
		credFile.close();
		debugOUT(String F("Saved network: ") + String(credentials.ssid) + " - " + String(credentials.password));
		return true;
	}
	debugOUT(F("Error saving network!!!"));
	return false;
};
void SckESP::removeNetwork(int index) {

	if (index >= 0) {

		int netCount = countSavedNetworks();

		if (netCount >= index) {

			char oldFileName[] = "/tmp.txt";

			// Renames original file
			SPIFFS.rename(CREDENTIALS_FILE, oldFileName);
			File oldFile = SPIFFS.open(oldFileName, "r");

			// Creates new credentials file
			File newFile = SPIFFS.open(CREDENTIALS_FILE, "w");
						
			if (newFile && oldFile) {

				// Copy previous nets
				int countedLines = 0;
				while (countedLines < index) {
					char b = oldFile.read();
					newFile.write(b);
					if (b == 10) countedLines++;
				}

				// Jumps the one to remove
				while (!oldFile.find(10)) {
					;
				}

				// Copy the rest
				while (oldFile.available()){
					char b = oldFile.read();
					newFile.write(b);
				}

				oldFile.close();
				newFile.close();

				// Removes old file
				SPIFFS.remove(oldFileName);

				debugOUT(String F("Removed network number ") + String(index+1));
			}
		}
	}
};
void SckESP::clearNetworks() {

	if (SPIFFS.exists(CREDENTIALS_FILE)) {
		SPIFFS.remove(CREDENTIALS_FILE);
		msgOut.com = ESP_CLEAR_WIFI_COM;
		SAMsendMsg();
	}
};
bool SckESP::readNetwork(int index) {

	// If no index specified, load last network
	if (index < 0) index = (countSavedNetworks() - 1);
	
	if (SPIFFS.exists(CREDENTIALS_FILE)) {
		File credFile = SPIFFS.open(CREDENTIALS_FILE, "r");

		// Go to line Index
		int countedLines = 0;
		while (countedLines < index) {
			if (credFile.find(10)) countedLines++;
		}

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
		strcpy(credentials.ssid, jsonNet["ssid"]);
		strcpy(credentials.password, jsonNet["pass"]);
		debugOUT(String(index+1) + F(": ") + String(credentials.ssid) + F(" - ") + String(credentials.password));
		return true;
	}
	
	return false;
};
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
	
	return -1;
};
void SckESP::clearDuplicated() {

	// Temporary store original credentials
	char checkingSSID[64];
	char checkingPASS[64];
	strncpy(checkingSSID, credentials.ssid, 64);
	strncpy(checkingPASS, credentials.password, 64);

	int netCount = countSavedNetworks();
	if (netCount > 0) {

		// Iterate over all saved networks
		for (int i=0;  i<netCount; i++) {

			if (readNetwork(i)) {
			
				int comparison = strcmp(checkingSSID, credentials.ssid);

				// If duplicated net found
				if (comparison == 0) {
					debugOUT(F("Removing duplicated network..."));
					removeNetwork(i);
					break;
				}
			}
		}
	}
	// Load again original credentials
	strncpy(credentials.ssid, checkingSSID, 64);
	strncpy(credentials.password, checkingPASS, 64);
};
int8_t SckESP::selectBestNetwork() {

	debugOUT(F("Scanning for known networks..."));

	// uint8_t netNumber = scanAP();
	uint8_t savedNetNumber = countSavedNetworks();

	String tssid;
	int32_t trssi = -9999;
	int8_t theBest = -1;

	if (savedNetNumber > 1) {
		for (int i=0; i<savedNetNumber; i++){
			readNetwork(i);
			for (int ii=0; ii<netNumber; ii++){
				char WIFIssid[64];
				int32_t WIFIrssi = WiFi.RSSI(ii);
				WiFi.SSID(ii).toCharArray(WIFIssid, 64);
				int comparison = strcmp(credentials.ssid, WIFIssid);
				if (comparison == 0 && WIFIrssi > trssi) {
					theBest = i;
					tssid = WIFIssid;
					trssi = WiFi.RSSI(ii);
				}
			}
		}
	}
	if (theBest >= 0) {
		debugOUT(String F("The best network is: ") + tssid);
		readNetwork(theBest);
	} else {
		debugOUT(F("Cant' find any saved network online, no best net!!!"));
	}
	return theBest;
};
void SckESP::setGoodNet(){

	// This function seems the same as addNetwork(), but only writes the file if its necessary to protect flash from wearing out.

	// Remeber actual net
	char goodSSID[64];
	char goodPASS[64];
	strncpy(goodSSID, credentials.ssid, 64);
	strncpy(goodPASS, credentials.password, 64);

	// Read last network
	readNetwork();

	// Only if best network is not in the last position, we move it
	int comparison = strcmp(goodSSID, credentials.ssid);
	if (comparison != 0) {

		strncpy(credentials.ssid, goodSSID, 64);
		strncpy(credentials.password, goodPASS, 64);

		debugOUT(String F("Setting ") + String(credentials.ssid) + F(" as preferred network"));

		addNetwork();

	} else {
		debugOUT(String(credentials.ssid) + F(" is already the preferred network"));
	}
};
void SckESP::scanAP() {
	netNumber = WiFi.scanNetworks();
	// Wait for scan...
	while (WiFi.scanComplete() == -2) {
		;
	}
	debugOUT(String(netNumber) + F(" networks found"));
};


// 	-------------
//	|	Token 	|
//	-------------
//
void SckESP::saveToken() {
	
	File tokenFile = SPIFFS.open(TOKEN_FILE, "w");
	if (tokenFile) {
		tokenFile.print(token);
		tokenFile.close();
	}
};
void SckESP::readToken() {
	if (!SPIFFS.exists(TOKEN_FILE)) {
		File tokenFile = SPIFFS.open(TOKEN_FILE, "w");
		if (tokenFile) {
			tokenFile.print("null");
			tokenFile.close();
		}
	}

	File tokenFile = SPIFFS.open(TOKEN_FILE, "r");
	if (tokenFile) {
		// Read line
		String buffer;
		while (tokenFile.available()) {
			char buff = tokenFile.read();
			buffer += buff;
		}
		buffer.toCharArray(token, 8);
	}
};



// 	-------------
//	|	MQTT 	|
//	-------------
//
bool SckESP::mqttStart() {
	
	debugOUT(F("Connecting to MQTT server..."));

	MQTTclient.setServer(MQTT_SERVER_NAME, 1883);

	if (MQTTclient.connect(token)) {
		debugOUT(F("Established MQTT connection..."));
		return true;
	} else {
		debugOUT(F("ERROR: MQTT connection failed!!!"));
		debugOUT(String(MQTTclient.state()));
		espStatus.mqtt = ESP_MQTT_ERROR_EVENT;
		return false;
	}
};
bool SckESP::mqttHellow() {

	debugOUT(F("Trying MQTT Hello..."));

	if (!MQTTclient.connected()) {
    	mqttStart();
  	}

	String helloTopic = String F("device/sck/") + String(token) + F("/hello");
	char ht[helloTopic.length()+1];
	helloTopic.toCharArray(ht, helloTopic.length()+1);

	String helloPayload = String(token) + F(":Hello");
	char pl[helloPayload.length()+1];
	helloPayload.toCharArray(pl, helloPayload.length()+1);

	debugOUT(String(ht));
	debugOUT(String(pl));

	if (MQTTclient.publish(ht, pl)) {
		debugOUT(F("MQTT Hello published OK !!!"));
		return true;
	}
	debugOUT(F("MQTT Hello ERROR !!!"));
	return false;
};
bool SckESP::mqttPublish(){

	debugOUT(F("Trying MQTT publish..."));

	if (!MQTTclient.connected()) {
    	mqttStart();
  	}

  	String Publishtopic = String F("device/sck/") + String(token) + F("/readings");
	char ht[Publishtopic.length()+1];
	Publishtopic.toCharArray(ht, Publishtopic.length()+1);

	/* Example
	{	"data":[
			{"recorded_at":"2017-03-24T13:35:14Z",
				"sensors":[
					{"id":29,"value":48.45},
					{"id":13,"value":66},
					{"id":12,"value":28},
					{"id":10,"value":4.45}
				]
			}
		]
	}
		*/

	// Prepare json for sending (very big buffer)
	uint16_t bufferSize = 4096;
	DynamicJsonBuffer jsonBuffer(bufferSize);
	
	// Root object
	JsonObject& jsonPublish = jsonBuffer.createObject();

	// Data array
	JsonArray& jsonData = jsonPublish.createNestedArray("data");

	// This time data object
	JsonObject& jsonThisTime = jsonData.createNestedObject();

	// Add time
	jsonThisTime["recorded_at"] = (epoch2iso(sensors[SENSOR_BATTERY].lastReadingTime));		// temp,

	// Sensors array
	JsonArray& jsonSensors = jsonThisTime.createNestedArray("sensors");

	// Iterate over sensors, only publish enabled sensors and with a valid id (ID != 0), also dont publish time as a sensor
	for (uint8_t i=1; i<SENSOR_COUNT; i++) {

		SensorType wichSensor = static_cast<SensorType>(i);

		// Only send enabled sensors
		if (sensors[wichSensor].enabled && sensors[wichSensor].id != 0) {
			
			// Create sensor object
			JsonObject& jsonThisSensor = jsonSensors.createNestedObject();

			// Add id and value
			jsonThisSensor["id"] = sensors[wichSensor].id;
			jsonThisSensor["value"] =  double_with_n_digits(sensors[wichSensor].reading, 2);
		}
	}

	// To allow big MQTT messages (more than 128 bytes) we had modified PubSubClient.h MQTT_MAX_PACKET_SIZE
	// If we see any problem with big packet transmission we should implement the stream option of the pubsubclient
	if (jsonPublish.measureLength() > 128) debugOUT(F("WARNING: MQTT message too big, remember to modify puSubClient library limit!!!"));

	char charPl[jsonPublish.measureLength()+1];
	jsonPublish.printTo(charPl, jsonPublish.measureLength()+1);

	// String to be published
	debugOUT(String(charPl));

	
	if (MQTTclient.publish(ht, charPl)) {
		return true;
		debugOUT(F("MQTT readings published OK !!!"));
	}
	debugOUT(F("MQTT readings ERROR !!!"));
	return false;
}
bool SckESP::mqttSend(String payload) {
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
      
      espStatus.time = ESP_TIME_UPDATED_EVENT;
      debugOUT(F("Time updated!!!"));

      return secsSince1900 - 2208988800UL;
    }
  }
  espStatus.time = ESP_TIME_FAIL_EVENT;
  debugOUT(F("No NTP Response!!!"));
  getHttpTime();
  return 0;
};
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
};
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
};
void SckESP::ledToggle(uint8_t wichLed) {
	ledValue[wichLed] = abs(ledValue[wichLed] - 1);
	digitalWrite(ledPin[wichLed], ledValue[wichLed]);
};
void SckESP::ledBlink(uint8_t wichLed, float rate) {
	if (wichLed == ledLeft) Lblink.attach_ms(rate, LedToggleLeft);
	else if (wichLed == ledRight) Rblink.attach_ms(rate, LedToggleRight);
};



