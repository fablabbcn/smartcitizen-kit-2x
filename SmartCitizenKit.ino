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
	// base.inputUpdate();
	
    // Clear the interrupt
    TC5->COUNT16.INTFLAG.bit.MC0 = 1;
};

float interval = 15000;
float intervalTimer = millis();

void setup() {

	base.setup();

}

void loop() {

	base.update();

	if (millis() - intervalTimer > interval && base.mode == MODE_NET){

		// take reading
		base.payloadData.time = base.ISOtime();
		base.payloadData.noise = urban.GetNoise();
		base.payloadData.humidity = urban.getHumidity();
		base.payloadData.temperature = urban.getTemperature();
		base.payloadData.battery = urban.getBattery();

		// save to sdcard (TEMP)
		if (base.openPublishFile()) {
			base.publishFile.print(base.payloadData.time);
			base.publishFile.print(",");
			base.publishFile.print(base.payloadData.noise);
			base.publishFile.print(",");
			base.publishFile.print(base.payloadData.humidity);
			base.publishFile.print(",");
			base.publishFile.print(base.payloadData.temperature);
			base.publishFile.print(",");
			base.publishFile.print(base.payloadData.battery);
			base.publishFile.print("\n");
			base.publishFile.close();
		}

		// If time is not updated or last update is oldest than 12 hours (43200 sec), updates time
		if (!base.onTime || base.rtc.getEpoch() - base.lastTimeSync > 43200) base.ESPsendCommand(F("sck.getTime()"));

		// Publish data
		else base.ESPpublish();

		intervalTimer = millis();
	}
}


// KITS
// e82d8e = 3769
// a1b2c3 = 3770
// t7d9z7 = 3771
/*
--------------------------------------
-- MQTT
--------------------------------------
--mqtt:connect(host[, port[, secure[, autoreconnect]]][, function(client)[, function(client, reason)]])

-- mqtt:subscribe(topic, qos[, function(client)])
-- mqtt:subscribe(table[, function(client)])
-- table: array of 'topic, qos' pairs to subscribe to
-- `/device/sck/devicetoken:/hello`
-- `/device/sck/devicetoken:/readings`
-- `/device/sck/devicetoken:/settings`
-- `/device/sck/devicetoken:/errors`

-- mqtt:publish(topic, payload, qos, retain[, function(client)])
*/