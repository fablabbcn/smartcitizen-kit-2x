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
	urban.setup();

	// Gets readings for first post
	base.payloadData.time = base.ISOtime();
	base.payloadData.noise = urban.GetNoise();
	base.payloadData.humidity = urban.getHumidity();
	base.payloadData.temperature = urban.getTemperature();
	base.payloadData.battery = urban.getBattery();

}

void loop() {

	base.update();

	if (millis() - base.intervalTimer > base.postInterval * 1000 && base.mode == MODE_NET){

		// take reading
		base.payloadData.time = base.ISOtime();
		base.payloadData.noise = urban.GetNoise();
		base.payloadData.humidity = urban.getHumidity();
		base.payloadData.temperature = urban.getTemperature();
		base.payloadData.battery = urban.getBattery();

		base.intervalTimer = millis();

		// save to sdcard (TEMP)
		// if (base.openPublishFile()) {
		// 	base.publishFile.print(base.payloadData.time);
		// 	base.publishFile.print(",");
		// 	base.publishFile.print(base.payloadData.noise);
		// 	base.publishFile.print(",");
		// 	base.publishFile.print(base.payloadData.humidity);
		// 	base.publishFile.print(",");
		// 	base.publishFile.print(base.payloadData.temperature);
		// 	base.publishFile.print(",");
		// 	base.publishFile.print(base.payloadData.battery);
		// 	base.publishFile.print("\n");
		// 	base.publishFile.close();
		// }

		// If time is not updated or last update is oldest than 12 hours (43200 sec), updates time
		if (!base.onTime || base.rtc.getEpoch() - base.lastTimeSync > 43200) base.ESPsendCommand(F("sck.getTime()"));

		// Publish data
		else base.ESPpublish();
	}
}
