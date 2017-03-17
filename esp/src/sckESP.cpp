#include "sckESP.h"

// Structs for SAM <<>> ESP communication
EasyTransfer BUS_in, BUS_out;

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

	// Wifi settings
	WiFi.hostname("smartcitizen");
	WiFi.persistent(false);		 		// Only write to flash credentials when changed (for protecting flash from wearing out)
	readNetwork();
	startAP();
};
void SckESP::update() {

	if (BUS_in.receiveData()) processMsg();

	actualWIFIStatus = WiFi.status();

	if (espStatus.web == ESP_WEB_ON_EVENT)	{
		webServer.handleClient();
		dnsServer.processNextRequest();
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
			Udp.begin(8888);
			setSyncProvider(ntpProvider);
			setSyncInterval(300);

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
			if (countSavedNetworks() > 0 && selectBestNetwork() >= 0) { 
			
				tryConnection();
			
			} else {
			
				// NO network available: Enter Ap mode
				ledBlink(ledRight, 100);
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
				msgOut.com = ESP_SET_WIFI_COM;
				sendNetwork(1);
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
	 		msgOut.com = ESP_GET_APCOUNT_COM;
	 		int n = scanAP();
			String sn = String(n);
			clearParam();
			sn.toCharArray(msgOut.param, 240);
	 		SAMsendMsg();
	 		break;

	 	} case ESP_GET_APLIST_COM: {
	 		msgOut.com = ESP_GET_APLIST_COM;
	 		
	 		int netNum = scanAP();

			for (int i=0; i<netNum; i++) {
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
	 		msgOut.com = ESP_GET_TIME_COM;
			String epochSTR = String(now());
			clearParam();
			epochSTR.toCharArray(msgOut.param, 240);
			SAMsendMsg();
	 		break;

	 	} case ESP_LED_OFF: {
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
	 	}
	}

	// Clear msg
	msgIn.time = 0;
	msgIn.com = 0;
	strncpy(msgIn.param, "", 240);
};
void SckESP::debugOUT(String strOut) {

	// TODO implementar otras vias de debug output
	if (serialDebug) { 
		Serial.print(F("ESP > "));
		Serial.println(strOut);
	}
};
void SckESP::SAMsendMsg() {

	if (sizeof(msgOut.param) > 240) {
		// TODO handle multiple packages in msgout
		debugOUT(F("WARNING: Serial package is more than 256 bytes of size!!!"));
	}

	msgOut.time = now();
	BUS_out.sendData();
};
void SckESP::sendStatus() {

	// debugOUT(F("Sending status data."));

	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["wifi"] 	= espStatus.wifi;
	jsonNet["net"] 		= espStatus.net;
	jsonNet["mqtt"] 	= espStatus.mqtt;
	jsonNet["time"] 	= espStatus.time;
	jsonNet["ap"] 		= espStatus.ap;
	jsonNet["web"]		= espStatus.web;
	jsonNet["conf"] 	= espStatus.conf;

	clearParam();
	jsonNet.printTo(msgOut.param, 240);
	msgOut.com = ESP_GET_STATUS_COM;
	SAMsendMsg();
};
void SckESP::clearParam() {

	memset(msgOut.param, 0, sizeof(msgOut.param));
};
void SckESP::sendNetwork(uint8_t index) {

	// Prepare json for sending
	StaticJsonBuffer<240> jsonBuffer;
	JsonObject& jsonNet = jsonBuffer.createObject();
	jsonNet["n"] = index;
	jsonNet["s"] = credentials.ssid;
	jsonNet["p"] = credentials.password;
	clearParam();
	jsonNet.printTo(msgOut.param, 240);
	SAMsendMsg();
};
void SckESP::SAMlistSavedNetworks() {

	int netCount = countSavedNetworks();

	if (netCount > 0) {

		msgOut.com = ESP_GET_WIFI_COM;
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

		WiFi.begin(credentials.ssid, credentials.password);
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

	// Build SSID name
	uint8_t macAddr[6];
	WiFi.softAPmacAddress(macAddr);

	String base = "SmartCitizen";
	String completo = base + String(macAddr[5]);

	char ssidName[16];
	completo.toCharArray(ssidName, 16);

	debugOUT(String F("Starting Ap with ssid: ") + String(ssidName));

	// IP for DNS
	IPAddress myIP(192, 168, 1, 1);

	// Start Soft AP
	WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(ssidName);
	espStatus.ap = ESP_AP_ON_EVENT;

	// DNS stuff (captive portal)
	dnsServer.start(DNS_PORT, "*", myIP);

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
	webServer.on("/aplist", [](){

   		String json = "{\"nets\":[";

   		int netNum = WiFi.scanNetworks();
		for (int i=0; i<netNum; i++) {
			json += "{\"ssid\":\"" + String(WiFi.SSID(i));
			json += "\",\"ch\":" + String(WiFi.channel(i));
			json += ",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
			if (i < (netNum - 1)) json += ",";
		}

		json += "]}";
		webServer.send(200, "text/json", json);
		json = String();
    	
	});

	// Handle SSDP
	webServer.on("/description.xml", HTTP_GET, [](){
    	SSDP.schema(webServer.client());
	});


	// mDNS
	if (!MDNS.begin("smartcitizen")) {
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

	// If no scan has been started, start one...
	if (netNum == -2) WiFi.scanNetworks();

	// // Wait for (ONLY) first scan...
	while (WiFi.scanComplete() == -2) {
		;
	}

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
	
	String response = "<!DOCTYPE html><html><head><meta name=viewport content=width=device-width, initial-scale=1><style>body {color: #434343;font-family: 'Helvetica Neue',Helvetica,Arial,sans-serif;font-size: 22px;line-height: 1.1;padding: 20px;}</style></head><body>";

	// TODO support open networks (nopassword)
	// If we found ssid AND pass
	if (webServer.hasArg("ssid") && webServer.hasArg("password"))  {

		String tssid = webServer.arg("ssid");
		String tpass = webServer.arg("password");

		// If ssid is no zero chars and pass is at least 8 chars
		if (tssid.length() > 0 && tpass.length() > 7) {

			tssid.toCharArray(credentials.ssid, 64);
			tpass.toCharArray(credentials.password, 64);

			addNetwork();

			response += String(F("Success: New net added: ")) + tssid + " - " + tpass + "<br/>";
			
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
			response += String(F("Success: New token added: ")) + ttoken + "<br/>";
		} else {
			response += String(F("Error: token must have 6 chars!!<br/>"));
		}
	} else {
		response += String(F("Warning: no token received<br/>"));
	}

	if (webServer.hasArg("epoch")) {

		String tepoch = webServer.arg("epoch");
		uint32_t iepoch = tepoch.toInt();
		const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

		if (iepoch >= DEFAULT_TIME) { 
       		setTime(iepoch);
       		espStatus.time = ESP_TIME_UPDATED_EVENT;
      		debugOUT(F("Time updated from apmode web!!!"));
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
		if (intTinterval > 0 && intTinterval < ONE_DAY_IN_SECONDS) {
			configuration.readInterval = intTinterval;
			espStatus.conf = ESP_CONF_CHANGED_EVENT;
			response += String(F("Success: New reading interval: ")) + String(intTinterval) + String(F(" seconds"));
		} else {
			response += String(F("Error: received read interval is not valid!!!"));
		}
		
	} else {
		response += String(F("Warning: no new reading interval received"));
	}
	
	response += "</body></html>";

	webServer.send(200, "text/html", response);
	if (WiFi.status() != WL_CONNECTED) tryConnection();
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

	uint8_t netNumber = scanAP();
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
int SckESP::scanAP() {
	int n = WiFi.scanNetworks();
	debugOUT(String(n) + F(" networks found"));
	return n;
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
bool SckESP::mqttStart(String server) {

};
bool SckESP::mqttHellow() {

};
bool SckESP::mqttSend(String payload) {

};


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



