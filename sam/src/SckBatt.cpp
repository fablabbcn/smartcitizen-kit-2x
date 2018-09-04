#include "SckBatt.h"

void SckCharger::setup()
{
	pinMode(pinCHARGER_INT, INPUT_PULLUP);

	resetConfig();

	// Disable I2C watchdog timer
	I2Cwatchdog(0);

	// Limit input current.
	inputCurrentLimit(900);

	// Limit charge current.
	chargerCurrentLimit(768);

	// We shouldn't take more than 5 hours to charge the normal battery
	chargeTimer(5);

	// if (getVBUSstatus() == VBUS_USB_HOST || getVBUSstatus() == VBUS_ADAPTER_PORT) OTG(false);
	// else OTG(true);
	OTG(true);

	attachInterrupt(pinCHARGER_INT, ISR_charger, FALLING);
}
bool SckCharger::resetConfig()
{

	byte conf = readREG(POWER_ON_CONF_REG);

	conf |= (1<<RESET_DEFAULT_CONFIG);

	if (writeREG(POWER_ON_CONF_REG, conf)) return true;
	else return false;
}
uint16_t SckCharger::inputCurrentLimit(int16_t current)
{

	// Bits 0:2 - > 000 – 100 mA, 001 – 150 mA, 010 – 500 mA, 011 – 900 mA, 100 – 1 A, 101 – 1.5 A, 110 - 2 A, 111 - 3 A
	uint16_t values[8] = {100, 150, 500, 900, 1000, 1500, 2000, 3000};

	if (current > -1) {

		byte limitIn = readREG(INPUT_SOURCE_CONTROL);
		limitIn &= ~(0b111 << INPUT_CURR_LIMIT);	// Clear bits
		if (current < 100) current = 100;

		// Go for the lower closest to the requested value
		for (uint8_t i=7; i>=0; i--) {
			if (current >= values[i]) {
				limitIn |= (i << INPUT_CURR_LIMIT);
				break;	
			}
		}

		writeREG(INPUT_SOURCE_CONTROL, limitIn);
	}

	return values[(readREG(INPUT_SOURCE_CONTROL) >> INPUT_CURR_LIMIT) & 0b111];
}
uint16_t SckCharger::chargerCurrentLimit(int16_t current)
{

	// (bits 2:6) Range: 512 – 2048mA (00000–11000), bit6 - 1024mA, bit5 - 512mA, bit4 - 256mA, bit3 - 128mA, bit2 - 64mA
	if (current > -1) {

		byte limitChg = readREG(CHARGE_CURRENT_CONTROL);
		limitChg &= ~(0b11111 << CHARGE_CURRENT_LIMIT);	// Clear bits
		if (current < 512) current = 512;
		else if (current > 2048) current = 2048;
		current = current - 512;

		// Go for the lower closest to the requested value
		for (int8_t i=4; i>=0; i--) {
			uint16_t value = pow(2, i) * 64;
			if (current >= value) {
				limitChg |= (1 << (i + CHARGE_CURRENT_LIMIT));
				current -= value;
			}
		}

		writeREG(CHARGE_CURRENT_CONTROL, limitChg);
	}

	return (((readREG(CHARGE_CURRENT_CONTROL) >> CHARGE_CURRENT_LIMIT) & 0b11111) * 64) + 512;
}
bool SckCharger::chargeState(int8_t enable)
{

	if (enable > -1) {
		byte conf = readREG(POWER_ON_CONF_REG);
		if (enable)	conf |= (1 << CHG_CONFIG);
		else  conf &= ~(1 << CHG_CONFIG);
		writeREG(POWER_ON_CONF_REG, conf);
	}

	return (readREG(POWER_ON_CONF_REG) >> CHG_CONFIG) & 1;
}
bool SckCharger::batfetState(int8_t enable)
{

	if (enable > -1) {
		byte tempMisc = readREG(MISC_REG);
		if (!enable) tempMisc |= (1 << BATFET_DISABLE); 	// 0 -> batfet ON
		else  tempMisc &= ~(1 << BATFET_DISABLE); 		// 1 -> batfet off
		writeREG(MISC_REG, tempMisc);
	}

	return !((readREG(MISC_REG) >> BATFET_DISABLE) & 1);
}
bool SckCharger::OTG(int8_t enable)
{

	// TODO review OTG function
	if (enable > -1) {
		
		// detach

		byte conf = readREG(POWER_ON_CONF_REG);
		if (enable) {

			chargeState(false);				// Charge off
			conf |= (1 << OTG_CONFIG);		// OTG on
			writeREG(POWER_ON_CONF_REG, conf);
			
		} else {
			SerialUSB.println("disabling OTG!");
			conf &= ~(1 << OTG_CONFIG);		// OTG off
			writeREG(POWER_ON_CONF_REG, conf);
			chargeState(true);				// Charge on
		}
		// writeREG(POWER_ON_CONF_REG, conf);
		
	}
	
	return (readREG(POWER_ON_CONF_REG) >> OTG_CONFIG) & 1;
}
uint8_t SckCharger::I2Cwatchdog(int16_t seconds)
{

	// I2C Watchdog Timer Setting. (Two bits 4:5) 00 – Disable timer, 01 – 40 s, 10 – 80 s, 11 – 160 s
	uint8_t values[4] = {0, 40, 80, 160};

	if (seconds > -1) {

		byte timerCtrl = readREG(CHG_TERM_TIMER_CTRL);
		timerCtrl &= ~(0b11 << I2C_WATCHDOG);				// Clear bits
		if (seconds > 0 && seconds < 40) seconds = 40;		// Exception (go for the minimal value, don't disable)

		// Go for the lower closest to the requested value
		for (uint8_t i=3; i>=0; i--) {
			if (seconds >= values[i]) {
				timerCtrl |= (i << I2C_WATCHDOG);
				break;	
			}
		}

		writeREG(CHG_TERM_TIMER_CTRL, timerCtrl);
	}

	return values[(readREG(CHG_TERM_TIMER_CTRL) & (0b11 << I2C_WATCHDOG)) >> I2C_WATCHDOG];
}
uint8_t SckCharger::chargeTimer(int8_t hours)
{

	// 0 hours = disable timer

	// Fast Charge Timer Setting. (Two bits 1:2) 00 – 5 hrs, 01 – 8 hrs, 10 – 12 hrs, 11 – 20 hrs
	uint8_t values[4] = {5, 8, 12, 20};

	if (hours > -1) {
		
		byte chgTimer = readREG(CHG_TERM_TIMER_CTRL);

		chgTimer &= ~(1 << CHG_TIMER_ENABLE);		// Disable timer
		writeREG(CHG_TERM_TIMER_CTRL, chgTimer);

		if (hours > 0) {

			if (hours < 5) hours = 5;

			chgTimer &= ~(0b11 << CHG_TIMER); 	// Clear bits

			// Go for the lower closest to the requested value
			for (uint8_t i=3; i>=0; i--) {
				if (hours >= values[i]) {
					chgTimer |= (i << CHG_TIMER);
					break;	
				}
			}
			writeREG(CHG_TERM_TIMER_CTRL, chgTimer);		// Save new settings

			chgTimer |= 1 << CHG_TIMER_ENABLE;				// Enable timer
			writeREG(CHG_TERM_TIMER_CTRL, chgTimer);
		}
	}

	// if enabled return actual value
	if ((readREG(CHG_TERM_TIMER_CTRL) & (1 << CHG_TIMER_ENABLE)) >> CHG_TIMER_ENABLE) return values[(readREG(CHG_TERM_TIMER_CTRL) & (0b11 << CHG_TIMER)) >> CHG_TIMER];
	else return 0;
}
bool SckCharger::getPowerGoodStatus()
{

	byte status = readREG(SYS_STATUS_REG);
	return status & (1 << PG_STAT);
}
bool SckCharger::getDPMstatus()
{

	byte status = readREG(SYS_STATUS_REG);
	return status & (1 << DPM_STAT);	
}
void SckCharger::forceInputCurrentLimitDetection()
{

	// Force current limit detection on VBUS connection -> MISC_REG:bit7 = 1
	byte tempMisc = readREG(MISC_REG);
	tempMisc |= (1 << DPDM_EN);
	writeREG(MISC_REG, tempMisc);
}
SckCharger::chargeStatus SckCharger::getChargeStatus()
{

	byte status = readREG(SYS_STATUS_REG);
	status &= (0b11 << CHRG_STAT);
	status >>= CHRG_STAT;
	return static_cast<chargeStatus>(status);
}
SckCharger::VBUSstatus SckCharger::getVBUSstatus()
{

	byte status = readREG(SYS_STATUS_REG);
	status &= (0b11 << VBUS_STAT);
	status >>= VBUS_STAT;
	return static_cast<VBUSstatus>(status);
}
byte SckCharger::getNewFault()
{

	byte fault = readREG(NEW_FAULT_REGISTER);

	return fault;
}
byte SckCharger::readREG(byte wichRegister)
{

	Wire.beginTransmission(address);
	Wire.write(wichRegister);
	Wire.endTransmission(true);
	Wire.requestFrom(address, 1);

	uint32_t started = millis();
	while(Wire.available() != 1) {
  		if (millis() - started > timeout) return -1;
   	}
   	return Wire.read();
}
bool SckCharger::writeREG(byte wichRegister, byte data)
{

	Wire.beginTransmission(address);
	Wire.write(wichRegister);
    Wire.write(data);
    Wire.endTransmission(true);

    if (readREG(wichRegister) == data) return true;
    else return false;
}
void SckCharger::event()
{
	// TODO limpiar los Serial print
	SerialUSB.println("Charger event");

	if (getChargeStatus() == CHRG_CHARGE_TERM_DONE) {
		/* sckOut("Batterry fully charged!!"); */
		/* chargeState(0); */
	}

	while (getDPMstatus()) {} // Wait for DPM detection finish

	if (getPowerGoodStatus()) {

		SerialUSB.println("Power source connected");
		if (!onUSB) {
			onUSB = true;
			// To start with a clean state and make sure charging is OK do a reset when power is connected.
			NVIC_SystemReset();
		}
		// charger.OTG(false);

	} else {

		SerialUSB.println("No power source");
		onUSB = false;
		// charger.OTG(true);
	}

	// TODO, React to any charger fault
	if (getNewFault() != 0) {
		SerialUSB.println("Charger fault!!");
	}

	if (!onUSB) digitalWrite(pinLED_USB, HIGH); 	// Turn off Serial leds
}

// Battery
bool SckBatt::isPresent()
{
	// First check pinBATT_INSERTION
	uint32_t valueRead = 0;
	pinPeripheral(pinBATT_INSERTION, PIO_ANALOG);
	while (ADC->STATUS.bit.SYNCBUSY == 1);
	ADC->INPUTCTRL.bit.MUXPOS = ADC_Channel6; 	// Selection for the positive ADC input
	while (ADC->STATUS.bit.SYNCBUSY == 1);
	ADC->CTRLA.bit.ENABLE = 0x01;             	// Enable ADC
	while (ADC->STATUS.bit.SYNCBUSY == 1); 		// Start conversion
	ADC->SWTRIG.bit.START = 1;
	ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY; 		// Clear the Data Ready flag
	while (ADC->STATUS.bit.SYNCBUSY == 1);		// Start conversion again, since The first conversion after the reference is changed must not be used.
	ADC->SWTRIG.bit.START = 1;
	while (ADC->INTFLAG.bit.RESRDY == 0);   	// Waiting for conversion to complete
	valueRead = ADC->RESULT.reg;			// Store the value
	while (ADC->STATUS.bit.SYNCBUSY == 1);
	ADC->CTRLA.bit.ENABLE = 0x00;             	// Disable ADC
	while (ADC->STATUS.bit.SYNCBUSY == 1);

	// Verify analog read of pinBATT_INSERTION
	if (valueRead < 400) {

		// Verify that fuel gauge is responding (revisar esto, por que no siempre responde bien...)
		// tengo que lograr una manera de resetear el gauge cuando la bateria se conecto despues del cable.
		// si no se logra habra que documentar MUY BIEN para que el usuario nunca conecte la bateria despues del cable, o buscar una manera de no encender cuando eso pase....
		Wire.beginTransmission(address);
		uint8_t error = Wire.endTransmission();

		if (error == 0) {

			present = true;	
			return true;
		} else {
			
			SerialUSB.println("Gauge not responding!!");

			//----- Se supone que esto lo saca de shutdown mode, pero no se despierta!!!!!
			/* pinMode(pinGAUGE_INT, OUTPUT); */
			/* digitalWrite(pinGAUGE_INT, LOW); */
			/* delay(2); */
			/* digitalWrite(pinGAUGE_INT, HIGH); */
			/* delay(2); */
			/* digitalWrite(pinGAUGE_INT, LOW); */
			/* delay(2); */
			/* pinMode(pinGAUGE_INT, INPUT_PULLUP); */
			/* attachInterrupt(pinGAUGE_INT, ISR_battery, FALLING); */
			//------------------
		
		
		}

	} 
	
	present = false;
	configured = false;
	return false;
}
bool SckBatt::setup()
{
	if (configured) return true;

	if (!isPresent()) return false;

	pinMode(pinGAUGE_INT, INPUT_PULLUP);
	attachInterrupt(pinGAUGE_INT, ISR_battery, FALLING);

	lipo.enterConfig();
	lipo.setCapacity(capacitymAh);
	lipo.setGPOUTPolarity(LOW);
	lipo.setGPOUTFunction(SOC_INT);
	lipo.setSOCIDelta(1);
	lipo.exitConfig();

	configured = true;

	// TODO if battery is not full and we are not charging run charger.setup here

	return true;
}
void SckBatt::event()
{
	if (!setup()) return;
	/* getReading(SENSOR_BATT_PERCENT); */
	/* sprintf(outBuff, "Battery charge %s%%", sensors[SENSOR_BATT_PERCENT].reading.c_str()); */
	/* sckOut(PRIO_LOW); */			
}
void SckBatt::batteryReport()
{
	if (!configured && !setup()) return;

	/* sprintf(outBuff, "Charge: %u %%\r\nVoltage: %u V\r\nCharge current: %i mA\r\nCapacity: %u/%u mAh\r\nState of health: %i", */
	/* 		lipo.soc(), */
	/* 		lipo.voltage(), */
	/* 		lipo.current(AVG), */
	/* 		lipo.capacity(REMAIN), */
	/* 		lipo.capacity(FULL), */
	/* 		lipo.soh() */
       /* ); */
}
