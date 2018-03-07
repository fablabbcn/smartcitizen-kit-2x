#pragma once

#include <Arduino.h>
#include <Wire.h>

enum chargeStatus {
	CHRG_NOT_CHARGING,
	CHRG_PRE_CHARGE,
	CHRG_FAST_CHARGING,
	CHRG_CHARGE_TERM_DONE,

	CHRG_STATE_COUNT
};

enum VBUSstatus {
	VBUS_UNKNOWN,
	VBUS_USB_HOST,
	VBUS_ADAPTER_PORT,
	VBUS_OTG,	
};

class SckCharger {
private:
	uint8_t address = 0x6B;

	uint8_t timeout = 10; // ms

	// Registers
	byte POWER_ON_CONF_REG = 0x01;
	byte CHG_CONFIG = 4;
	byte OTG_CONFIG = 5;
	byte RESET_DEFAULT_CONFIG = 7;

	byte SYS_STATUS_REG = 0x08;
	byte PG_STAT = 2;
	byte CHRG_STAT = 4;	// 2 bytes
	byte VBUS_STAT = 6;	// 2 bytes

	byte NEW_FAULT_REGISTER = 0x09;
	
	int8_t readREG(byte wichRegister);
	bool writeREG(byte wichRegister, byte data);

public:
	const char *chargeStatusTitles[CHRG_STATE_COUNT] PROGMEM = {
		"Not charging",
		"Pre charging",
		"Fast charging",
		"Charge finished",
	};

	const char *VBUSStatusTitles[CHRG_STATE_COUNT] PROGMEM = {
		"Unknown",
		"USB host connected",
		"Adapter connected",
		"USB OTG connected",
	};

	void setup();
	bool setCharge(bool enable);
	bool getCharge();
	bool setOTG(bool enable);
	bool getOTG();
	bool resetConfig();
	bool getPowerGoodStatus();
	chargeStatus getChargeStatus();
	VBUSstatus getVBUSstatus();
	const char* chargeStatusTitle();
	const char* VBUSstatusTitle();
	void getNewFault();
	void isr();

};