#include "sckUrban.h"

uint8_t pot_6_db_preset[] = {0,  7, 26, 96, 255};
uint8_t pot_7_db_preset[] = {0, 17, 97, 255, 255};

void SckUrban::setup() {

	// Gases
	gasSetup(SENSOR_CO);
	gasSetup(SENSOR_NO2);

	// Sound
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
		case SENSOR_CO: return gasRead(SENSOR_CO); 	break;
		case SENSOR_CO_HEAT_TIME: return gasHeatingTime(SENSOR_CO); break;
		case SENSOR_CO_HEAT_CURRENT: return gasGetHeaterCurrent(SENSOR_CO); break;
		case SENSOR_CO_HEAT_SUPPLY_VOLTAGE: return gasGetRegulatorVoltage(SENSOR_CO); break;
		case SENSOR_CO_HEAT_DROP_VOLTAGE: return (SENSOR_CO); break;
		case SENSOR_CO_LOAD_RESISTANCE: return gasGetLoadResistance(SENSOR_CO); break;
		case SENSOR_NO2: return gasRead(SENSOR_NO2); break;
		case SENSOR_NO2_HEAT_TIME: return gasHeatingTime(SENSOR_NO2); break;
		case SENSOR_NO2_HEAT_CURRENT: return gasGetHeaterCurrent(SENSOR_NO2); break;
		case SENSOR_NO2_HEAT_SUPPLY_VOLTAGE: return gasGetRegulatorVoltage(SENSOR_NO2); break;
		case SENSOR_NO2_HEAT_DROP_VOLTAGE: return (SENSOR_NO2); break;
		case SENSOR_NO2_LOAD_RESISTANCE: return gasGetLoadResistance(SENSOR_NO2); break;
	}
}

String SckUrban::control(SensorType wichSensor, String command) {
	 switch (wichSensor) {
		case SENSOR_NO2:
		case SENSOR_CO: {

			if (command.startsWith("set on")) {
				gasOn(wichSensor);
				return F("Starting sensor...");

			} else if (command.startsWith("set off")) {
				gasOff(wichSensor);
				return F("Shuting off heater...");

			} else if (command.startsWith("set current")) {
				command.replace("set current", "");
				command.trim();
				int wichValue = command.toInt();
				gasHeat(wichSensor, wichValue);
				return String F("Setting current to: ") + String(wichValue) + F(" mA\n\rActual value: ") + String(gasGetHeaterCurrent(wichSensor)) + F(" mA");

			} else if (command.startsWith("set voltage")) {
				command.replace("set voltage", "");
				command.trim();
				int wichValue = command.toInt();
				gasSetRegulatorVoltage(wichSensor, wichValue);
				return String F("Setting heater voltage to: ") + String(wichValue) + F(" mV\n\rActual value: ") + String(gasGetRegulatorVoltage(wichSensor)) + F(" mV");

			} else if (command.startsWith("correct current")) {
				gasCorrectHeaterCurrent(wichSensor);
				return String F("Actual value: ") + String(gasGetHeaterCurrent(wichSensor)) + F(" mA\n\rCorrecting current\n\rActual value: ") + String(gasGetHeaterCurrent(wichSensor)) + F(" mA");

			} else if (command.startsWith("help")) {
				return F("Available commands for this sensor:\n\r* set on - set off\n\r* set current\n\r* set voltage");

			} else {
				return F("Unrecognized command!! please try again...");
			}

			break;
		}
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

// Gas sensors
void SckUrban::gasSetup(SensorType wichSensor) {

	if (wichSensor == SENSOR_CO) {
		// Voltage regulator setup
		pinMode(SHUTDOWN_CONTROL_REGULATOR_CO_SENSOR_HEATER_PIN, OUTPUT);
		digitalWrite(SHUTDOWN_CONTROL_REGULATOR_CO_SENSOR_HEATER_PIN, LOW);		// Turn off regulator

		// Load resistor setup (minimal safe value is 820)
		setPot(POT_CO_LOAD_RESISTOR, 50000);
		gasCOheaterState = false;

	} else if (wichSensor == SENSOR_NO2) {

		// Voltage regulator setup
		pinMode(SHUTDOWN_CONTROL_REGULATOR_NO2_SENSOR_HEATER_PIN, OUTPUT);
		digitalWrite(SHUTDOWN_CONTROL_REGULATOR_NO2_SENSOR_HEATER_PIN, LOW);		// Turn off regulator

		// Load resistor setup (minimal safe value is 820)
		setPot(POT_NO2_LOAD_RESISTOR, 50000);
		gasNO2heaterState = false;
	}
}

void SckUrban::gasOn(SensorType wichSensor) {

	if (wichSensor == SENSOR_CO) {
		
		// Load resistor setup (minimal safe value is 820)
		setPot(POT_CO_LOAD_RESISTOR, 100000);
		
		gasHeat(wichSensor, CO_HEATING_CURRENT);
		// TODO do i need to correct current here???

	} else if (wichSensor == SENSOR_NO2) {

		// Load resistor setup (minimal safe value is 820)
		setPot(POT_NO2_LOAD_RESISTOR, 100000);
		
		gasHeat(wichSensor, NO2_HEATING_CURRENT);
	}
}

void SckUrban::gasOff(SensorType wichSensor) {

	if (wichSensor == SENSOR_CO) {
		// Turn off regulator
		digitalWrite(SHUTDOWN_CONTROL_REGULATOR_CO_SENSOR_HEATER_PIN, LOW);

		// Load resistor setup (minimal safe value is 820)
		setPot(POT_CO_LOAD_RESISTOR, 100000);

		gasCOheaterState = false;

	} else if (wichSensor == SENSOR_NO2) {
		// Turn off regulator
		digitalWrite(SHUTDOWN_CONTROL_REGULATOR_NO2_SENSOR_HEATER_PIN, LOW);

		// Load resistor setup (minimal safe value is 820)
		setPot(POT_NO2_LOAD_RESISTOR, 100000);

		gasNO2heaterState = false;
	}
}

void SckUrban::gasHeat(SensorType wichSensor, uint32_t wichCurrent) {

	if (wichSensor == SENSOR_CO) {

		// store requested current
		// TODO Save this current in eprom for persistence
		CO_HEATING_CURRENT = wichCurrent;				

		// Calculate required voltage for requested current
		uint32_t calculatedRegulatorVoltage = (CO_HEATER_RESISTANCE * CO_HEATING_CURRENT) + (CO_HEATER_RESISTOR * CO_HEATING_CURRENT);

		// Start on the low side and wait for current correction
		calculatedRegulatorVoltage -= 500;

		// Set voltage to functional value
		gasSetRegulatorVoltage(wichSensor, calculatedRegulatorVoltage);

		// Turn on regulator
		digitalWrite(SHUTDOWN_CONTROL_REGULATOR_CO_SENSOR_HEATER_PIN, HIGH);

		startHeaterTime_CO = millis();
		gasCOheaterState = true;

		// Correct current
		gasCorrectHeaterCurrent(wichSensor);

	} else if (wichSensor == SENSOR_NO2) {

		// store requested current
		// Save this current in eprom for persistence
		NO2_HEATING_CURRENT = wichCurrent;

		// Calculate required voltage for requested current
		uint32_t calculatedRegulatorVoltage = (NO2_HEATER_RESISTANCE * NO2_HEATING_CURRENT) + (NO2_HEATER_RESISTOR * NO2_HEATING_CURRENT);

		// Start on the low side and wait for current correction
		calculatedRegulatorVoltage -= 500;
		
		// Set voltage to functional value
		gasSetRegulatorVoltage(wichSensor, calculatedRegulatorVoltage);

		// Turn on regulator
		digitalWrite(SHUTDOWN_CONTROL_REGULATOR_NO2_SENSOR_HEATER_PIN, HIGH);

		startHeaterTime_NO2 = millis();
		gasNO2heaterState = true;

		// Correct current
		gasCorrectHeaterCurrent(wichSensor);
	}
}

float SckUrban::gasGetRegulatorVoltage(SensorType wichSensor) {

	// Get actual variable resistor value (for selected sensor)
	uint32_t actualPOTvalue = 0;
	if (wichSensor == SENSOR_CO) actualPOTvalue = getPot(POT_CO_REGULATOR);
	else if (wichSensor == SENSOR_NO2) actualPOTvalue = getPot(POT_NO2_REGULATOR);

	// Calculate actual regulator voltage TODO check calculation
	float regulatorVoltage = (((float)actualPOTvalue / HEATER_REGULATOR_RESISTOR) + 1) * 800.0;

	return regulatorVoltage;
}

float SckUrban::gasGetDropVoltage(SensorType wichSensor) {
	if (wichSensor == SENSOR_CO) return ((float)average(CO_HEATER_VOLTAGE_PIN) * VCC) / RESOLUTION_ANALOG; 			// (mV) Measure voltage
	else if (wichSensor == SENSOR_NO2) return ((float)average(NO2_HEATER_VOLTAGE_PIN) * VCC) / RESOLUTION_ANALOG; 	// (mV) Measure voltage
}

void SckUrban::gasSetRegulatorVoltage(SensorType wichSensor, uint32_t wichVoltage) {

	// each step is around 13 mV

	// Minimal value (800 mV)
	if (wichVoltage < 800) wichVoltage = 800;

	// Calculate required variable resistor for desired voltage
	float desiredPOTvalue = ((wichVoltage / 800.0) - 1) * HEATER_REGULATOR_RESISTOR;

	// Set new variable resistor value
	if (wichSensor == SENSOR_CO) {
		setPot(POT_CO_REGULATOR, (uint32_t)desiredPOTvalue);
	} else if (wichSensor == SENSOR_NO2) {
		setPot(POT_NO2_REGULATOR, (uint32_t)desiredPOTvalue);
	}
}

float SckUrban::gasGetHeaterCurrent(SensorType wichSensor) {
	float measuredVoltage = gasGetDropVoltage(wichSensor);
	if (wichSensor == SENSOR_CO) return measuredVoltage / CO_HEATER_RESISTOR;			// (mA) Calculates current
	else if (wichSensor == SENSOR_NO2) return measuredVoltage / NO2_HEATER_RESISTOR;		// (mA) Calculates current
}

void SckUrban::gasCorrectHeaterCurrent(SensorType wichSensor) {

	// New current value depends on sensor
	float desiredCurrent = 0;
	if (wichSensor == SENSOR_CO) desiredCurrent = CO_HEATING_CURRENT;
	else if (wichSensor == SENSOR_NO2) desiredCurrent = NO2_HEATING_CURRENT;

	// HeaterCurrent = HeaterVoltage / HEATER_RESISTOR
	float heaterVoltage = gasGetDropVoltage(wichSensor); 											// (mV) Measure voltage
	float heaterCurrent = 0;
	if (wichSensor == SENSOR_CO) heaterCurrent = heaterVoltage / CO_HEATER_RESISTOR;				// (mA) Calculates current
	else if (wichSensor == SENSOR_NO2) heaterCurrent = heaterVoltage / NO2_HEATER_RESISTOR;			// (mA) Calculates current

	while (desiredCurrent - heaterCurrent > 0) {

		// HeaterCurrent = HeaterVoltage / HEATER_RESISTOR
		heaterVoltage = gasGetDropVoltage(wichSensor); 											// (mV) Measure voltage
		if (wichSensor == SENSOR_CO) heaterCurrent = heaterVoltage / CO_HEATER_RESISTOR;			// (mA) Calculates current
		else if (wichSensor == SENSOR_NO2) heaterCurrent = heaterVoltage / NO2_HEATER_RESISTOR;		// (mA) Calculates current

		// HeaterResistance = (RegulatorVoltage - HeaterVoltage) / HeaterCurrent;
		float regulatorVoltage = gasGetRegulatorVoltage(wichSensor);
		float heaterResistance = (regulatorVoltage - heaterVoltage) / heaterCurrent;				// (Ohms)

		// RegulatorVoltage = HeaterCurrent * (HeaterResistance + HEATER_RESISTOR)
		float newRegulatorVoltage = 0;
		if (wichSensor == SENSOR_CO) newRegulatorVoltage = desiredCurrent * (heaterResistance + CO_HEATER_RESISTOR);		// Calculates new voltage for desired current
		else if (wichSensor == SENSOR_NO2) newRegulatorVoltage = desiredCurrent * (heaterResistance + NO2_HEATER_RESISTOR);	// Calculates new voltage for desired current

		// Increase at least one step of the regulator POT (one step is +/- 13 mV)
		if (newRegulatorVoltage - regulatorVoltage < 20) newRegulatorVoltage += 20;

		gasSetRegulatorVoltage(wichSensor, newRegulatorVoltage);										// Sets calculated voltage

		heaterCurrent = gasGetHeaterCurrent(wichSensor);
	}


}

float SckUrban::gasGetSensorResistance(SensorType wichSensor) {

	float sensorVoltage = 0;
	
	if (wichSensor == SENSOR_CO) sensorVoltage = ((float)average(CO_SENSOR_VOLTAGE_PIN) * VCC) / RESOLUTION_ANALOG; 	// (mV)	Measure sensor Voltage
	else if (wichSensor == SENSOR_NO2) sensorVoltage = ((float)average(NO2_SENSOR_VOLTAGE_PIN) * VCC) / RESOLUTION_ANALOG; 	// (mV)	Measure sensor Voltage

	if (sensorVoltage > VCC) sensorVoltage = VCC;
	float sensorResistance = ((VCC - sensorVoltage) / sensorVoltage) * gasGetLoadResistance(wichSensor); 		// (Ohms) calculate sensor resistance

	return sensorResistance;
}

uint32_t SckUrban::gasHeatingTime(SensorType wichSensor) {
	
	if (wichSensor == SENSOR_CO && gasCOheaterState) {

		return (millis() - startHeaterTime_CO) / 1000;

	} else if (wichSensor == SENSOR_NO2 && gasNO2heaterState) {
	
		return (millis() - startHeaterTime_NO2) / 1000;

	// If the sensors are off
	} else {
		return 0;
	}
}

uint32_t SckUrban::gasGetLoadResistance(SensorType wichSensor) {
	if (wichSensor == SENSOR_CO) return getPot(POT_CO_LOAD_RESISTOR);
	else if (wichSensor == SENSOR_NO2) return getPot(POT_NO2_LOAD_RESISTOR);
}

float SckUrban::gasRead(SensorType wichSensor) {

	// Turn on heater if it isn't
	if (wichSensor == SENSOR_CO && !gasCOheaterState) gasHeat(SENSOR_CO, CO_HEATING_CURRENT);
	else if (wichSensor == SENSOR_NO2 && !gasNO2heaterState) gasHeat(SENSOR_NO2, NO2_HEATING_CURRENT);

	Resistor wichResistor;
	if (wichSensor == SENSOR_CO) {
		wichResistor.deviceAddress = POT_CO_LOAD_RESISTOR.deviceAddress;
		wichResistor.resistorAddress = POT_CO_LOAD_RESISTOR.resistorAddress;
	} else if (wichSensor == SENSOR_NO2) {
		wichResistor.deviceAddress = POT_NO2_LOAD_RESISTOR.deviceAddress;
		wichResistor.resistorAddress = POT_NO2_LOAD_RESISTOR.resistorAddress;
	}

	// Get value from both resistors
	float loadResistor = getPot(wichResistor);							// (Ohms)
	float sensorResistance = gasGetSensorResistance(wichSensor);		// (Ohms)

	// Adjust range max 5 times
	uint8_t cycles = 5;
	for (uint8_t i=0; i<cycles; ++i)	{
		if (abs(loadResistor - sensorResistance) > 1000) {				// If difference between result and POT load resistor is graeter than 1000, try to improve resolution
			if (sensorResistance < 2000) setPot(wichResistor, 2000);	// Set POT to minimal value in 2000
			else if (sensorResistance > 100000) {
				setPot(wichResistor, 100000); 							// Set POT to maximum value
				break;													// Dont keep going if result resitance is greater than max POT value
			}
			else setPot(wichResistor, sensorResistance);				// Set POT to the same value as the result
			delay(10);

			loadResistor = getPot(wichResistor);
			sensorResistance = gasGetSensorResistance(wichSensor);

		} else break;
	}
	return sensorResistance / 1000;
}


// Utility functions
void SckUrban::ADCini() {
	byte temp = readI2C(ADC_DIR,0)&B00000000;
	writeI2C(ADC_DIR, 0, temp);
}

void SckUrban::ADCoff() {
	byte temp = readI2C(ADC_DIR,0)&B00000000;
	writeI2C(ADC_DIR, 0, temp);
}

void SckUrban::setPot(Resistor wichPot, uint32_t value) {

	// Check minimal safe value for Gas sensor (820)
	if (wichPot.deviceAddress == POT3 && value < 820) value = 820;

	// Data to be writen
	int data=0x00;
	if (value>100000) value = 100000;
	data = (int)(value/ohmsPerStep);
	
	Wire.beginTransmission(wichPot.deviceAddress);
	Wire.write(wichPot.resistorAddress);
	Wire.write(data);
	Wire.endTransmission();
	delay(4);
}

uint32_t SckUrban::getPot(Resistor wichPot) {

	byte data = 0x0000;

	Wire.beginTransmission(wichPot.deviceAddress);
	Wire.write(wichPot.resistorAddress);
	Wire.endTransmission();
	Wire.requestFrom(wichPot.deviceAddress,1);

	// Wait for answer with a timeout
	uint16_t waitTimeout = 500;
	uint32_t time = millis();
	while (!Wire.available()) if ((millis() - time) > waitTimeout) return 0x00;
	data = Wire.read();
	return data*ohmsPerStep;
}

float SckUrban::average(uint8_t wichPin) {

	uint32_t numReadings = 100;
	long total = 0;
	float average = 0;
	for(uint32_t i=0; i<numReadings; i++) {
		total = total + analogRead(wichPin);
	}
	average = (float)total / numReadings;  
	return average;
}
