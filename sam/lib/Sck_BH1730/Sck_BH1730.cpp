#include  "Sck_BH1730.h"

bool Ctrl_BH1730::start(TwoWire * _wire, byte address)
{
	// TODO ver que se puede usar para estar seguro de que el sensor es el que pensamos (ask for model??)
	if (started) return true;

	wireBus = _wire;
	addr = address;

	started = true;
	return true;
}

bool Ctrl_BH1730::stop()
{
	// 0x00 register - CONTROL
	uint8_t CONTROL = B000000;
	// ADC_INTR: 	5	0:Interrupt is inactive.
	// 			1:Interrupt is active.
	// ADC_VALID:	4	0:ADC data is not updated after last reading.
	// 			1:ADC data is updated after last reading.
	// ONE_TIME:	3	0:ADC measurement is continuous.
	// 			1:ADC measurement is one time.
	// 			ADC	transits to power down automatically.
	// DATA_SEL:	2	0:ADC measurement Type0 and Type1.
	// 			1:ADC measurement Type0 only.
	// ADC_EN:	1	0:ADC measurement stop.
	// 			1:ADC measurement start.
	// POWER:	0	0:ADC power down.
	// 			1:ADC power on.

	// Send Configuration
	// This will save around 150 uA
	wireBus->beginTransmission(addr);
	wireBus->write(0x80);
	wireBus->write(CONTROL);
	wireBus->endTransmission();

	started = false;
	return true;
}

int8_t Ctrl_BH1730::getReading(const Measurement * measurement, char * buff)
{
	float result = 0;

	// Start in the default integration time
	ITIME = 218;

	if (!updateValues()) return -1;

	// Adjust the Integration Time (ITIME)
	for (uint8_t i=0; i<6; i++) {

		if (DATA0 > goUp || DATA1 > goUp) {
			ITIME += (((ITIME_max - ITIME) / 2) + 1);
			if (ITIME > 250) ITIME = ITIME_max;

			// TODO unificar el sistema de debug para los device objects
			if (debug) {
				SerialUSB.print(DATA0);
				SerialUSB.print(" -- ");
				SerialUSB.print(DATA1);
				SerialUSB.print(" >> ");
				SerialUSB.println(ITIME);
			}
		} else break;

		if (!updateValues()) return -1;
	}

	// Lux calculation (Datasheet page 13)
	float Lx = 0;
	if (DATA0 > 0 && DATA1 > 0) {
		if (DATA1/DATA0 < 0.26) Lx = ((1.290 * DATA0 - 2.733 * DATA1) / Gain) * (102.6 / ITIME_ms);
		else if (DATA1/DATA0 < 0.55) Lx = ((0.795 * DATA0 - 0.859 * DATA1) / Gain) * (102.6 / ITIME_ms);
		else if (DATA1/DATA0 < 1.09) Lx = ((0.510 * DATA0 - 0.345 * DATA1) / Gain) * (102.6 / ITIME_ms);
		else if (DATA1/DATA0 < 2.13) Lx = ((0.276 * DATA0 - 0.130 * DATA1) / Gain) * (102.6 / ITIME_ms);
		else Lx = 0;
	}

	Lx = max(0, Lx);
	result  = (int)Lx;

	if (debug) {
		SerialUSB.print("Integration Time ITIME_ms: ");
		SerialUSB.println(ITIME_ms);
		SerialUSB.print("Measurement Time Tmt: ");
		SerialUSB.println(Tmt);
		SerialUSB.print("Gain: ");
		SerialUSB.println(Gain);
		SerialUSB.print("Visible Light DATA0: ");
		SerialUSB.println(DATA0);
		SerialUSB.print("Infrared Light DATA1: ");
		SerialUSB.println(DATA1);
		SerialUSB.print("Calculated Lux: ");
		SerialUSB.println(Lx);
	}

	stop();

	snprintf(buff, READING_BUFF_SIZE, "%.*f", measurement->precision, result);
	return 0;
}

bool Ctrl_BH1730::updateValues()
{
	// Datasheet http://rohmfs.rohm.com/en/products/databook/datasheet/ic/sensor/light/bh1730fvc-e.pdf

	// 0x00 register - CONTROL
	uint8_t CONTROL = B000011;
	// ADC_INTR: 	5	0:Interrupt is inactive.
	// 			1:Interrupt is active.
	// ADC_VALID:	4	0:ADC data is not updated after last reading.
	// 			1:ADC data is updated after last reading.
	// ONE_TIME:	3	0:ADC measurement is continuous.
	// 			1:ADC measurement is one time.
	// 			ADC	transits to power down automatically.
	// DATA_SEL:	2	0:ADC measurement Type0 and Type1.
	// 			1:ADC measurement Type0 only.
	// ADC_EN:	1	0:ADC measurement stop.
	// 			1:ADC measurement start.
	// POWER:	0	0:ADC power down.
	// 			1:ADC power on.

	// 0x01 register - TIMMING
	// uint8_t ITIME  = 0xDA; 	// Datasheet default value (218 DEC)

	// 00h: Start / Stop of measurement is set by special command. (ADC manual integration mode)
	// 01h to FFh: Integration time is determined by ITIME value (defaultt is oxDA)
	// Integration Time : ITIME_ms = Tint * 964 * (256 - ITIME)
	// Measurement time : Tmt= ITIME_ms + Tint * 714

	// 0x02 register - INTERRUPT
	uint8_t INTERRUPT = B00000000;
	// RES 		7 	Write 0
	// INT_STOP	6	0 : ADC measurement does not stop.
	// 				1 : ADC measurement stops and transits to power down mode when interrupt becomes active.
	// RES 		5 	Write 0
	// INT_EN 	4 	0 : Disable interrupt function.
	// 				1 : Enable interrupt function.
	// PERSIST 	3:0 Interrupt persistence function.
	//	0000 : Interrupt becomes active at each measurement end.
	//	0001 : Interrupt status is updated at each measurement end.
	//	0010 : Interrupt status is updated if two consecutive threshold judgments are the same.
	//	When  set  0011  or  more,  interrupt  status  is  updated  if same threshold judgments continue consecutively same times as the number set in PERSIST.

	// 0x03, 0x04 registers - TH_LOW Low interrupt threshold
	uint8_t TH_LOW0 = 0x00;		// Lower byte of low interrupt threshold
	uint8_t TH_LOW1 = 0x00;		// Upper byte of low interrupt threshold

	// 0x05, 0x06 registers - TH_UP High interrupt threshold
	uint8_t TH_UP0 = 0xFF;		// Lower byte of high interrupt threshold
	uint8_t TH_UP1 = 0xFF;		// Upper byte of high interrupt threshold

	// 0x07 - GAIN
	uint8_t GAIN = 0x00;
	// GAIN  2:0	ADC resolution setting
	//	  X00 : x1 gain mode
	//	  X01 : x2 gain mode
	//	  X10 : x64 gain mode
	//	  X11 : x128 gain mode

	uint8_t DATA[8] = {CONTROL, ITIME, INTERRUPT, TH_LOW0, TH_LOW1, TH_UP0, TH_UP1, GAIN};

	// Send Configuration
	wireBus->beginTransmission(addr);
	wireBus->write(0x80);
	for (int i= 0; i<8; i++) wireBus->write(DATA[i]);
	wireBus->endTransmission();

	// Calculate timming values
	ITIME_ms = (Tint * 964 * (256 - ITIME)) / 1000;
	Tmt =  ITIME_ms + (Tint * 714);

	// Wait for ADC to finish
	uint32_t started = millis();
	uint8_t answer = 0;
	while ((answer & 0x10) == 0) {
		delay(10);
		wireBus->beginTransmission(addr);
		wireBus->write(0x80);
		wireBus->endTransmission();
		wireBus->requestFrom(addr, 1);
		answer = wireBus->read();
		if (millis() - started > Tmt) {
			if (debug) SerialUSB.println("ERROR: Timeout waiting for reading");
			return false;
		}
	}

	// Ask for reading
	wireBus->beginTransmission(addr);
	wireBus->write(0x94);
	wireBus->endTransmission();
	wireBus->requestFrom(addr, 4);

	// Get result
	uint16_t IDATA0 = 0;
	uint16_t IDATA1 = 0;
	IDATA0 = wireBus->read();
	IDATA0 = IDATA0 | (wireBus->read()<<8);
	IDATA1 = wireBus->read();
	IDATA1 = IDATA1 | (wireBus->read()<<8);
	DATA0 = (float)IDATA0;
	DATA1 = (float)IDATA1;

	// Setup gain
	Gain = 1;
	if (GAIN == 0x01) Gain = 2;
	else if (GAIN == 0x02) Gain = 64;
	else if (GAIN == 0x03) Gain = 128;

	return true;
}
