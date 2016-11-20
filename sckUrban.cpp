#include "sckUrban.h"

uint8_t pot_6_db_preset[] = {0,  7, 26};
uint8_t pot_7_db_preset[] = {0, 17, 97};

void SckUrban::setup() {

//void SckUrban::setup() {
  digitalWrite(IO0, LOW); 		// Turn off CO Sensor Heather
  digitalWrite(IO1, LOW); 		// Turn off NO2 Sensor Heater


  // TEST TEMP
  ADCini();
};


  ////TEMP
void SckUrban::ADCini()
  {
    byte temp = readI2C(ADC_DIR,0)&B00000000;
    //temp = temp|B11000000;
    writeI2C(ADC_DIR, 0, temp);
  }


float SckUrban::GetNoise() {
  uint16_t soundraw = 0;
  // uint8_t section = 0;
  boolean validReading = 0;

  while (!validReading) {
    switch (gain_step) {
      case 0:
        soundraw = analogRead(S4);
        if (soundraw <= 757)
        {
          sounddB = 9.0f * log10f(((float)(analogRead(S4) + 1))/RESOLUTION_ANALOG*VCC) + 40;
          validReading = 1;
        }
        else gainChange(1);
      break;
      case 1:
        soundraw = analogRead(S4);
        if ((soundraw >= 6) & (soundraw <= 645))
        {
          sounddB = 12.0f * log10f(((float)(analogRead(S4) + 1))/RESOLUTION_ANALOG*VCC) + 58;
          validReading = 1;
        }
        else if (soundraw < 6) gainChange(0);
        else if (soundraw > 645) gainChange(2);
      break;
      case 2:
        soundraw = analogRead(S4);
        if ((soundraw >= 13) & (soundraw <= 1203))
        {
          sounddB = 11.0f * log10f(((float)(analogRead(S4) + 1))/RESOLUTION_ANALOG*VCC) + 76;
          validReading = 1;
        }
        else if (soundraw < 13) gainChange(1);
        else if (soundraw > 1203) {
          sounddB = 45.0f * log10f(((float)(analogRead(S4) + 1))/RESOLUTION_ANALOG*VCC) - 16;
          validReading = 1;
        }
      break;
    }
  }
return sounddB;
}

void SckUrban::gainChange(uint8_t value)
{
  writeResistorRaw(6, pot_6_db_preset[value]);
  writeResistorRaw(7, pot_7_db_preset[value]);
  if (gain_step < value) delay(250); // la carga del condensador que filtra la señal rectificada es inmediata, la descarga no
                              // por lo que si aumentamos la ganancia el voltaje sera mayor y no hemos de esperar tanto, si
                              // disminuimos la ganancia hemos de esperar mas a que el condensador se descargue
                              // value = 0 G = 76dB
                              // value = 1 G = 34dB
                              // value = 2 G = 17dB
  gain_step = value;
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

void SckUrban::writeI2C(byte deviceaddress, byte address, byte data) {
  Wire.beginTransmission(deviceaddress);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
  delay(4);
}

byte SckUrban::readI2C(int deviceaddress, byte address) {
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



/*Sensor temperature*/
  
uint16_t SckUrban::readSHT(uint8_t type){
      uint16_t DATA = 0;
      Wire.beginTransmission(SHT21_DIR);
      Wire.write(type);
      Wire.endTransmission();
      Wire.requestFrom(SHT21_DIR,3);
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


// void SCKDriver::getSHT()
//    {
//         *__Temperature = (-46.85 + (175.72*(readSHT(0xE3)/65536.0)));
//         *__Humidity    = (-6 + (125*(readSHT(0xE5)/65536.0)));
//     }