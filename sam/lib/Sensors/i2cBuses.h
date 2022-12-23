#pragma once

struct I2cBus
{
	TwoWire * _wire;
	const char * name;
};

// SCK auxiliary I2C interface
extern TwoWire auxWire;

// Number of available I2C buses (max 4)
static const uint8_t i2cBuses_num = 2;

const I2cBus i2cBuses[i2cBuses_num] = { 
	{ &Wire, "Wire" }, 
	{ &auxWire, "AuxWire" }
};
