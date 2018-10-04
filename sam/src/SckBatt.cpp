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

	OTG(true);

	attachInterrupt(pinCHARGER_INT, ISR_charger, FALLING);

	chargeState(0);

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
			SerialUSB.println("disabling OTG!");
			conf &= ~(1 << OTG_CONFIG);		// OTG off
			writeREG(POWER_ON_CONF_REG, conf);
			chargeState(true);				// Charge on
		}
		
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
void SckCharger::event()
{
	// Wait for DPM detection finish
	while (getDPMstatus()) delay(1);

	if (getPowerGoodStatus()) {
		if (!onUSB) {
			onUSB = true;
			NVIC_SystemReset(); 	// To start with a clean state and make sure charging is OK do a reset when power is connected.
		}

	} else onUSB = false;

	if (!onUSB) digitalWrite(pinLED_USB, HIGH); 	// Turn off Serial leds
}

// Battery
bool SckBatt::isPresent()
{
	// First check pinBATT_INSERTION
	uint32_t valueRead = 0;
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

	if (valueRead < 400) {
		Wire.beginTransmission(address);
		uint8_t error = Wire.endTransmission();

		if (error == 0) {
			present = true;	
			if (!configured) {
				if (!setup()) return false;
			}
			return true;
		}
	} 
	
	present = false;
	configured = false;
	return false;
}
bool SckBatt::setup(bool force)
{
	// This function should only be called if we are sure the batt is present (from inside battery.isPresent())
	if (configured && !force) return true;

	// Check if gauge is alive
	Wire.beginTransmission(address);
	uint8_t error = Wire.endTransmission();
	if (error != 0) return false;

	// --- Configure battery
	enterConfig();

	// Set chemID
	setChemID(GAUGE_CTRL_SET_CHEM_B);
	
	// Set Design Capacity
	setSubclass(GAUGE_CLASSID_STATE, designCapacityOffset, designCapacity);
	
	// Set Design Energy
	setSubclass(GAUGE_CLASSID_STATE, designEnergyOffset, designCapacity * nominalVoltage);
	
	// Set Terminate Voltage
	setSubclass(GAUGE_CLASSID_STATE, terminateVoltageOffset, terminateVoltage);
	
	// Set Taper Rate 
	setSubclass(GAUGE_CLASSID_STATE, taperRateOffset, designCapacity  / (0.1 * taperCurrent));

	// Allow reporting 100% even when Charge termination is not detected
	setSubclass(GAUGE_CLASSID_OPCONFIG, opConfig_C_Offset, 0b10001111);

	// Configure interrupt for 1 percent changes in charge, active LOW
	interruptSetup();

	exitConfig();

	configured = true;

	return true;
}
float SckBatt::voltage()
{
	return readWord(GAUGE_COM_VOLTAGE) / 1000.0;
}
uint8_t SckBatt::percent()
{
	lastPercent = readWord(GAUGE_COM_SOC);
	return lastPercent;
}
int16_t SckBatt::current()
{
	return readWord(GAUGE_COM_CURRENT);
}
int16_t SckBatt::power()
{
	return readWord(GAUGE_COM_POWER);
}
uint8_t SckBatt::health()
{
	return readWord(GAUGE_COM_SOH);
}
uint16_t SckBatt::remainCapacity()
{
	return readWord(GAUGE_COM_REMAIN_CAPACITY);
}
bool SckBatt::enterConfig()
{
	// Unseal (We need to send the command twice)
	if (!(readControlWord(GAUGE_CTRL_SET_UNSEALED) && readControlWord(GAUGE_CTRL_SET_UNSEALED))) return false;

	// Enter config update mode and wait until the command is complete
	if (readControlWord(GAUGE_CTRL_SET_CONFIG_UPDATE)) {
		int16_t timeout = 2000;
		while ((timeout--) && (!(readWord(GAUGE_REG_FLAGS) & GAUGE_FLAGS_CONFIG_UPDATE))) delay(1);
		if (timeout < 1) return false;
	}

	return true;
}
bool SckBatt::exitConfig()
{
	// Soft Reset exits Config Update state
	if (!readControlWord(GAUGE_CTRL_SOFT_RESET)) return false;

	// Wait Config Update State change has been done
	int16_t timeout = 2000;
	while ((timeout--) && ((readWord(GAUGE_REG_FLAGS) & GAUGE_FLAGS_CONFIG_UPDATE))) delay(1);
	if (timeout < 1) return false;

	// Seal the device
	readControlWord(GAUGE_CTRL_SET_SEALED);

	return false;
}
bool SckBatt::setChemID(uint16_t wichID)
{
	if (readControlWord(wichID)){

		uint16_t profile = readControlWord(GAUGE_CTRL_GET_CHEM_ID);

		uint16_t desiredProfile = 0;
		if (wichID == GAUGE_CTRL_SET_CHEM_A) desiredProfile = 0x3230;
		else if (wichID == GAUGE_CTRL_SET_CHEM_B) desiredProfile = 0x1202;
		else if (wichID == GAUGE_CTRL_SET_CHEM_C) desiredProfile = 0x3142;

		if (profile == desiredProfile) return true;
	}

	return false;
}
bool SckBatt::interruptSetup()
{
	// (http://www.ti.com/lit/ug/sluubb0/sluubb0.pdf) page 38
	// Default opConfig = 0110010001111000
	// (no) changes:
	// GPIOPOL (bit 11) --> 0 (LOW level interrupt)
	// BATLOWEN (bit 2) --> 0 (enables SOC_INT)
	uint16_t newOPConfig = 0b0110010001111000;
	if (!setSubclass(GAUGE_CLASSID_OPCONFIG, opConfigOffset, newOPConfig)) return false;

	// (http://www.ti.com/lit/ug/sluubb0/sluubb0.pdf) page 50
	// SOC interrupt delta (if charge percent changes this or more it will raise an interrupt)
	// State class (82), offset 20.
	// new value: 1%
	if (!setSubclass(GAUGE_CLASSID_STATE, 20, 1)) return false;

	return true;
}
bool SckBatt::setSubclass(uint8_t subclassID, uint8_t offset, uint16_t value)
{
	uint8_t MSB = value >> 8;
	uint8_t LSB = value & 0x00FF;
	uint8_t toSendData[2] = {MSB, LSB};

	writeExtendedData(subclassID, offset, toSendData, 2);

	return (getSubclass(subclassID, offset) == value);
}
uint16_t SckBatt::getSubclass(uint8_t subclassID, uint8_t offset)
{
	uint8_t MSB = readExtendedData(subclassID, offset);
	uint8_t LSB = readExtendedData(subclassID, offset+1);

	return ((uint16_t)MSB << 8) | LSB;

}
uint16_t SckBatt::readWord(uint16_t subAddress)
{
	uint8_t data[2];
	i2cReadBytes(subAddress, data, 2);
	return ((uint16_t) data[1] << 8) | data[0];
}
uint16_t SckBatt::readControlWord(uint16_t function)
{
	uint8_t subCommandMSB = (function >> 8);
	uint8_t subCommandLSB = (function & 0x00FF);
	uint8_t command[2] = {subCommandLSB, subCommandMSB};
	uint8_t data[2] = {0, 0};
	
	i2cWriteBytes((uint8_t) 0, command, 2);
	
	if (i2cReadBytes((uint8_t) 0, data, 2)) return ((uint16_t)data[1] << 8) | data[0];
	
	return false;
}
bool SckBatt::writeExtendedData(uint8_t classID, uint8_t offset, uint8_t * data, uint8_t len)
{
	if (len > 32) return false;
	
	// Enable block data memory control
	if (!blockDataControl()) return false;

	// Write class ID using DataBlockClass()
	if (!blockDataClass(classID)) return false;
	
	// Write 32-bit block offset (usually 0)
	blockDataOffset(offset / 32);

	// Compute checksum going in
	computeBlockChecksum();

	// Write data bytes:
	for (int i = 0; i < len; i++) {
		// Write to offset, mod 32 if offset is greater than 32
		// The blockDataOffset above sets the 32-bit block
		writeBlockData((offset % 32) + i, data[i]);
	}
	
	// Write new checksum using BlockDataChecksum (0x60)
	uint8_t newCsum = computeBlockChecksum();
	writeBlockChecksum(newCsum);

	return true;
}
uint8_t SckBatt::readExtendedData(uint8_t classID, uint8_t offset)
{
	uint8_t retData = 0;
		
	// Enable block data memory control
	if (!blockDataControl()) return false;

	// Write class ID using DataBlockClass()
	if (!blockDataClass(classID)) return false;

	// Write 32-bit block offset (usually 0)
	blockDataOffset(offset / 32);
	
	// Compute checksum going in
	computeBlockChecksum();

	retData = readBlockData(offset % 32); // Read from offset (limit to 0-31)
	
	return retData;
}
bool SckBatt::blockDataControl()
{
	// Issue a BlockDataControl() command to enable BlockData access
	uint8_t enableByte = 0x00;
	return i2cWriteBytes(0x61, &enableByte, 1);
}
bool SckBatt::blockDataClass(uint8_t id)
{
	// Issue a DataClass() command to set the data class to be accessed
	return i2cWriteBytes(0x3E, &id, 1);
}
bool SckBatt::blockDataOffset(uint8_t offset)
{
	// Issue a DataBlock() command to set the data block to be accessed
	return i2cWriteBytes(0x3F, &offset, 1);
}
uint8_t SckBatt::computeBlockChecksum()
{
	uint8_t data[32];
	i2cReadBytes(0x40, data, 32);

	uint8_t csum = 0;
	for (int i=0; i<32; i++)
	{
		csum += data[i];
	}
	csum = 255 - csum;
	
	return csum;
}
bool SckBatt::writeBlockChecksum(uint8_t csum)
{
	// Use the BlockDataCheckSum() command to write a checksum value
	return i2cWriteBytes(0x60, &csum, 1);	
}
bool SckBatt::writeBlockData(uint8_t offset, uint8_t data)
{
	// Use BlockData() to write a byte to an offset of the loaded data
	uint8_t address = offset + 0x40;
	return i2cWriteBytes(address, &data, 1);
}
uint8_t SckBatt::readBlockData(uint8_t offset)
{
	// Use BlockData() to read a byte from the loaded extended data
	uint8_t ret;
	uint8_t address = offset + 0x40;
	i2cReadBytes(address, &ret, 1);
	return ret;
}
bool SckBatt::i2cWriteBytes(uint8_t subAddress, uint8_t * src, uint8_t count)
{
	Wire.beginTransmission(address);
	Wire.write(subAddress);
	for (int i=0; i<count; i++) Wire.write(src[i]);
	Wire.endTransmission(true);
	
	return true;	
}
bool SckBatt::i2cReadBytes(uint8_t subAddress, uint8_t * dest, uint8_t count)
{
	int16_t timeout = 2000;	
	Wire.beginTransmission(address);
	Wire.write(subAddress);
	Wire.endTransmission(true);
	
	Wire.requestFrom(address, count);
	while ((Wire.available() < count) && timeout--)
		delay(1);
	if (timeout) {
		for (int i=0; i<count; i++) dest[i] = Wire.read();
		return true;
	}
	
	return false;
}
