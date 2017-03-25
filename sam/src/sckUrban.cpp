#include "sckUrban.h"

uint8_t pot_6_db_preset[] = {0,  7, 26, 96, 255};
uint8_t pot_7_db_preset[] = {0, 17, 97, 255, 255};

void SckUrban::setup() {

	pinMode(IO0, OUTPUT);
	pinMode(IO1, OUTPUT);

	digitalWrite(IO0, LOW); 		// Turn off CO Sensor Heather
	digitalWrite(IO1, LOW); 		// Turn off NO2 Sensor Heater
	
	ADCini();

	ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV16;       // clock prescaler to 16
	ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1024 |   // 1024 samples for averaging
											 ADC_AVGCTRL_ADJRES(0x4ul);   // Adjusting result by 4
	gainChange(0);
};

float SckUrban::getReading(SensorType wichSensor) {
	switch (wichSensor) {
		case SENSOR_NOISE: return getNoise(); break;
		case SENSOR_HUMIDITY: return getHumidity(); break;
		case SENSOR_TEMPERATURE: return getTemperature(); break;
		case SENSOR_LIGHT: return getLight(); break;
		case SENSOR_CO: return getCO(); break;
		case SENSOR_NO2: return getNO2(); break;
	}
}

// Noise sensor
float SckUrban::getNoise() {
	uint16_t soundraw = 0;
	// uint8_t section = 0;
	boolean validReading = 0;

	while (!validReading) {
		switch (gain_step) {
			case 0:
				soundraw = analogRead(S4);
				if (soundraw <= 600)                      // primer intervalo 0~66, gain=max=76dB
				{
					sounddB = 12.5f * log10f(getsound()) + 33;
					validReading = 1;
				}
				else gainChange(1);
			break;
			case 1:
				soundraw = analogRead(S4);
				if ((soundraw >= 15) & (soundraw <= 400)) // segundo intervalo 66~85, gain=34dB
				{
					sounddB = 12.5f * log10f(getsound()) + 53.5;
					validReading = 1;
				}
				else if (soundraw < 15) gainChange(0);
				else if (soundraw > 400) gainChange(2);
			break;
			case 2:
				soundraw = analogRead(S4);
				if ((soundraw >= 20) & (soundraw <= 120)) // tercer intervalo 85~95, gain=17dB
				{
					sounddB = 12.5f * log10f(getsound()) + 69.5;
					validReading = 1;
				}
				else if (soundraw < 20) gainChange(1);
				else if (soundraw > 120) gainChange(3);
			break;
			case 3:
				soundraw = analogRead(S4);
				if ((soundraw >= 20) & (soundraw <= 120)) // cuarto intervalo 95~105, gain=8dB
				{
					sounddB = 12.5f * log10f(getsound()) + 79;
					validReading = 1;
				}
				else if (soundraw < 20) gainChange(2);
				else if (soundraw > 120) gainChange(4);
			break;
			case 4:
				soundraw = analogRead(S4);
				if ((soundraw >= 82) & (soundraw <= 400)) // quinto intervalo 105~final, gain=min=5dB
				{
					sounddB = 13.5f * log10f(getsound()) + 79.5;
					validReading = 1;
				}
				else if (soundraw < 82) gainChange(3);
				else if (soundraw > 400) {
					sounddB = 13.5f * log10f(getsound()) + 79.5; // espacio no comprobado
					validReading = 1;
				}
			break;
		}
	}
	return sounddB;
}

void SckUrban::gainChange(uint8_t value) {
	writeResistorRaw(6, pot_6_db_preset[value]);
	//delay(20);
	writeResistorRaw(7, pot_7_db_preset[value]);
	if (gain_step < value) delay(250); // la carga del condensador que filtra la señal rectificada es inmediata, la descarga no
															// por lo que si aumentamos la ganancia el voltaje sera mayor y no hemos de esperar tanto, si
															// disminuimos la ganancia hemos de esperar mas a que el condensador se descargue
															// value = 0 G = 76dB
															// value = 1 G = 34dB
															// value = 2 G = 17dB
															// value = 3 G = 8dB
															// value = 4 G = 5.8dB
	gain_step = value;
}

float SckUrban::getsound() {
		return ((float)(analogRead(S4)) + 1) / RESOLUTION_ANALOG * VCC;
}

void SckUrban::writeResistorRaw(byte resistor, int value) {

	 if (value>255) value = 0;
	 else if (value<0) value = 255;

	 byte POT = POT1;
	 byte ADDR = resistor;

	 if ((resistor==6)||(resistor==7))
		 {
			 POT = POT4;
			 ADDR = resistor - 6;
		 }
	 writeI2C(POT, 16, 192);        // select WR (volatile) registers in POT
	 writeI2C(POT, ADDR, value);
}

void SckUrban::writeI2C(byte deviceaddress, byte address, byte data ) {
	Wire.beginTransmission(deviceaddress);
	Wire.write(address);
	Wire.write(data);
	Wire.endTransmission();
	delay(4);
}

byte SckUrban::readI2C(int deviceaddress, byte address ) {
	byte  data = 0x0000;
	Wire.beginTransmission(deviceaddress);
	Wire.write(address);
	Wire.endTransmission();
	Wire.requestFrom(deviceaddress,1);
	unsigned long time = millis();
	while (!Wire.available()) if ((millis() - time)>500) return 0x00;
	data = Wire.read();
	return data;
}

// Temperature sensor
uint16_t SckUrban::readSHT(uint8_t type){
			uint16_t DATA = 0;
			Wire.beginTransmission(SHT21_I2C_DIR);
			Wire.write(type);
			Wire.endTransmission();
			Wire.requestFrom(SHT21_I2C_DIR,3);
			unsigned long time = millis();
			while (!Wire.available()) if ((millis() - time)>500) return 0x00;
			DATA = Wire.read()<<8; 
			DATA += Wire.read(); 
			Wire.read();
			DATA &= ~0x0003; 
			return DATA;
}
	
float SckUrban::getHumidity() {
	return (-6 + (125*(readSHT(0xE5)/65536.0)));
}

float SckUrban::getTemperature() {
	return (-46.85 + (175.72*(readSHT(0xE3)/65536.0)));
}

float SckUrban::getLight() {

	uint8_t TIME0  = 0xDA;
	uint8_t GAIN0 = 0x00;
	uint8_t DATA [8] = {0x03, TIME0, 0x00 ,0x00, 0x00, 0xFF, 0xFF ,GAIN0} ;

	uint16_t DATA0 = 0;
	uint16_t DATA1 = 0;

	Wire.beginTransmission(BH1730_I2C_DIR);
	Wire.write(0x80|0x00);
	for(int i= 0; i<8; i++) Wire.write(DATA[i]);
	Wire.endTransmission();
	delay(100); 
	Wire.beginTransmission(BH1730_I2C_DIR);
	Wire.write(0x94);  
	Wire.endTransmission();
	Wire.requestFrom(BH1730_I2C_DIR, 4);
	DATA0 = Wire.read();
	DATA0=DATA0|(Wire.read()<<8);
	DATA1 = Wire.read();
	DATA1=DATA1|(Wire.read()<<8);

	uint8_t Gain = 0x00; 
	if (GAIN0 == 0x00) Gain = 1;
	else if (GAIN0 == 0x01) Gain = 2;
	else if (GAIN0 == 0x02) Gain = 64;
	else if (GAIN0 == 0x03) Gain = 128;

	float ITIME =  (256- TIME0)*2.7;

	float Lx = 0;
	float cons = (Gain * 100) / ITIME;
	float comp = (float)DATA1/DATA0;


	if (comp<0.26) Lx = ( 1.290*DATA0 - 2.733*DATA1 ) / cons;
	else if (comp < 0.55) Lx = ( 0.795*DATA0 - 0.859*DATA1 ) / cons;
	else if (comp < 1.09) Lx = ( 0.510*DATA0 - 0.345*DATA1 ) / cons;
	else if (comp < 2.13) Lx = ( 0.276*DATA0 - 0.130*DATA1 ) / cons;
	else Lx=0;

	return Lx;
}

float SckUrban::getCO() {
	return 0;
}

float SckUrban::getNO2() {
	return 0;
}


void SckUrban::ADCini() {
	byte temp = readI2C(ADC_DIR,0)&B00000000;
	writeI2C(ADC_DIR, 0, temp);
}

void SckUrban::ADCoff() {
	byte temp = readI2C(ADC_DIR,0)&B00000000;
	writeI2C(ADC_DIR, 0, temp);
}
