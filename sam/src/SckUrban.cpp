#include "SckUrban.h"
#include "SckBase.h"

// Hardware Serial UART PM
Uart SerialPM (&sercom5, pinPM_SERIAL_RX, pinPM_SERIAL_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);
void SERCOM5_Handler() {
	SerialPM.IrqHandler();
}

bool SckUrban::setup()
{
	if (!sck_bh1721fvc.start()) return false;
	if (!sck_sht31.start()) return false;
	if (!sck_noise.start()) return false;
	if (!sck_mpl3115A2.start()) return false;
	if (!sck_ccs811.start()) return false;
	sck_pm.start(); // This sensor is independent of the urban board. That's way we don't declare error if it's not there.

	return true;
};
bool SckUrban::start(SensorType wichSensor)
{
	switch(wichSensor) {
		case SENSOR_LIGHT: 				if (sck_bh1721fvc.start()) return true; break;
		case SENSOR_TEMPERATURE:
		case SENSOR_HUMIDITY: 				if (sck_sht31.start()) return true; break;
		case SENSOR_NOISE_DBA:
		case SENSOR_NOISE_DBC:
		case SENSOR_NOISE_DBZ:
		case SENSOR_NOISE_FFT: 				if (sck_noise.start()) return true; break;
		case SENSOR_ALTITUDE:
		case SENSOR_PRESSURE:
		case SENSOR_PRESSURE_TEMP: 			if (sck_mpl3115A2.start()) return true; break;
		case SENSOR_CCS811_VOCS:			return sck_ccs811.start(); break;
		case SENSOR_CCS811_ECO2: 			return sck_ccs811.start(); break;
		case SENSOR_PM_1:
		case SENSOR_PM_25:
		case SENSOR_PM_10:
		case SENSOR_PN_03:
		case SENSOR_PN_05:
		case SENSOR_PN_1:
		case SENSOR_PN_25:
		case SENSOR_PN_5:
		case SENSOR_PN_10: 				if (sck_pm.start()) return true; break;
		default: break;
	}

	return false;
}
bool SckUrban::stop(SensorType wichSensor)
{
	switch(wichSensor) {
		case SENSOR_LIGHT: 				if (sck_bh1721fvc.stop()) return true; break;
		case SENSOR_TEMPERATURE:
		case SENSOR_HUMIDITY: 				if (sck_sht31.stop()) return true; break;
		case SENSOR_NOISE_DBA:
		case SENSOR_NOISE_DBC:
		case SENSOR_NOISE_DBZ:
		case SENSOR_NOISE_FFT: 				if (sck_noise.stop()) return true;
		case SENSOR_ALTITUDE:
		case SENSOR_PRESSURE:
		case SENSOR_PRESSURE_TEMP: 			if (sck_mpl3115A2.stop()) return true; break;
		case SENSOR_CCS811_VOCS:			return sck_ccs811.stop(); break;
		case SENSOR_CCS811_ECO2: 			return sck_ccs811.stop(); break;
		case SENSOR_PM_1:
		case SENSOR_PM_25:
		case SENSOR_PM_10:
		case SENSOR_PN_03:
		case SENSOR_PN_05:
		case SENSOR_PN_1:
		case SENSOR_PN_25:
		case SENSOR_PN_5:
		case SENSOR_PN_10: 				if (sck_pm.stop()) return true; break;
		default: break;
	}

	return false;
}

String SckUrban::getReading(SckBase *base, SensorType wichSensor)
{

	switch(wichSensor) {
		case SENSOR_LIGHT:			if (sck_bh1721fvc.get()) return String(sck_bh1721fvc.reading); break;
		case SENSOR_TEMPERATURE: 		if (sck_sht31.update()) return String(sck_sht31.temperature); break;
		case SENSOR_HUMIDITY: 			if (sck_sht31.update()) return String(sck_sht31.humidity); break;
		case SENSOR_NOISE_DBA: 			if (sck_noise.getReading(SENSOR_NOISE_DBA)) return String(sck_noise.readingDB); break;
		case SENSOR_NOISE_DBC: 			if (sck_noise.getReading(SENSOR_NOISE_DBC)) return String(sck_noise.readingDB); break;
		case SENSOR_NOISE_DBZ: 			if (sck_noise.getReading(SENSOR_NOISE_DBZ)) return String(sck_noise.readingDB); break;
		case SENSOR_NOISE_FFT: 			if (sck_noise.getReading(SENSOR_NOISE_FFT)) {
								// TODO find a way to give access to readingsFFT instead of storing them on a String (too much RAM)
								// For now it just prints the values to console
								for (uint16_t i=1; i<sck_noise.FFT_NUM; i++) SerialUSB.println(sck_noise.readingFFT[i]);
								return "Look above!";
								break;
							}
		case SENSOR_ALTITUDE:			if (sck_mpl3115A2.getAltitude()) return String(sck_mpl3115A2.altitude); break;
		case SENSOR_PRESSURE:			if (sck_mpl3115A2.getPressure()) return String(sck_mpl3115A2.pressure); break;
		case SENSOR_PRESSURE_TEMP:		if (sck_mpl3115A2.getTemperature()) return String(sck_mpl3115A2.temperature); break;
		case SENSOR_CCS811_VOCS:		if (sck_ccs811.getReading(base)) return String(sck_ccs811.VOCgas); break;
		case SENSOR_CCS811_ECO2:		if (sck_ccs811.getReading(base)) return String(sck_ccs811.ECO2gas); break;
		case SENSOR_PM_1: 			if (sck_pm.update()) return String(sck_pm.pm1); break;
		case SENSOR_PM_25: 			if (sck_pm.update()) return String(sck_pm.pm25); break;
		case SENSOR_PM_10: 			if (sck_pm.update()) return String(sck_pm.pm10); break;
		case SENSOR_PN_03: 			if (sck_pm.update()) return String(sck_pm.pn03); break;
		case SENSOR_PN_05: 			if (sck_pm.update()) return String(sck_pm.pn05); break;
		case SENSOR_PN_1: 			if (sck_pm.update()) return String(sck_pm.pn1); break;
		case SENSOR_PN_25: 			if (sck_pm.update()) return String(sck_pm.pn25); break;
		case SENSOR_PN_5: 			if (sck_pm.update()) return String(sck_pm.pn5); break;
		case SENSOR_PN_10: 			if (sck_pm.update()) return String(sck_pm.pn10); break;
		default: break;
	}

	return "null";
}
bool SckUrban::control(SckBase *base, SensorType wichSensor, String command)
{

         switch (wichSensor) {
		case SENSOR_NOISE_DBA:
		case SENSOR_NOISE_DBC:
		case SENSOR_NOISE_DBZ:
		case SENSOR_NOISE_FFT: {
			if (command.startsWith("debug")) {
				sck_noise.debugFlag = !sck_noise.debugFlag;
				sprintf(base->outBuff, "Noise debug: %s", sck_noise.debugFlag  ? "true" : "false");
				base->sckOut();
				return true;
			}		       
		} case SENSOR_CCS811_VOCS:
		case SENSOR_CCS811_ECO2: {
			
			if (command.startsWith("compensate")) {
				sck_ccs811.compensate = !sck_ccs811.compensate;
				return (sck_ccs811.compensate ? "True" : "False");
			}
		}
		default: break;
        }

        base->sckOut("Sensor not recognized!!");
	return false;
}

// Light
bool Sck_BH1721FVC::start()
{
	if (!I2Cdetect(&Wire, address)) return false;
	return true;
}
bool Sck_BH1721FVC::stop()
{
	return true;
}
bool Sck_BH1721FVC::get()
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
bool Sck_SHT31::start()
{
	_Wire->begin();
	_Wire->beginTransmission(address);
	byte error = _Wire->endTransmission();
	if (error != 0) return false;

	delay(1); 		// In case the device was off
	sendComm(SOFT_RESET); 	// Send reset command
	delay(50); 		// Give time to finish reset
	update();

	return true;
}
bool Sck_SHT31::stop()
{

	// It will go to idle state by itself after 1ms
	return true;
}
bool Sck_SHT31::update()
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

// Noise
bool Sck_Noise::start()
{
	if (alreadyStarted) return true;

	REG_GCLK_GENCTRL = GCLK_GENCTRL_ID(4);  // Select GCLK4
	while (GCLK->STATUS.bit.SYNCBUSY);

	alreadyStarted = true;
	return true;
}
bool Sck_Noise::stop()
{
	return true;
}
bool Sck_Noise::getReading(SensorType wichSensor)
{
	if (!I2S.begin(I2S_PHILIPS_MODE, sampleRate, 32)) return false;

	// Wait 263000 I2s cycles or 85 ms at 441000 hz
	uint32_t startPoint = millis();
	while (millis() - startPoint < 100) I2S.read();

	// Fill buffer with samples from I2S bus
	int32_t source[SAMPLE_NUM];
	uint16_t bufferIndex = 0;

	startPoint = millis();
	uint8_t timeOut = 30; 	// (ms) Timeout to avoid hangs if the I2S is not responfing
	while (bufferIndex < SAMPLE_NUM) {
		int32_t buff = I2S.read();
		if (buff) {
			source[bufferIndex] = buff>>7;
			bufferIndex ++;
		}

		if (millis() - startPoint > timeOut) {
			I2S.end();
			return false;
		}
	}
	I2S.end();

	// Get de average of recorded samples
	int32_t sum = 0;
	for (uint16_t i=0; i<SAMPLE_NUM; i++) sum += source[i];
	int32_t avg = sum / SAMPLE_NUM;

	// Center samples in zero
	for (uint16_t i=0; i<SAMPLE_NUM; i++) source[i] = source[i] - avg;

	// FFT
	FFT(source);

	switch(wichSensor) {

		case SENSOR_NOISE_DBA:
			// Equalization and A weighting
			for (uint16_t i=0; i<FFT_NUM; i++) readingFFT[i] *= (double)(equalWeight_A[i] / 65536.0);
			break;
		case SENSOR_NOISE_DBC:
			// Equlization and C weighting
			for (uint16_t i=0; i<FFT_NUM; i++) readingFFT[i] *= (double)(equalWeight_C[i] / 65536.0);
			break;
		case SENSOR_NOISE_DBZ:
			// Just Equalization
			for (uint16_t i=0; i<FFT_NUM; i++) readingFFT[i] *= (double)(equalTab[i] / 65536.0);
			break;
		case SENSOR_NOISE_FFT:
			// Convert FFT to dB
			fft2db();
			return true;
			break;
		default: break;
	}

	// RMS
	long long rmsSum = 0;
	double rmsOut = 0;
	for (uint16_t i=0; i<FFT_NUM; i++) rmsSum += pow(readingFFT[i], 2) / FFT_NUM;
	rmsOut = sqrt(rmsSum);
	rmsOut = rmsOut * 1 / RMS_HANN * sqrt(FFT_NUM) / sqrt(2);

	// Convert to dB
	readingDB = (double) (FULL_SCALE_DBSPL - (FULL_SCALE_DBFS - (20 * log10(rmsOut * sqrt(2)))));

	if (debugFlag) {
		SerialUSB.println("samples, FFT_weighted");
		for (uint16_t i=0; i<SAMPLE_NUM; i++) {
			SerialUSB.print(source[i]);
			SerialUSB.print(",");
			if (i < 256) SerialUSB.println(readingFFT[i]);
			else SerialUSB.println();
		}
	}

	return true;
}
bool Sck_Noise::FFT(int32_t *source)
{
	int16_t scaledSource[SAMPLE_NUM];
	double divider = dynamicScale(source, scaledSource);

	applyWindow(scaledSource, hannWindow, SAMPLE_NUM);

	static int16_t ALIGN4 scratchData[SAMPLE_NUM * 2];

	// Split the data
	for(int i=0; i<SAMPLE_NUM*2; i+=2){
		scratchData[i] = scaledSource[i/2]; // Real
		scratchData[i+1] = 0; // Imaginary
	}

	arm_radix2_butterfly(scratchData, (int16_t)SAMPLE_NUM, (int16_t *)twiddleCoefQ15_512);
	arm_bitreversal(scratchData, SAMPLE_NUM, (uint16_t *)armBitRevTable8);

	for (int i=0; i<SAMPLE_NUM/2; i++) {

		// Calculate result and normalize SpectrumBuffer, also revert dynamic scaling
		uint32_t myReal = pow(scratchData[i*2], 2);
		uint32_t myImg = pow(scratchData[(i*2)+1], 2);

		readingFFT[i] = sqrt(myReal + myImg) * divider * 4;
	}

	// Exception for the first bin
	readingFFT[0] = readingFFT[0] / 2;

	return 0;
}
double Sck_Noise::dynamicScale(int32_t *source, int16_t *scaledSource)
{
	int32_t maxLevel = 0;
	for (uint16_t i=0; i<SAMPLE_NUM; i++) if (abs(source[i]) > maxLevel) maxLevel = abs(source[i]);
	double divider = (maxLevel+1) / 32768.0; // 16 bits
	if (divider < 1) divider = 1;

	for (uint16_t i=0; i<SAMPLE_NUM; i++) scaledSource[i] = source[i] / divider;

	return divider;
}
void Sck_Noise::applyWindow(int16_t *src, const uint16_t *window, uint16_t len)
{
	/* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
		-------
		This is an FFT library for ARM cortex M0+ CPUs
		Adafruit invests time and resources providing this open source code,
		please support Adafruit and open-source hardware by purchasing products from Adafruit!
		Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
		------
	*/

	while(len--){
		int32_t val = *src * *window++;
		*src = val >> 15;
		src++;
	}
}
void Sck_Noise::arm_radix2_butterfly(int16_t * pSrc, int16_t fftLen, int16_t * pCoef)
{
	/* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
		-------
		This is an FFT library for ARM cortex M0+ CPUs
		Adafruit invests time and resources providing this open source code,
		please support Adafruit and open-source hardware by purchasing products from Adafruit!
		Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
		------
	*/

	int i, j, k, l;
	int n1, n2, ia;
	int16_t xt, yt, cosVal, sinVal;

	n2 = fftLen;

	n1 = n2;
	n2 = n2 >> 1;
	ia = 0;

	// loop for groups
	for (j=0; j<n2; j++) {
		cosVal = pCoef[ia * 2];
		sinVal = pCoef[(ia * 2) + 1];
		ia++;

		// loop for butterfly
		for (i=j; i<fftLen; i+=n1) {
			l = i + n2;
			xt = (pSrc[2 * i] >> 2u) - (pSrc[2 * l] >> 2u);
			pSrc[2 * i] = ((pSrc[2 * i] >> 2u) + (pSrc[2 * l] >> 2u)) >> 1u;

			yt = (pSrc[2 * i + 1] >> 2u) - (pSrc[2 * l + 1] >> 2u);
			pSrc[2 * i + 1] =
				((pSrc[2 * l + 1] >> 2u) + (pSrc[2 * i + 1] >> 2u)) >> 1u;

			pSrc[2u * l] = (((int16_t) (((int32_t) xt * cosVal) >> 16)) +
					((int16_t) (((int32_t) yt * sinVal) >> 16)));

			pSrc[2u * l + 1u] = (((int16_t) (((int32_t) yt * cosVal) >> 16)) -
					((int16_t) (((int32_t) xt * sinVal) >> 16)));

		}                           // butterfly loop end
	}                             // groups loop end

	uint16_t twidCoefModifier = 2;

	// loop for stage
	for (k = fftLen / 2; k > 2; k = k >> 1) {
		n1 = n2;
		n2 = n2 >> 1;
		ia = 0;

		// loop for groups
		for (j=0; j<n2; j++) {
			cosVal = pCoef[ia * 2];
			sinVal = pCoef[(ia * 2) + 1];

			ia = ia + twidCoefModifier;

			// loop for butterfly
			for (i=j; i<fftLen; i+=n1) {
				l = i + n2;
				xt = pSrc[2 * i] - pSrc[2 * l];
				pSrc[2 * i] = (pSrc[2 * i] + pSrc[2 * l]) >> 1u;

				yt = pSrc[2 * i + 1] - pSrc[2 * l + 1];
				pSrc[2 * i + 1] = (pSrc[2 * l + 1] + pSrc[2 * i + 1]) >> 1u;

				pSrc[2u * l] = (((int16_t) (((int32_t) xt * cosVal) >> 16)) +
						((int16_t) (((int32_t) yt * sinVal) >> 16)));

				pSrc[2u * l + 1u] = (((int16_t) (((int32_t) yt * cosVal) >> 16)) -
						((int16_t) (((int32_t) xt * sinVal) >> 16)));

			}                         // butterfly loop end
		}                           // groups loop end
		twidCoefModifier = twidCoefModifier << 1u;
	}                             // stages loop end

	n1 = n2;
	n2 = n2 >> 1;
	ia = 0;
	// loop for groups
	for (j=0; j<n2; j++) {
		cosVal = pCoef[ia * 2];
		sinVal = pCoef[(ia * 2) + 1];

		ia = ia + twidCoefModifier;

		// loop for butterfly
		for (i=j; i<fftLen; i+=n1) {
			l = i + n2;
			xt = pSrc[2 * i] - pSrc[2 * l];
			pSrc[2 * i] = (pSrc[2 * i] + pSrc[2 * l]);

			yt = pSrc[2 * i + 1] - pSrc[2 * l + 1];
			pSrc[2 * i + 1] = (pSrc[2 * l + 1] + pSrc[2 * i + 1]);

			pSrc[2u * l] = xt;

			pSrc[2u * l + 1u] = yt;

		}                           // butterfly loop end
	}                             // groups loop end
}
void Sck_Noise::arm_bitreversal(int16_t * pSrc16, uint32_t fftLen, uint16_t * pBitRevTab)
{
	/* This code is from https://github.com/adafruit/Adafruit_ZeroFFT thank you!
		-------
		This is an FFT library for ARM cortex M0+ CPUs
		Adafruit invests time and resources providing this open source code,
		please support Adafruit and open-source hardware by purchasing products from Adafruit!
		Written by Dean Miller for Adafruit Industries. MIT license, all text above must be included in any redistribution
		------
	*/

	int32_t *pSrc = (int32_t *) pSrc16;
	int32_t in;
	uint32_t fftLenBy2, fftLenBy2p1;
	uint32_t i, j;

	/*  Initializations */
	j = 0u;
	fftLenBy2 = fftLen / 2u;
	fftLenBy2p1 = (fftLen / 2u) + 1u;

	/* Bit Reversal Implementation */
	for (i = 0u; i <= (fftLenBy2 - 2u); i += 2u) {
		if(i < j) {
			in = pSrc[i];
			pSrc[i] = pSrc[j];
			pSrc[j] = in;

			in = pSrc[i + fftLenBy2p1];
			pSrc[i + fftLenBy2p1] = pSrc[j + fftLenBy2p1];
			pSrc[j + fftLenBy2p1] = in;
		}

		in = pSrc[i + 1u];
		pSrc[i + 1u] = pSrc[j + fftLenBy2];
		pSrc[j + fftLenBy2] = in;

		/*  Reading the index for the bit reversal */
		j = *pBitRevTab;

		/*  Updating the bit reversal index depending on the fft length  */
		pBitRevTab++;
	}
}
void Sck_Noise::fft2db()
{
    for (uint16_t i=0; i<FFT_NUM; i++) {
	    if (readingFFT[i] > 0) readingFFT[i] = FULL_SCALE_DBSPL - (FULL_SCALE_DBFS - (20 * log10(readingFFT[i] * sqrt(2))));
	    if (readingFFT[i] < 0) readingFFT[i] = 0;
    }
}

// Barometric pressure and Altitude
bool Sck_MPL3115A2::start()
{
	if (!I2Cdetect(&Wire, address)) return false;
	if (Adafruit_mpl3115A2.begin()) return true;
	return false;
}
bool Sck_MPL3115A2::stop()
{

	return true;
}
bool Sck_MPL3115A2::getAltitude()
{

	Adafruit_mpl3115A2.begin();

	// TODO callibration with control interface
	// Maybe we could implement get online data to calibrate this
	// mpl3115A2.setSeaPressure(102250.0);

	// TODO timeout to prevent hangs on external lib
	altitude = Adafruit_mpl3115A2.getAltitude();

	return true;
}
bool Sck_MPL3115A2::getPressure()
{

	Adafruit_mpl3115A2.begin();

	// TODO timeout to prevent hangs on external lib
	pressure = Adafruit_mpl3115A2.getPressure() / 1000;

	return true;
}
bool Sck_MPL3115A2::getTemperature()
{

	Adafruit_mpl3115A2.begin();

	// TODO timeout to prevent hangs on external lib
	altitude = Adafruit_mpl3115A2.getAltitude();
	temperature =  Adafruit_mpl3115A2.getTemperature();	// Only works after a getAltitude! don't call this allone

	return true;
}

// PM sensor
bool Sck_PM::start()
{
	if (started) return true;
	if (detectionFailed) return false;

	pinMode(pinPM_ENABLE, OUTPUT);
	digitalWrite(pinPM_ENABLE, HIGH);
	SerialPM.begin(9600);

	uint32_t startTimer = millis();
	while (millis() - startTimer < 4000) {
		delay(50);
		if (SerialPM.available()) {
			started = true;
			return true;
		}
	}
	stop();
	detectionFailed = true;
	return false;
}
bool Sck_PM::stop()
{
	digitalWrite(pinPM_ENABLE, LOW);
	SerialPM.end();
	started = false;
	detectionFailed = false;

	return true;
}
bool Sck_PM::update()
{
	if (millis() - lastReading < 1000) return true; 	// PM sensor only delivers one reading per second
	if (millis() - lastFail < 1000) return false; 		// We need at least one second after las fail

	// Empty serial buffer
	while(SerialPM.available()) SerialPM.read();

	// Wait for new readings
	uint32_t startPoint = millis();
	while(SerialPM.available() < (buffLong + 2)) {
		if (millis() - startPoint > 1500) {
			// Timeout
			lastFail = millis();

			// After 10 seconds declare the PM innactive
			if (millis() - lastReading < 10000) {
				active = false;
			}
			return false;
		}
	}


	uint16_t sum = 0;

	// Search for start char 1
	byte sc1 = 0;
	startPoint = millis();
	while (sc1 != 0x42) {
		sc1 = SerialPM.read();
		if (millis() - startPoint > 1500) return false;
	}
	sum += sc1;

	// Confirm we receive start char 2
	byte sc2 = 0;
	sc2 = SerialPM.read();

	if (sc2 == 0x4d) {

		sum += sc2;

		unsigned char buff[buffLong];
		byte howMany =  SerialPM.readBytes(buff, buffLong);

		// Is buffer complete?
		if (howMany < 30) {
			return false;
		}

		// Checksum
		uint16_t checkSum = (buff[28]<<8) + buff[29];
		for(int i=0; i<(buffLong - 2); i++) sum += buff[i];
		if(sum != checkSum) {
			return false;
		}

		// Get the values
		pm1 = (buff[2]<<8) + buff[3];
		pm25 = (buff[4]<<8) + buff[5];
		pm10 = (buff[6]<<8) + buff[7];
		pn03 = (buff[14]<<8) + buff[15];
		pn05 = (buff[16]<<8) + buff[17];
		pn1 = (buff[18]<<8) + buff[19];
		pn25 = (buff[20]<<8) + buff[21];
		pn5 = (buff[22]<<8) + buff[23];
		pn10 = (buff[24]<<8) + buff[25];

		lastReading = millis();
		lastFail = 0;
		active = true;

		return true;
	}
	return false;
}
bool Sck_PM::reset()
{
	digitalWrite(pinPM_ENABLE, LOW);
	delay(200);
	digitalWrite(pinPM_ENABLE, HIGH);
	return true;
}

// VOC and ECO2 
bool Sck_CCS811::start()
{
	if (alreadyStarted) return true;

	if (ccs.begin() != CCS811Core::SENSOR_SUCCESS) return false;

	if (ccs.setDriveMode(driveMode != CCS811Core::SENSOR_SUCCESS)) return false;

	startTime = millis();
	alreadyStarted = true;
	return true;
}
bool Sck_CCS811::stop()
{
	alreadyStarted = false;
	startTime = 0;
	return true;
}
bool Sck_CCS811::getReading(SckBase *base)
{
	if (!alreadyStarted) start();
	if (((startTime == 0) || ((millis() - startTime) < warmingTime)) && !base->inTest) return false;
	if (millis() - lastReadingMill < 1000) return true;
	lastReadingMill = millis();

	if (!ccs.dataAvailable()) return false;

	ccs.readAlgorithmResults();

	VOCgas = ccs.getTVOC();
	ECO2gas = ccs.getCO2();

	if (compensate) {
		if (base->sensors[SENSOR_TEMPERATURE].enabled && base->sensors[SENSOR_HUMIDITY].enabled) {
			if (base->getReading(SENSOR_HUMIDITY) && base->getReading(SENSOR_TEMPERATURE)) {
				ccs.setEnvironmentalData(base->sensors[SENSOR_HUMIDITY].reading.toFloat(), base->sensors[SENSOR_TEMPERATURE].reading.toFloat());
			} 
		}
	}
	return true;
}
uint16_t Sck_CCS811::getBaseline()
{
	if (!alreadyStarted) {
		if (!start()) return false;
	}
	return ccs.getBaseline();
}
bool Sck_CCS811::setBaseline(uint16_t wichBaseline)
{
	if (!alreadyStarted) {
		if (!start()) return false;
	}
	if (ccs.setBaseline(wichBaseline) != ccs.SENSOR_SUCCESS); return false;
	return true;
}


