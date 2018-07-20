#include "SckUrban.h"
#include "SckBase.h"

bool SckUrban::begin(SckBase *base)
{
	uint32_t currentTime = 0;
	if (base->st.timeStat.ok) currentTime = base->rtc.getEpoch();

	for (uint16_t i=0; i<SENSOR_COUNT; i++) {
		SensorType thisType = SENSOR_COUNT;
		thisType = static_cast<SensorType>(i);
		if (base->sensors[thisType].location == BOARD_URBAN) {
			if (base->sensors[thisType].enabled) {
				switch(thisType) {
					case SENSOR_LIGHT: 				if (!sck_bh1721fvc.begin()) return false; break;
					case SENSOR_TEMPERATURE:
					case SENSOR_HUMIDITY: 				if (!sck_sht31.begin()) return false; break;
					case SENSOR_CO:
					case SENSOR_CO_RESISTANCE:
					case SENSOR_NO2:
					case SENSOR_NO2_RESISTANCE:			if (!sck_mics4514.begin(currentTime))	return false; break;
					/* case SENSOR_NOISE_DBA: */
					/* case SENSOR_NOISE_DBC: */
					/* case SENSOR_NOISE_DBZ: 				if (!sck_ics43432.configure()) return false; break; */
					case SENSOR_ALTITUDE:
					case SENSOR_PRESSURE:
					case SENSOR_PRESSURE_TEMP: 			if (!sck_mpl3115A2.begin()) return false; break;		
					case SENSOR_PARTICLE_RED:
					case SENSOR_PARTICLE_GREEN:
					case SENSOR_PARTICLE_IR:
					case SENSOR_PARTICLE_TEMPERATURE: 		if (!sck_max30105.begin()) return false; break;
					default: break;
				}
			} else {
				switch(thisType) {
					case SENSOR_CO:
					case SENSOR_CO_RESISTANCE: 
					case SENSOR_NO2:
					case SENSOR_NO2_RESISTANCE:
						// If all the sensors that requires heating are off
						if (!base->sensors[SENSOR_CO].enabled &&
						    !base->sensors[SENSOR_CO_RESISTANCE].enabled &&
						    !base->sensors[SENSOR_NO2].enabled &&
						    !base->sensors[SENSOR_NO2_RESISTANCE].enabled) {
							uint32_t currentTime = 0;
							if (base->st.timeStat.ok) currentTime = base->rtc.getEpoch();
							sck_mics4514.stop(currentTime);
						}
						break;
					default: break;
				}	
			}
		}
	}
	return true;
};

String SckUrban::getReading(SckBase *base, SensorType wichSensor, bool wait)
{

	switch(wichSensor) {
		case SENSOR_LIGHT:			if (sck_bh1721fvc.get(wait)) return String(sck_bh1721fvc.reading); break;
		case SENSOR_TEMPERATURE: 		if (sck_sht31.update(wait)) return String(sck_sht31.temperature); break;
		case SENSOR_HUMIDITY: 			if (sck_sht31.update(wait)) return String(sck_sht31.humidity); break;
		case SENSOR_CO:
			if (sck_sht31.update(wait)) {
				if (sck_mics4514.getCO(sck_sht31.temperature, sck_sht31.humidity)) return String(sck_mics4514.co);
			}
			break;
		case SENSOR_CO_RESISTANCE: 		if (sck_mics4514.getCOresistance()) return String(sck_mics4514.coResistance); break;
		case SENSOR_CO_HEAT_TIME:
			{
				uint32_t currentTime = 0;
				if (base->st.timeStat.ok) currentTime = base->rtc.getEpoch();
				return String(sck_mics4514.getHeatTime(currentTime)); break;
			}
		case SENSOR_NO2:
			if (sck_sht31.update(wait)) {
				if (sck_mics4514.getNO2(sck_sht31.temperature, sck_sht31.humidity)) return String(sck_mics4514.no2);
			}
			break;
		case SENSOR_NO2_RESISTANCE: 		if (sck_mics4514.getNO2resistance()) return String(sck_mics4514.no2Resistance); break;
		case SENSOR_NO2_HEAT_TIME:
			{
				uint32_t currentTime = 0;
				if (base->st.timeStat.ok) currentTime = base->rtc.getEpoch();
				return String(sck_mics4514.getHeatTime(currentTime)); break;
			}
		case SENSOR_NO2_LOAD_RESISTANCE:	if (sck_mics4514.getNO2load()) return String(sck_mics4514.no2LoadResistor); break;
		/* case SENSOR_NOISE_DBA: 			if (sck_ics43432.bufferFilled()) return String(sck_ics43432.getReading(A_WEIGHTING)); break; */
		/* case SENSOR_NOISE_DBC: 			if (sck_ics43432.bufferFilled()) return String(sck_ics43432.getReading(C_WEIGHTING)); break; */
		/* case SENSOR_NOISE_DBZ: 			if (sck_ics43432.bufferFilled()) return String(sck_ics43432.getReading(Z_WEIGHTING)); break; */
		case SENSOR_ALTITUDE:			if (sck_mpl3115A2.getAltitude(wait)) return String(sck_mpl3115A2.altitude); break;
		case SENSOR_PRESSURE:			if (sck_mpl3115A2.getPressure(wait)) return String(sck_mpl3115A2.pressure); break;
		case SENSOR_PRESSURE_TEMP:		if (sck_mpl3115A2.getTemperature(wait)) return String(sck_mpl3115A2.temperature); break;
		case SENSOR_PARTICLE_RED:		if (sck_max30105.getRed(wait)) return String(sck_max30105.redChann); break;
		case SENSOR_PARTICLE_GREEN:		if (sck_max30105.getGreen(wait)) return String(sck_max30105.greenChann); break;
		case SENSOR_PARTICLE_IR:		if (sck_max30105.getIR(wait)) return String(sck_max30105.IRchann); break;
		case SENSOR_PARTICLE_TEMPERATURE:	if (sck_max30105.getTemperature(wait)) return String(sck_max30105.temperature); break;
		default: break;
	}

	return "none";
}
bool SckUrban::control(SckBase *base, SensorType wichSensor, String command)
{

         switch (wichSensor) {
		case SENSOR_CO:
		case SENSOR_NO2: {
			if (command.startsWith("help")) {
				base->sckOut("Available commands for this sensor:\r\nStill nothing!!"); 
				return true;
			} else base->sckOut("Unrecognized command!! please try again..."); return false;
			break;
		}
		default: break;
        }

        base->sckOut("Sensor not recognized!!");
	return false;
}

// Light
bool Sck_BH1721FVC::begin()
{
	return true;
}
bool Sck_BH1721FVC::stop()
{
	return true;
}
bool Sck_BH1721FVC::get(bool wait)
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
	uint8_t ITIME0  = 0xA0;
	// float TOP = 26500.0; 	 // This is relative to the value above (less resolution more range) TODO define max based on calibration curve (to be implemented)

	// 00h: Start / Stop of measurement is set by special command. (ADC manual integration mode)
	// 01h to FFh: Integration time is determined by ITIME value
	// Integration Time : ITIME_ms = Tint * 964 * (256 - ITIME)
	// Measurement time : Tmt= ITIME_ms + Tint * 714

	// TIME0 posible values, more time = more resolution
	// 0xA0 (~3200 values in 260 ms)
	// 0xB0 (~2100 values in 220 ms)
	// 0xC0 (~1300 values in 180 ms)
	// 0xD0 (~800 values in 130 ms)
	// 0xE0 (~350 values in 88 ms)
	// 0xF0 (~80 values in 45 ms)
	// 0xFA (12 values in 18 ms)
	// 0XFB (8 values in 15 ms)

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

	uint8_t DATA[8] = {CONTROL, ITIME0, INTERRUPT, TH_LOW0, TH_LOW1, TH_UP0, TH_UP1, GAIN};

	// Send Configuration
	Wire.beginTransmission(address);
	Wire.write(0x80);
	for (int i= 0; i<8; i++) Wire.write(DATA[i]);
	Wire.endTransmission();


	// TODO calibration curve
	float Tint = 2.8; 	// From datasheet (2.8 typ -- 4.0 max)
	float ITIME_ms = (Tint * 964 * (256 - ITIME0)) / 1000;
	delay (ITIME_ms+50);

	// Ask for reading
	Wire.beginTransmission(address);
	Wire.write(0x94);
	Wire.endTransmission();
	Wire.requestFrom(address, 4);

	// Get result
	int16_t DATA0 = 0;
	uint16_t DATA1 = 0;
	DATA0 = Wire.read();
	DATA0 = DATA0 | (Wire.read()<<8);
	DATA1 = Wire.read();
	DATA1 = DATA1 | (Wire.read()<<8);

	// Setup gain
	uint8_t Gain = 1;
	if (GAIN == 0x01) Gain = 2;
	else if (GAIN == 0x02) Gain = 64;
	else if (GAIN == 0x03) Gain = 128;

	float Lx = 0;
	if (DATA0 !=0) {
	if (DATA1/DATA0 < 0.26) Lx = (1.290 * DATA0 - 2.733 * DATA1) / Gain * 102.6 / ITIME_ms;
	else if (DATA1/DATA0 < 0.55) Lx = (0.795 * DATA0 - 0.859 * DATA1) / Gain * 102.6 / ITIME_ms;
	else if (DATA1/DATA0 < 1.09) Lx = (0.510 * DATA0 - 0.345 * DATA1) / Gain * 102.6 / ITIME_ms;
	else if (DATA1/DATA0 < 2.13) Lx = (0.276 * DATA0 - 0.130 * DATA1) / Gain * 102.6 / ITIME_ms;
	else Lx = 0;
	}

	Lx = max(0, Lx);
	reading  = Lx;

	return true;
}

// SHT31 (Temperature and Humidity)
// TODO Implement calibration routine
// TODO implement heater controller
Sck_SHT31::Sck_SHT31(TwoWire *localWire)
{
	_Wire = localWire;
}
bool Sck_SHT31::begin()
{
	_Wire->begin();
	_Wire->beginTransmission(address);
	byte error = _Wire->endTransmission();
	if (error != 0) return false;

	delay(1); 		// In case the device was off
	sendComm(SOFT_RESET); 	// Send reset command
	delay(50); 		// Give time to finish reset
	update(true);

	return true;
}
bool Sck_SHT31::stop()
{

	// It will go to idle state by itself after 1ms
	return true;
}
bool Sck_SHT31::update(bool wait)
{
	uint32_t elapsed = millis() - lastTime;
	if (elapsed < timeout) delay(timeout - elapsed);

	uint8_t readbuffer[6];
	sendComm(SINGLE_SHOT_HIGH_REP);

	_Wire->requestFrom(address, (uint8_t)6);
	// Wait for answer (datasheet says 15ms is the max)
	uint32_t started = millis();
	while(_Wire->available() != 6) {
		if (millis() - started > timeout) return 0;
	}

	// Read response
	for (uint8_t i=0; i<6; i++) readbuffer[i] = _Wire->read();

	uint16_t ST, SRH;
	ST = readbuffer[0];
	ST <<= 8;
	ST |= readbuffer[1];

	// Check Temperature crc
	if (readbuffer[2] != crc8(readbuffer, 2)) return false;
	SRH = readbuffer[3];
	SRH <<= 8;
	SRH |= readbuffer[4];

	// check Humidity crc
	if (readbuffer[5] != crc8(readbuffer+3, 2)) return false;
	double temp = ST;
	temp *= 175;
	temp /= 0xffff;
	temp = -45 + temp;
	temperature = (float)temp;

	double shum = SRH;
	shum *= 100;
	shum /= 0xFFFF;
	humidity = (float)shum;

	lastTime = millis();

	return true;
}
void Sck_SHT31::sendComm(uint16_t comm)
{
	_Wire->beginTransmission(address);
	_Wire->write(comm >> 8);
	_Wire->write(comm & 0xFF);
	_Wire->endTransmission();
}
uint8_t Sck_SHT31::crc8(const uint8_t *data, int len)
{

	/* CRC-8 formula from page 14 of SHT spec pdf */

	/* Test data 0xBE, 0xEF should yield 0x92 */

	/* Initialization data 0xFF */
	/* Polynomial 0x31 (x8 + x5 +x4 +1) */
	/* Final XOR 0x00 */

	const uint8_t POLYNOMIAL(0x31);
	uint8_t crc(0xFF);

	for ( int j = len; j; --j ) {
		crc ^= *data++;
		for ( int i = 8; i; --i ) {
			crc = ( crc & 0x80 )
				? (crc << 1) ^ POLYNOMIAL
				: (crc << 1);
		}
	}
	return crc;
}

// Gases
bool Sck_MICS4514::begin(uint32_t startTime)
{
	if (heaterRunning) return true;

	// To protect MICS turn off heaters (HIGH=off, LOW=on)
	pinMode(pinPWM_HEATER_CO, OUTPUT);
	pinMode(pinPWM_HEATER_NO2, OUTPUT);
	digitalWrite(pinPWM_HEATER_CO, HIGH);
	digitalWrite(pinPWM_HEATER_NO2, HIGH);

	pinMode(pinREAD_CO, INPUT);
	pinMode(pinREAD_NO2, INPUT);

	// Put the load resistor in middle position
	setNO2load(8000);

	startHeaterTime = startTime;
	/* return startHeater(); */
	return true;
}
bool Sck_MICS4514::stop(uint32_t stopTime)
{
	if (!heaterRunning) return true;
	heaterRunning = false;
	stopHeaterTime = stopTime;
	startHeaterTime = 0;

	TCC1->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
	while (TCC1->SYNCBUSY.reg & TCC_SYNCBUSY_MASK);

	return true;
}
bool Sck_MICS4514::getCOresistance()
{
	float sensorVoltage;
	sensorVoltage = ((average(pinREAD_CO) * (float)VCC) / (float)ANALOG_RESOLUTION);
	if (sensorVoltage > VCC) sensorVoltage = VCC;
	coResistance = (((VCC - sensorVoltage) / sensorVoltage) * coLoadResistor) / 1000.0;
	return true;
}
bool Sck_MICS4514::getCO(float temperature, float humidity)
{
	if (!calCO.valid) return false;

	getCOresistance();

	co = calCO.A + (calCO.gas.fac1 * pow(coResistance, calCO.gas.ind1)) + (calCO.gas.fac2 * pow(coResistance, calCO.gas.ind2))
	+ (calCO.temp.fac1 * pow(temperature, calCO.temp.ind1)) + (calCO.temp.fac2 * pow(temperature, calCO.temp.ind2))
	+ (calCO.hum.fac1 * pow(humidity, calCO.hum.ind1)) + (calCO.hum.fac2 * pow(humidity, calCO.hum.ind2));

	co = max(0, co);
	return true;
}
bool Sck_MICS4514::getNO2resistance()
{
	float sensorVoltage;
	sensorVoltage = ((average(pinREAD_NO2) * (float)VCC) / (float)ANALOG_RESOLUTION);
	if (sensorVoltage > VCC) sensorVoltage = VCC;
	getNO2load();
	no2Resistance = (((VCC - sensorVoltage) / sensorVoltage) * no2LoadResistor) / 1000.0;
	return true;
}
bool Sck_MICS4514::getNO2(float temperature, float humidity)
{
	if (!calNO2.valid) return false;

	getNO2resistance();

	no2 = calNO2.A + (calNO2.gas.fac1 * pow(no2Resistance, calNO2.gas.ind1)) + (calNO2.gas.fac2 * pow(no2Resistance, calNO2.gas.ind2))
	+ (calNO2.temp.fac1 * pow(temperature, calNO2.temp.ind1)) + (calNO2.temp.fac2 * pow(temperature, calNO2.temp.ind2))
	+ (calNO2.hum.fac1 * pow(humidity, calNO2.hum.ind1)) + (calNO2.hum.fac2 * pow(humidity, calNO2.hum.ind2));

	no2 = no2 * 1000; 	// ppb

	no2 = max(0, no2);
	return true;
}
bool Sck_MICS4514::setNO2load(uint32_t value)
{
	// Check minimal/maximal safe value for Gas sensor (Datasheet says 820 Ohms minimal) ~ 870 because of the rounding of POT discrete steps
	if (value < 870) value = 870;
	else if (value > 10000) value = 10000;

	// Data to be writen (based on datasheet page 59) (http://ww1.microchip.com/downloads/en/DeviceDoc/22107B.pdf)
	// Sending data in MSB 7 bits (with a zero at the end)
	byte data = (int)(value / ohmsPerStep);
	data <<= 1;

	const byte MCP4551_CMD_WRITE = 0b00000000;

	Wire.beginTransmission(POT_NO2_LOAD_ADDRESS);
	Wire.write(MCP4551_CMD_WRITE);
	Wire.write(data);
	return (Wire.endTransmission() == 0);
}
bool Sck_MICS4514::getNO2load()
{
	const byte MCP4551_CMD_READ	= 0b00001100;

	Wire.beginTransmission(POT_NO2_LOAD_ADDRESS);
	Wire.write(MCP4551_CMD_READ);
	Wire.endTransmission();
	Wire.requestFrom(POT_NO2_LOAD_ADDRESS, 2);

	// Wait for answer with a timeout
	uint16_t waitTimeout = 500;
	uint32_t time = millis();
	while (!Wire.available()) if ((millis() - time) > waitTimeout) return false;

	// (Based on datasheet page 61) (http://ww1.microchip.com/downloads/en/DeviceDoc/22107B.pdf)
	// using only MSB 7 bits from second byte
	Wire.read();
	byte buffer = Wire.read();
	buffer >>= 1;

	no2LoadResistor = (uint16_t)(buffer * ohmsPerStep);

	return true;
}
bool Sck_MICS4514::startHeater()
{
	heaterRunning = true;

	// Frequency = GCLK frequency / (2 * N * PER)       where N = prescaler value (CTRLA register)

	// Posible values for N:
	// TCC_CTRLA_PRESCALER_DIV1
	// TCC_CTRLA_PRESCALER_DIV2
	// TCC_CTRLA_PRESCALER_DIV4
	// TCC_CTRLA_PRESCALER_DIV8
	// TCC_CTRLA_PRESCALER_DIV16
	// TCC_CTRLA_PRESCALER_DIV64
	// TCC_CTRLA_PRESCALER_DIV256
	// TCC_CTRLA_PRESCALER_DIV1024
	#define MY_DIVIDER TCC_CTRLA_PRESCALER_DIV1

 	// With N = 1
	// FOR 12 bits
	// Frequency = 48MHz / (2 * 1 * 4096) = 5,859.375 ~ 5.9 kHz
	// Resolution at 5.9kHz = log(4096 + 1) / log(2) = 12 bits
	//
	// FOR 10 bits
	// Frequency = 48MHz / (2 * 1 * 1024) = 23,437.5 ~ 23.4 kHz
	// Resolution at 23.4kHz = log(1024 + 1) / log(2) = 10 bits
	//
	// FOR 8 bits
	// Frequency = 48MHz / (2 * 1 * 256) = 93,750 ~ 93.7 kHz
	// Resolution at 93.7kHz = log(256 + 1) / log(2) = 8 bits
	//
	// FOR 6 bits
	// Frequency = 48MHz / (2 * 1 * 96) = 250 kHz
	// Resolution at 250kHz = log(96 + 1) / log(2) = 6.6 bits

	uint8_t resolution = 10;					// Resolution in bits
	uint16_t maxValue = pow(2, resolution);

	REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) |          // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
					  GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
	while (GCLK->STATUS.bit.SYNCBUSY);

	REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
					   GCLK_GENCTRL_GENEN |         // Enable GCLK4
					   GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
					   GCLK_GENCTRL_ID(4);          // Select GCLK4
	while (GCLK->STATUS.bit.SYNCBUSY);

	// Enable the port multiplexer for the digital pin
	PORT->Group[PORTA].PINCFG[8].bit.PMUXEN = 1;
	PORT->Group[PORTA].PINCFG[9].bit.PMUXEN = 1;

	// Connect the TCC0 timer to pin PA09 - port pins are paired odd PMUO and even PMUXE
	// F & E specify the timers: TCC0, TCC1 and TCC2
	PORT->Group[PORTA].PMUX[8 >> 1].reg |= PORT_PMUX_PMUXE_F;
	PORT->Group[PORTA].PMUX[9 >> 1].reg |= PORT_PMUX_PMUXO_F;

	// Feed GCLK4 to TCC0 and TCC1
	REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TCC0 and TCC1
					   GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
					   GCLK_CLKCTRL_ID_TCC0_TCC1;   // Feed GCLK4 to TCC0 and TCC1
	while (GCLK->STATUS.bit.SYNCBUSY);

	//Set for Single slope PWM operation: timers or counters count up to TOP value and then repeat
	REG_TCC1_WAVE |=  TCC_WAVE_WAVEGEN_NPWM;
	while (TCC1->SYNCBUSY.bit.WAVE);

	// Each timer counts up to a maximum or TOP value set by the PER register,
	// this determines the frequency of the PWM operation:
	REG_TCC1_PER = maxValue;         		// Set the frequency of the PWM on TCC0
	while (TCC1->SYNCBUSY.bit.PER);

	REG_TCC1_CC0 = (uint16_t)(maxValue * (dutyCycle_CO / 100.0)); 	// CO
	REG_TCC1_CC1 = (uint16_t)(maxValue * (dutyCycle_NO2 / 100.0)); 	// NO2
	while (TCC1->SYNCBUSY.bit.CC1);

	// Divide the 48MHz signal by 1 giving 48MHz (20.83ns) TCC0 timer tick and enable the outputs
	REG_TCC1_CTRLA |= MY_DIVIDER |    // Divide GCLK4 (posibles values: 1,2,4,8,16,64,256,1024)
					  TCC_CTRLA_ENABLE;             // Enable the TCC0 output
	while (TCC1->SYNCBUSY.bit.ENABLE);
	return true;
}
uint32_t Sck_MICS4514::getHeatTime(uint32_t currentTime)
{
	if (!heaterRunning) return 0;
	if (startHeaterTime == 0) startHeaterTime = currentTime;
	return currentTime - startHeaterTime;
}
float Sck_MICS4514::average(uint8_t wichPin)
{

	uint16_t numReadings = 500;
	long total = 0;
	for(uint16_t i=0; i<numReadings; i++) {
		total = total + analogRead(wichPin);
	}
	float average = (float)total / numReadings;
	return average;
}

// Noise


// Barometric pressure and Altitude
bool Sck_MPL3115A2::begin()
{

	if (Adafruit_mpl3115A2.begin()) return true;
	return false;
}
bool Sck_MPL3115A2::stop()
{

	return true;
}
bool Sck_MPL3115A2::getAltitude(bool wait)
{

	Adafruit_mpl3115A2.begin();

	// TODO callibration with control interface
	// Maybe we could implement get online data to calibrate this
	// mpl3115A2.setSeaPressure(102250.0);

	// TODO timeout to prevent hangs on external lib
	altitude = Adafruit_mpl3115A2.getAltitude();

	return true;
}
bool Sck_MPL3115A2::getPressure(bool wait)
{

	Adafruit_mpl3115A2.begin();

	// TODO timeout to prevent hangs on external lib
	pressure = Adafruit_mpl3115A2.getPressure() / 1000;

	return true;
}
bool Sck_MPL3115A2::getTemperature(bool wait)
{

	Adafruit_mpl3115A2.begin();

	// TODO timeout to prevent hangs on external lib
	altitude = Adafruit_mpl3115A2.getAltitude();
	temperature =  Adafruit_mpl3115A2.getTemperature();	// Only works after a getAltitude! don't call this allone

	return true;
}

// Dust Particles
bool Sck_MAX30105::begin()
{

	if (sparkfun_max30105.begin()) return true;
	return false;
}
bool Sck_MAX30105::stop()
{

	sparkfun_max30105.shutDown();
	return true;
}
bool Sck_MAX30105::getRed(bool wait)
{

	sparkfun_max30105.wakeUp();

	// TODO Dig more into setup parameters
	sparkfun_max30105.setup();
	redChann = (float)sparkfun_max30105.getRed();
	sparkfun_max30105.shutDown();
	return true;
}
bool Sck_MAX30105::getGreen(bool wait)
{

	sparkfun_max30105.wakeUp();

	// TODO Dig more into setup parameters
	sparkfun_max30105.setup();
	greenChann = (float)sparkfun_max30105.getGreen();
	sparkfun_max30105.shutDown();
	return true;
}
bool Sck_MAX30105::getIR(bool wait)
{

	sparkfun_max30105.wakeUp();

	// TODO Dig more into setup parameters
	sparkfun_max30105.setup();
	IRchann = (float)sparkfun_max30105.getIR();
	sparkfun_max30105.shutDown();
	return true;
}
bool Sck_MAX30105::getTemperature(bool wait)
{	// NOT WORKING!!! (sparkfun lib)

	sparkfun_max30105.wakeUp();

	// TODO Dig more into setup parameters
	sparkfun_max30105.setup(0);
	temperature = (float)sparkfun_max30105.readTemperature();
	sparkfun_max30105.shutDown();
	return true;
}

