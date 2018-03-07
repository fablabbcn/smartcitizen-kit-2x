#include "SckCharger.h"

bool SckCharger::setCharge(bool enable) {
	byte conf = readREG(POWER_ON_CONF_REG);

	if (enable) bitSet(conf, CHG_CONFIG);
	else bitClear(conf, CHG_CONFIG);

	if (writeREG(POWER_ON_CONF_REG, conf)) return true;
	else return false;
}
bool SckCharger::getCharge() {

	return bitRead(readREG(POWER_ON_CONF_REG), CHG_CONFIG);
}
bool SckCharger::setOTG(bool enable) {
	byte conf = readREG(POWER_ON_CONF_REG);

	if (enable) bitSet(conf, OTG_CONFIG);
	else bitClear(conf, OTG_CONFIG);

	if (writeREG(POWER_ON_CONF_REG, conf)) return true;
	else return false;
}
bool SckCharger::getOTG() {

	return bitRead(readREG(POWER_ON_CONF_REG), OTG_CONFIG);
}
bool SckCharger::resetConfig() {

	byte conf = readREG(POWER_ON_CONF_REG);

	bitSet(conf, RESET_DEFAULT_CONFIG);

	if (writeREG(POWER_ON_CONF_REG, conf)) return true;
	else return false;
}
bool SckCharger::getPowerGoodStatus() {

	byte status = readREG(SYS_STATUS_REG);

	byte pgStat = (status >> PG_STAT) & 1;

	if (pgStat) return true;
	else return false;
}
chargeStatus SckCharger::getChargeStatus() {

	byte status = readREG(SYS_STATUS_REG);
	status = (status >> CHRG_STAT) & 2;
	return static_cast<chargeStatus>(status);
}
VBUSstatus SckCharger::getVBUSstatus() {

	byte status = readREG(SYS_STATUS_REG);
	status = (status >> VBUS_STAT) & 2;
	return static_cast<VBUSstatus>(status);
}
const char* SckCharger::VBUSstatusTitle() {

	return VBUSStatusTitles[getVBUSstatus()];
}
const char* SckCharger::chargeStatusTitle() {

	return VBUSStatusTitles[getChargeStatus()];	
}
void SckCharger::getNewFault() {

	byte fault = readREG(NEW_FAULT_REGISTER);
	SerialUSB.println(fault, BIN);
}
int8_t SckCharger::readREG(byte wichRegister) {
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
bool SckCharger::writeREG(byte wichRegister, byte data) {

	Wire.beginTransmission(address);
	Wire.write(wichRegister);
    Wire.write(data);
    Wire.endTransmission();

    if (readREG(wichRegister) == data) return true;
    else return false;
}



// void SckBase::chargerEvent() {
// 	SerialUSB.println("charger event!!!");
// 	SerialUSB.println(digitalRead(pinCHARGER_INT));
// 	led.update(led.YELLOW, led.PULSE_STATIC);
// }