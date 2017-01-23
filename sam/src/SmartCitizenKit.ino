#include "sckBase.h"
#include "sckUrban.h"
#include "sckAux.h"

SckBase base;
SckUrban urban;

// Button interrupt handler BUSCAR COMO PONERLO DENTRO DEL CODIGO sckBase
void ISR_button() {
	base.buttonEvent();
};
// Timer 5 interrupt handler   BUSCAR COMO PONERLO DENTRO DEL CODIGO sckBase
void TC5_Handler (void) {
	base.led.tick();
    // Clear the interrupt
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
};

void setup() {

	base.setup();
	// urban.setup();

	// Gets readings for first post
	// base.readings.time = base.ISOtime();
	// base.readings.noise.data = urban.GetNoise();
	// base.readings.humidity.data = urban.getHumidity();
	// base.readings.temperature.data = urban.getTemperature();
	// base.readings.battery.data = base.getBattery();

}

void loop() {

	base.update();

	// if (millis() - base.intervalTimer > base.postInterval * 1000 && base.mode == MODE_NET){

	// 	// take reading
	// 	base.readings.time = base.ISOtime();
	// 	base.readings.noise.data = urban.GetNoise();
	// 	base.readings.humidity.data = urban.getHumidity();
	// 	base.readings.temperature.data = urban.getTemperature();
	// 	base.readings.battery.data = base.getBattery();

	// 	base.intervalTimer = millis();

	// 	// save to sdcard (TEMP)
	// 	// if (base.openPublishFile()) {
	// 	// 	base.publishFile.print(base.readings.time);
	// 	// 	base.publishFile.print(",");
	// 	// 	base.publishFile.print(base.readings.noise);
	// 	// 	base.publishFile.print(",");
	// 	// 	base.publishFile.print(base.readings.humidity);
	// 	// 	base.publishFile.print(",");
	// 	// 	base.publishFile.print(base.readings.temperature);
	// 	// 	base.publishFile.print(",");
	// 	// 	base.publishFile.print(base.readings.battery);
	// 	// 	base.publishFile.print("\n");
	// 	// 	base.publishFile.close();
	// 	// }

	// 	// If time is not updated or last update is oldest than 12 hours (43200 sec), updates time
	// 	if (!base.onTime || base.rtc.getEpoch() - base.lastTimeSync > 43200) base.ESPsendCommand(F("sck.getTime()"));

	// 	// Publish data
	// 	else base.ESPpublish();
	// }
}
