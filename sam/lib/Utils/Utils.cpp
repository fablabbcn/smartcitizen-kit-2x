#include "Utils.h"
#include "Pins.h"
#include <stdarg.h>

void prompt()
{
	Serial.print("SCK > ");
}

// TODO view how to manage plot function... (auxboards is deprecated!!)
void plot(String value, const char *title, const char *unit)
{
	// auxBoards.plot(value, title, unit);
}

void sckPrintf(const char * format, ...)
{
	const uint8_t buffSize = 240;
	char mybuff[buffSize];

	va_list pargs;
	va_start(pargs, format);
	vsnprintf(mybuff, buffSize, format, pargs);
	va_end(pargs);

	Serial.print(mybuff);

	// TODO config does not exist here...
	// Debug output to sdcard
	// if (config.debug.sdcard) {
	// 	if (!sdSelect()) return;
	// 	debugFile.file = sd.open(debugFile.name, FILE_WRITE);
	// 	if (debugFile.file) {
	// 		ISOtime();
	// 		debugFile.file.print(ISOtimeBuff);
	// 		debugFile.file.print("-->");
	// 		debugFile.file.println(mybuff); // TODO pass only a pointer
	// 		debugFile.file.close();
	// 	} else st.cardPresent = false;
	// }
	//
	// // Debug output to oled display
	// if (config.debug.oled) {
	// 	if (sensors[SENSOR_GROVE_OLED].enabled) auxBoards.print(mybuff); // TODO pass only a pointer
	// }
}
void sckPrintfln(const char * format, ...)
{
	const uint8_t buffSize = 240;
	char mybuff[buffSize];

	va_list pargs;
	va_start(pargs, format);
	vsnprintf(mybuff, buffSize, format, pargs);
	va_end(pargs);
	
	Serial.println(mybuff);
}
