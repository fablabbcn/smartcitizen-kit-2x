#include "SckPot.h"

uint32_t SckPot::getValue() {
	Wire.beginTransmission(address);
	Wire.write(channel);
	Wire.endTransmission();
	Wire.requestFrom(address, 1);
	unsigned long time = millis();
	while (!Wire.available()) if ((millis() - time) > 50) return 0x00;
	byte data = Wire.read();
	return data * ohmsPerStep;
}
bool SckPot::setValue(float value) {
	int data=0x00;
	if (value>100000) value = 100000;
	data = (int)(value/ohmsPerStep);
	Wire.beginTransmission(address);
	Wire.write(channel);
	Wire.write(data);
	uint8_t result = Wire.endTransmission();
	if (result == 0) return true;
	return false;
}
// void SckBase::writeI2C(byte deviceaddress, byte address, byte data ) {
//   Wire.beginTransmission(deviceaddress);
//   Wire.write(address);
//   Wire.write(data);
//   Wire.endTransmission();
//   delay(4);
// }

// byte SckBase::readI2C(int deviceaddress, byte address) {
//   Wire.beginTransmission(deviceaddress);
//   Wire.write(address);
//   Wire.endTransmission();
//   Wire.requestFrom(deviceaddress,1);
//   if (Wire.available() != 1) return 0x00;
//   byte data = Wire.read();
//   return data;
// } 

// float SckBase::readResistor(byte resistor) {
//    byte POT = POT1;
//    byte ADDR = resistor;
//    if ((resistor==2)||(resistor==3))
//      {
//        POT = POT2;
//        ADDR = resistor - 2;
//      }
//    else if ((resistor==4)||(resistor==5))
//      {
//        POT = POT3;
//        ADDR = resistor - 4;
//      }
//    else if ((resistor==6)||(resistor==7))
//      {
//        POT = POT4;
//        ADDR = resistor - 6;
//      }
//    return readI2C(POT, ADDR)*ohmsPerStep;
// }

// void SckBase::writeResistor(byte resistor, float value ) {
//    byte POT = POT1;
//    byte ADDR = resistor;
//    int data=0x00;
//    if (value>100000) value = 100000;
//    data = (int)(value/ohmsPerStep);
//    if ((resistor==2)||(resistor==3))
//      {
//        POT = POT2;
//        ADDR = resistor - 2;
//      }
//    else if ((resistor==4)||(resistor==5))
//      {
//        POT = POT3;
//        ADDR = resistor - 4;
//      }
//    else if ((resistor==6)||(resistor==7))
//      {
//        POT = POT4;
//        ADDR = resistor - 6;
//      }
//    writeI2C(POT, ADDR, data);
// }
