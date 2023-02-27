#include "SckSerial.h"

void SckSerial::begin(uint32_t bauds) 
{
	_serial.begin(bauds);
}
bool  SckSerial::_send()
{
	// Sends header 
	_serial.write(0x06);
	_serial.write(0x85);

	// Sends message
	_serial.write(msg);

	char debugBuff[24];
	snprintf(debugBuff, 24, "Sent msg: %u", msg);
	debugPln(debugBuff);

	// Sends size
	uint16_t size = (uint16_t)strlen(buff);
	_serial.write(reinterpret_cast<uint8_t*>(&size), sizeof(uint16_t));

	snprintf(debugBuff, 24, "Sent size: %u", size);
	debugPln(debugBuff);

	// Jump this if there is no payload to send
	if (size > 0) {

		// Sends payload
		uint16_t sum = 0;
		for (uint16_t i=0; i<size; i++) {
			sum ^= buff[i];		// checksum calculation
			_serial.write(buff[i]);
		}

		debugP("Sent message: ");
		debugPln(buff);

		// Send checksum
		_serial.write(reinterpret_cast<uint8_t*>(&sum), sizeof(uint16_t));
		snprintf(debugBuff, 24, "Sent chksum: %u", sum);
		debugPln(debugBuff);
	}

	// Check for ACK
	byte rxack[2];
	if (!_serial.readBytes(rxack, 2)) {
			debugPln("Timeout while waiting for ACK");
			return false;
	}
	if (rxack[0] != 0x06 || rxack[1] != 0x86) {
		debugPln("Error on receiving ACK");
		return false;
	}
	debugPln("ACK received");

	// clear buffer
	buff[0] = '\0';

	return true;
}
bool SckSerial::send(SCKMessage wichMessage)
{
	// Use this function if the buffer is prefilled with payload
	
	msg = wichMessage;
	
	// Retry loop
	uint8_t _try = 0;
	while (_try < maxTry) {
		if (_send()) {
			error = false;
			return true;
		}
		_try++;
	}

	error = true;

	debugPln("ERROR sending msg to ESP", true);

	return false;
}
bool SckSerial::send(SCKMessage wichMessage, const char *content)
{
	// Use this function to send payload as parameter
	// or to send a msg without payload (by sending an empty content: "")
	
	// Check if message fits on buffer
	if (strlen(content) > NETBUFF_SIZE) return false;

	// Fills buffer with content
	sprintf(buff, "%s", content);

	return send(wichMessage);
}
bool SckSerial::receive()
{
	// Check if enough data available: header + msg + size
	if (_serial.available() < 2 + 1 + 2 ) return false;

	// check for Header char
	if (_serial.read() != 0x06) return false;
	if (_serial.read() != 0x85) return false;
	
	debugPln("Got header char");

	// Get SCKmessage (not payload)
	msg = _serial.read();

	char debugBuff[24];
	snprintf(debugBuff, 24, "msg: %u", msg);
	debugPln(debugBuff);

	// Get size
	uint16_t size;
	_serial.readBytes(reinterpret_cast<uint8_t*>(&size), sizeof(uint16_t));

	snprintf(debugBuff, 24, "size: %u", size);
	debugPln(debugBuff);

	// Finish now if no payload is present (just command)
	if (size == 0) {
		debugPln("Msg without payload");
		sendACK();
		return true;
	}
	
	// Get payload
	uint16_t readed = _serial.readBytes(buff, size);
	if (readed != size) {
		debugPln("Error while receiving payload");
		return false;
	}

	// Add end of string
	buff[size] = '\0';

	// Calculate checksum
	uint16_t csCalc = 0;
	for (uint16_t i=0; i<size; i++)	csCalc ^= buff[i];

	debugP("Payload: ");
	debugPln(buff);

	// Gets and compares checksum
	uint16_t csRec;
	_serial.readBytes(reinterpret_cast<uint8_t*>(&csRec), sizeof(uint16_t));

	snprintf(debugBuff, 24, "Chksum calc: %u", csCalc);
	debugP(debugBuff);
	snprintf(debugBuff, 24, " - received: %u", csRec);
	debugP(debugBuff);

	if (csRec != csCalc) {
		debugPln(" -> ERROR");
		return false;
	}
	debugPln(" -> OK");

	sendACK();
	return true;
}
void SckSerial::sendACK()
{
	debugPln("Sending ACK");
	_serial.write(0x06);
	_serial.write(0x86);
}
void SckSerial::debugP(char *msg, bool force)
{
	if (!debug && !force) return;

// Debug for SAM via SerialUSB
#ifdef ARDUINO_ARCH_SAMD
	SerialUSB.print(msg);
#endif

// Debug for ESP
#ifdef ARDUINO_ARCH_ESP8266
#endif
}
void SckSerial::debugPln(char *msg, bool force)
{
	if (!debug && !force) return;

// Debug for SAM via SerialUSB
#ifdef ARDUINO_ARCH_SAMD
	SerialUSB.println(msg);
#endif

// Debug for ESP
#ifdef ARDUINO_ARCH_ESP8266
#endif
}
