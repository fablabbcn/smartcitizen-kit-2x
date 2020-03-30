#include "SckBatt.h"
#include "SckBase.h"

void SckCharger::setup(SckBase *base)
{
	pinMode(pinCHARGER_INT, INPUT_PULLUP);

	resetConfig();

	// Set SYSV_MIN to 3.3v  011
	sysMinVolt(3300);

	// Disable I2C watchdog timer
	I2Cwatchdog(0);

	// Limit input current.
	inputCurrentLimit(1500);

	// Limit charge current.
	chargerCurrentLimit(base->config.battConf.chargeCurrent);

	// 5, 8, 12 or 20 hours depending on battery size.
	uint32_t newHours = (base->config.battConf.battCapacity / base->config.battConf.chargeCurrent);
	chargeTimer(newHours);

	OTG(true);

	chargeState(1);
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
		if (enable) conf |= (1 << CHG_CONFIG);
		else  conf &= ~(1 << CHG_CONFIG);
		writeREG(POWER_ON_CONF_REG, conf);
	}

	return (readREG(POWER_ON_CONF_REG) >> CHG_CONFIG) & 1;
}
bool SckCharger::batfetState(int8_t enable)
{

	if (enable > -1) {
		byte tempMisc = readREG(MISC_REG);
		if (!enable) {
			tempMisc |= (1 << BATFET_DISABLE); 	// 0 -> batfet ON
			batfetON = true;
		} else  {
			tempMisc &= ~(1 << BATFET_DISABLE); 		// 1 -> batfet off
			batfetON = false;
		}
		writeREG(MISC_REG, tempMisc);
	}

	return !((readREG(MISC_REG) >> BATFET_DISABLE) & 1);
}
bool SckCharger::OTG(int8_t enable)
{

	// TODO review OTG function
	if (enable > -1) {

		byte conf = readREG(POWER_ON_CONF_REG);
		if (enable) {

			chargeState(false);				// Charge off
			conf |= (1 << OTG_CONFIG);		// OTG on
			writeREG(POWER_ON_CONF_REG, conf);

		} else {
			conf &= ~(1 << OTG_CONFIG);		// OTG off
			writeREG(POWER_ON_CONF_REG, conf);
			chargeState(true);				// Charge on
		}

	}

	return (readREG(POWER_ON_CONF_REG) >> OTG_CONFIG) & 1;
}
uint8_t SckCharger::I2Cwatchdog(int16_t rseconds)
{

	// I2C Watchdog Timer Setting. (Two bits 4:5) 00 – Disable timer, 01 – 40 s, 10 – 80 s, 11 – 160 s
	uint8_t values[4] = {0, 40, 80, 160};

	if (rseconds > -1) {

		byte timerCtrl = readREG(CHG_TERM_TIMER_CTRL);
		timerCtrl &= ~(0b11 << I2C_WATCHDOG);				// Clear bits
		if (rseconds > 0 && rseconds < 40) rseconds = 40;		// Exception (go for the minimal value, don't disable)

		// Go for the lower closest to the requested value
		for (uint8_t i=3; i>=0; i--) {
			if (rseconds >= values[i]) {
				timerCtrl |= (i << I2C_WATCHDOG);
				break;
			}
		}

		writeREG(CHG_TERM_TIMER_CTRL, timerCtrl);
	}

	return values[(readREG(CHG_TERM_TIMER_CTRL) & (0b11 << I2C_WATCHDOG)) >> I2C_WATCHDOG];
}
uint8_t SckCharger::chargeTimer(int8_t rhours)
{
	// 0 hours = disable timer
	// Fast Charge Timer Setting. (Two bits 1:2) 00 – 5 hrs, 01 – 8 hrs, 10 – 12 hrs, 11 – 20 hrs
	uint8_t values[4] = {5, 8, 12, 20};

	if (rhours > -1) {

		byte chgTimer = readREG(CHG_TERM_TIMER_CTRL);

		chgTimer &= ~(1 << CHG_TIMER_ENABLE);		// Disable timer
		writeREG(CHG_TERM_TIMER_CTRL, chgTimer);

		if (rhours > 0) {

			if (rhours < 5) rhours = 5;
			else if (rhours > 20) rhours = 20;

			chgTimer &= ~(0b11 << CHG_TIMER); 	// Clear bits

			// Go for the upper closest to the requested value
			for (uint8_t i=0; i<4; i++) {
				if (rhours <= values[i]) {
					chgTimer |= (i<<CHG_TIMER);
					break;
				}
			}
			writeREG(CHG_TERM_TIMER_CTRL, chgTimer);		// Save new settings

			chgTimer |= 1 << CHG_TIMER_ENABLE;				// Enable timer
			writeREG(CHG_TERM_TIMER_CTRL, chgTimer);
		}
	}

	// if enabled return actual value, if disabled return 0
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
SckCharger::ChargeStatus SckCharger::getChargeStatus()
{
	byte status = readREG(SYS_STATUS_REG);
	status &= (0b11 << CHRG_STAT);
	status >>= CHRG_STAT;
	return static_cast<ChargeStatus>(status);
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
void SckCharger::detectUSB(SckBase *base)
{
	VBUSstatus vbusStatus = getVBUSstatus();

	if (vbusStatus == VBUS_UNKNOWN) {
		// Wait for DPM detection finish
		while (getDPMstatus()) delay(1);
	}

	if (vbusStatus == VBUS_ADAPTER_PORT) {
		if (!onUSB) {
			onUSB = true;
			if (!base->sckOFF) NVIC_SystemReset(); 	// To start with a clean state and make sure charging is OK do a reset when power is connected.
		}
	} else  {
		if (onUSB) {
			// This registers disconnection of the charger as a user event
			base->lastUserEvent = millis();
		}
		onUSB = false;
	}

	if (!onUSB) digitalWrite(pinLED_USB, HIGH); 	// Turn off Serial leds
}
float SckCharger::sysMinVolt(int16_t voltage)
{
	byte powOnReg = readREG(POWER_ON_CONF_REG);

	// Set voltage
	if (voltage >= 3000 && voltage <= 3700) {

		powOnReg &= 0b11110001; 	// Clear Minimum System Voltage Limit bits
		uint16_t diffVolt = voltage - 3000;

		if (diffVolt >= 400) {
			powOnReg |= (1 << 3);
			diffVolt -= 400;
		}

		if (diffVolt >= 200) {
			powOnReg |= (1 << 2);
			diffVolt -= 200;
		}

		if (diffVolt >= 100) {
			powOnReg |= (1 << 1);
			diffVolt -= 100;
		}

		writeREG(POWER_ON_CONF_REG, powOnReg);
	}

	// Get voltage
	uint16_t minVolt = 3000;

	powOnReg = readREG(POWER_ON_CONF_REG);

	powOnReg = (powOnReg>>VSYS_MIN) & 0b111;

	minVolt += (100 * (powOnReg & 1));
	minVolt += (200 * ((powOnReg>>1) & 1));
	minVolt += (400 * ((powOnReg>>2) & 1));

	return (float)(minVolt/1000.0);
}
bool SckCharger::getBatLowerSysMin()
{
	byte ssr = readREG(SYS_STATUS_REG);

	return (ssr & 1);
}

// Battery
bool SckBatt::setup()
{
	pinMode(pinMEASURE_BATT, INPUT);
	pinPeripheral(pinMEASURE_BATT, PIO_ANALOG);

	return true;
}
float SckBatt::voltage()
{
	uint32_t total = 0;
	int8_t sampleNum = 10;
	for (uint8_t i=0; i<sampleNum; i++) total += analogRead(pinMEASURE_BATT);

	float thisVoltage = (total/sampleNum) * 2.0 / analogResolution * workingVoltage;
	last_volt = thisVoltage;
	return thisVoltage;
}
int8_t SckBatt::percent(SckCharger *charger)
{
	int8_t percent = -1;

	uint16_t thisVoltage = (uint16_t)(voltage() * 1000);

	SckCharger::ChargeStatus currentStatus = charger->getChargeStatus();

	if (currentStatus == charger->CHRG_PRE_CHARGE || currentStatus == charger->CHRG_FAST_CHARGING) {

		if (thisVoltage >= batTableCharging[99]) percent = 100;
		else if (thisVoltage <= batTableCharging[0]) percent = 0;
		else {
			// Search in the batt table
			for (int8_t i=1; i<100; i++) {
				if (thisVoltage < batTableCharging[i]) {
					percent = i;
					break;
				}
			}
		}
	} else {

		if (thisVoltage >= batTable[99]) percent = 100;
		else if (thisVoltage <= batTable[0]) percent = 0;
		else {
			// Search in the batt table
			for (int8_t i=1; i<100; i++) {
				if (thisVoltage < batTable[i]) {
					percent = i;
					break;
				}
			}
		}
	}
	last_percent = percent;
	return percent;
}
