#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Pins.h>
#include "wiring_private.h"

class SckCharger
{
private:
	uint8_t address = 0x6B;
	uint8_t timeout = 10; // ms

	// Registers
	byte INPUT_SOURCE_CONTROL = 0; 			
	byte INPUT_CURR_LIMIT = 0;		// Bits 0:2 - > 000 – 100 mA, 001 – 150 mA, 010 – 500 mA, 011 – 900 mA, 100 – 1 A, 01 – 1.5 A, 110 - 2 A 111 - 3 A
	
	byte POWER_ON_CONF_REG = 1;
	byte BOOST_LIM = 0;			// Limit for boost mode. 0 – 1 A, 1 – 1.5 A
	byte CHG_CONFIG = 4;			// Charger Configuration. 0 - Charge Disable; 1- Charge Enable
	byte OTG_CONFIG = 5;			// Charger Configuration. 0 – OTG Disable; 1 – OTG Enable. OTG_CONFIG would over-ride Charge Enable Function in CHG_CONFIG
	byte I2C_WATCHDOG_TIMER_RESET = 6;	// I2C Watchdog Timer Reset. 0 – Normal; 1 – Reset
	byte RESET_DEFAULT_CONFIG = 7;		// 0 – Keep current register setting, 1 – Reset to default

	byte CHARGE_CURRENT_CONTROL = 2;
	byte CHARGE_CURRENT_LIMIT = 2;		// Limit charge current.(bits 2:6) Range: 512 – 2048mA (00000–11000), bit6 - 1024mA, bit5 - 512mA, bit4 - 256mA, bit3 - 128mA, bit2 - 64mA, (ej. 00000 -> 512mA, 00100 -> 768mA)

	byte CHG_TERM_TIMER_CTRL = 5;
	byte CHG_TIMER = 1;			// Fast Charge Timer Setting. (Two bits 1:2) 00 – 5 hrs, 01 – 8 hrs, 10 – 12 hrs, 11 – 20 hrs
	byte CHG_TIMER_ENABLE = 3;		// Charging Safety Timer Enable. 0 - Disable, 1 - Enable.
	byte I2C_WATCHDOG = 4;			// I2C Watchdog Timer Setting. (Two bits 4:5) 00 – Disable timer, 01 – 40 s, 10 – 80 s, 11 – 160 s
	byte CHG_TERM_ENABLE = 7;		// Charging Termination Enable. 0 - Disable, 1 - Enable.

	byte MISC_REG = 7;
	byte INT_MASK_BAT_FAULT = 0;		// Battery fault interrupt mask. 0 – No INT during CHRG_FAULT, 1 – INT on CHRG_FAULT
	byte INT_MASK_CHRG_FAULT = 1;		// CHarge fault interrupt mask. 0 – No INT during BAT_FAULT, 1 – INT on BAT_FAULT
	byte BATFET_DISABLE = 5; 		// Force BATFET Off. 0 - Allow BATFET (Q4) turn on, 1 - Turn off BATFET (Q4)
	byte DPDM_EN = 7;			// Force DPDM detection. 0 – Not in Force detection; 1 – Force detection when VBUS power is present

	byte SYS_STATUS_REG = 8;		// System Status Register
	byte PG_STAT = 2;			// Power Good status. 0 – Not Power Good, 1 – Power Good
	byte DPM_STAT = 3;			// DPM (detection) 0 - Not in DPM, 1 - On DPM
	byte CHRG_STAT = 4;			// (Two bits 4:5) 00 – Not Charging, 01 – Pre-charge (<VBATLOWV), 10 – Fast Charging, 11 – Charge Termination Done
	byte VBUS_STAT = 6;			// (Two bits 6:7) 00 – Unknown (no input, or DPDM detection incomplete), 01 – USB host, 10 – Adapter port, 11 – OTG

	byte NEW_FAULT_REGISTER = 9;		
	byte BAT_FAULT = 3;			// 0 – Normal, 1 – Battery OVP
	byte CHRG_FAULT = 4;			// (Two bits 4:5) 00 – Normal, 01 – Input fault (OVP or bad source), 10 - Thermal shutdown, 11 – Charge Timer Expiration
	byte OTG_FAULT = 6;			// 0 – Normal, 1 – VBUS overloaded in OTG, or VBUS OVP, or battery is too low (any conditions that cannot start boost function)
	byte WATCHDOG_FAULT = 7;		// 0 – Normal, 1- Watchdog timer expiration	

	byte readREG(byte wichRegister);
	bool writeREG(byte wichRegister, byte data);

public:

	enum ChargeStatus {
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

		VBUS_STATUS_COUNT
	};

	const char *chargeStatusTitles[CHRG_STATE_COUNT] PROGMEM = {
		"Not charging",
		"Pre charging",
		"Fast charging",
		"Charge finished",
	};

	const char *VBUSStatusTitles[VBUS_STATUS_COUNT] PROGMEM = {
		"Unknown",
		"USB host connected",
		"Adapter connected",
		"USB OTG connected",
	};

	const char *enTitles[2] PROGMEM = {
		"disabled",
		"enabled",
	};

	void setup();
	bool resetConfig();
	uint16_t inputCurrentLimit(int16_t current=-1);
	uint16_t chargerCurrentLimit(int16_t current=-1);
	bool chargeState(int8_t enable=-1);
	bool batfetState(int8_t enable=-1);
	bool OTG(int8_t enable=-1);
	uint8_t I2Cwatchdog(int16_t seconds=-1);
	uint8_t chargeTimer(int8_t hours=-1);
	bool getPowerGoodStatus();
	bool getDPMstatus();
	void forceInputCurrentLimitDetection();
	void event();
	ChargeStatus getChargeStatus();
	VBUSstatus getVBUSstatus();
	byte getNewFault();		// TODO
	bool onUSB = true;
	bool batfetON = false;
	ChargeStatus chargeStatus = CHRG_NOT_CHARGING;

};

class SckBatt
{
	// Parts of this code where taken from https://github.com/sparkfun/SparkFun_BQ27441_Arduino_Library
	// Thanks Sparkfun!!
	private:
		// Nominal Voltage of battery in volts
		const float nominalVoltage = 3.7;

		// Minimum operating voltage in mV.
		const uint16_t terminateVoltage = 2500;
		const uint8_t terminateVoltageOffset = 10;

		// Low limit of current flow at which charger stops charging cycle in mAh (wih added error margin)
		// Page 31 of http://www.ti.com/lit/ds/symlink/bq24259.pdf
		const uint16_t taperCurrent = 313;

		// Page 39 of http://www.ti.com/lit/ug/sluubb0/sluubb0.pdf
		// 00 = Chem ID 3230 is used.
		// 01 = Chem ID 1202 is used. (default batt of SCK)
		// 10 = Chem ID 3142 is used.
		// 11 = RSVD
		const byte chemID = 1;

		uint8_t designCapacityOffset = 6;

		// Design energy in mWh, page 49 of (http://www.ti.com/lit/ug/sluubb0/sluubb0.pdf)
		// designEnergy = designCapacity * nominalVoltage
		uint8_t designEnergyOffset = 8;

		// Taper Rate: used to sync full charge between gauge and charger
		// taperRate = designCapacity / (0.1 * taperCurrent)
		uint8_t taperRateOffset = 21;

		// OpConfig
		const uint8_t opConfigOffset 			= 0;
		const uint8_t opConfig_C_Offset			= 2;

		// Gauge Control() commands
	    	const uint16_t GAUGE_REG_CTRL_STATUS		= 0x00;
		const uint16_t GAUGE_CTRL_GET_CHEM_ID		= 0x0008;
		const uint16_t GAUGE_CTRL_SET_CHEM_A		= 0x0030; // Chem profile: 3230 - 4.35v charging voltage
		const uint16_t GAUGE_CTRL_SET_CHEM_B		= 0x0031; // Chem profile: 1202- 4.2v charging voltage (default for SCK)
		const uint16_t GAUGE_CTRL_SET_CHEM_C		= 0x0032; // Chem profile: 3142 - 4.4v charging voltage

		const uint16_t GAUGE_CTRL_SET_CONFIG_UPDATE 	= 0x0013;
		const uint16_t GAUGE_CTRL_SET_UNSEALED 		= 0x8000;
		const uint16_t GAUGE_CTRL_SET_SEALED 		= 0x0020;
		const uint16_t GAUGE_CTRL_SOFT_RESET 		= 0x0042;

		const uint16_t GAUGE_REG_FLAGS 			= 0x06;
		const uint16_t GAUGE_FLAGS_CONFIG_UPDATE 	= 4;
	
		const uint8_t GAUGE_CLASSID_STATE 		= 82;
		const uint8_t GAUGE_CLASSID_OPCONFIG 		= 64;

		const uint8_t GAUGE_COM_VOLTAGE 		= 0x04;
		const uint8_t GAUGE_COM_SOC 			= 0x1C;
		const uint8_t GAUGE_COM_CURRENT 		= 0x10;
		const uint8_t GAUGE_COM_POWER 			= 0x18;
		const uint8_t GAUGE_COM_SOH 			= 0x20;
		const uint8_t GAUGE_COM_REMAIN_CAPACITY		= 0x2A;

		bool enterConfig();
		bool exitConfig();
		bool setChemID(uint16_t wichID);
		bool interruptSetup();
		bool setSubclass(uint8_t subclassID, uint8_t offset, uint16_t value);
		uint16_t getSubclass(uint8_t subclassID, uint8_t offset);
		uint16_t readWord(uint16_t subAddress);
		uint16_t readControlWord(uint16_t function); 	// param function is the subcommand of control() to be read
		bool writeExtendedData(uint8_t classID, uint8_t offset, uint8_t * data, uint8_t len);
		uint8_t readExtendedData(uint8_t classID, uint8_t offset);
		bool blockDataControl();
		bool blockDataClass(uint8_t id);
		bool blockDataOffset(uint8_t offset);
		uint8_t computeBlockChecksum(void);
		bool writeBlockData(uint8_t offset, uint8_t data);
		uint8_t readBlockData(uint8_t offset);
		bool writeBlockChecksum(uint8_t csum);
		bool i2cWriteBytes(uint8_t subAddress, uint8_t * src, uint8_t count); 		// Write a specified number of bytes over I2C to a given subAddress
		bool i2cReadBytes(uint8_t subAddress, uint8_t * dest, uint8_t count); 	// Read a specified number of bytes over I2C at a given subAddress
		
		uint8_t address = 0x55;
		bool configured = false;
	public:
		const uint8_t threshold_low = 10;
		const uint8_t threshold_emergency = 2;
		uint8_t lowBatCounter = 0;
		uint8_t emergencyLowBatCounter = 0;

		bool present = false;
		uint8_t lastPercent = 0;
		// Design capacity in mAh, page 49 of (http://www.ti.com/lit/ug/sluubb0/sluubb0.pdf)
		uint16_t designCapacity = 2000; 	// Don't change this default here, change it in Config.h. This will be overwriten by config value

		bool setup(SckCharger charger, bool force=false);
		bool isPresent(SckCharger charger);
		float voltage();
		int16_t current();
		int16_t power(); 	// Negative during discharge, positive when charging, (mWh)
		uint8_t percent();
		uint8_t health();
		uint16_t remainCapacity();

};

void ISR_charger();
void ISR_battery();
