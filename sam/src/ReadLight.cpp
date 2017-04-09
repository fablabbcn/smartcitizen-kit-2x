
/*  TODO Organize and translate to english !!
 * 
El sistema detecta los cambios de color, de esta manera no es necesaria una señal de reloj ni ninguna sincronización entre el kit y la pantalla. La señal se puede enviar a tan lento como se quiera, el único limite es que no puede ir más rápido que la velocidad de muestreo que tiene el sensor de luz. De hecho debe ir a una velocidad de alrededor de 3 veces la velocidad de muestreo del sensor, el cual para aceptar una lectura como válida debe capturarla por lo menos 2 veces.

Los colores a usar van de 0 (negro) a levelnum (blanco) el blanco se utiliza para indicar una repetición de el color anterior. La sensibilidad de el sensor disminuye mientras más rápido sea el muestreo y aumenta con un muestreo lento. Después de bastantes pruebas creo que un buen balance entre robustez y velocidad es usar una velocidad de refresh de 70ms y 9 niveles de color, lo cual permite enviar alrededor de 5 bytes/seg.

La escala de grises que se utiliza debe ser lineal para que la diferencia entre cada nivel sea la misma (o lo mas cercano posible a esto). El problema es que los monitores realizan una corrección de gama diseñada para que el ojo humano, cuya sensibilidad no es lineal (vemos mucho más detalle en los tonos obscuros). Para que nuestro sensor de luz reciba una escala lineal tenemos que revertir esta corrección:

 	math.pow(value, (1.0 / gamma))

 	donde value va de 0 (negro) 1 (blanco)

La gamma correction varia de sistema a sistema, normalmente en un rango desde 1.8 (mac) hasta 2.2 (win) para aproximarnos lo mas posible a ambos sistemas usamos 2.0
https://en.wikipedia.org/wiki/Gamma_correction

Los valores posibles se manejan asi:
	levelNum 	= número total de niveles posibles.
	REPEAT 		= el último valor en levelNum (blanco)
	MAX			= el ultimo valor aceptado (uno menos que levelNum)
	MIN 		= el primer valor aceptado (negro)

los colores posibles si usamos una escala de 8 valores son:
	0,1,2,3,4,5,6,7  --  son los valores aceptados
	8  --  es el valor de repeticion REPEAT

Para repetir un valor se utiliza el color MAX (blanco):
	0-8 	= 0 0
	0-8-0 	= 0 0 0

La secuencia de inicio INIT (en la cual se incluye una de calibrado) es:
	MAX-REPEAT-RAMP(levelNum)-MIN-REPEAT-MIN-REPEAT
	780123456780808

Si pasa DOG_TIMEOUT (en milisegundos) sin recibir nada se reinicia el proceso y se pone en espera del ciclo de INIT (y calibrado).

Despues de esto se envia STX (start text) seguido de el mensaje en ASCII en sistema octal con tres digitos para cada caracter, sólo se incluye el leading zero para completar los tres digitos para cada caracter enviado.

Para poder integrar más comandos en el futuro por esta vía se usa un caracter newLine (\n) octal - 012, para separar el commando, y los diferentes parametros. Para terminar el texto se envia ETX (end of text) octal - 003 después el CRC y al final EOT (end of transmission) octal - 004.
De esta manera queda:
	STXcomando\nparametro1\nparametro2\nparametroN\nETXCRCEOT

Para el comando de envio de credenciales, al que llamaremos auth:

	INITSTXauth\nmySSID\nmyPASS\nmyTOKEN\nETXCRCEOT

El CRC debe incluir  el commando + los parametros, (todo lo que se encuentre entre STX y ETX) lo cual no debe ser mayor a 1024 bytes.


FLOW
1. El usuario captura el ssid y password en un formulario al lado de una imagen que le indica donde colocar el sck en la pantalla.
2. Coloca el SCK pegado a la pantalla en la imagen y pica espacio o un mouse click o lo que sea... y el javascript hace lo siguiente:
	1. Envia la secuencia de inicio seguida de el mensaje, crc, etc
	2. Si la recepcion de el mensaje es exitosa se da feedback con el led (verde?)
	3. Enseguida se intenta la conexión al wifi y si es exitosa se sale de apmode y se da feedback con el led (azul) además de avisar a la plataforma via MQTT para que con un webSocket el usuario reciba aviso de que el sync fue exitoso.
	4. A partir de aqui la configuración e interaccion con el kit sera atraves de la plataforma via MQTT.
 */

#include "ReadLight.h"

//Setup must be executed before read
void ReadLight::setup() {
  // Wire.begin();
  reset();
  feedDOG();          // Start timer for watchdog

  debugFlag = false;   // Remove for production

  // Set values for light readings (only once)
  uint8_t DATA [8] = {0x07, TIME0, 0x00 ,0x00, 0x00, 0xFF, 0xFF ,0x00} ;

  Wire.beginTransmission(BH1730);
  Wire.write(0x80|0x00);
  for(int i= 0; i<8; i++) Wire.write(DATA[i]);
  Wire.endTransmission();
  delay(20);
  Wire.beginTransmission(BH1730);
  Wire.write(0x94);
  Wire.endTransmission();
}

/*
 *  Starts the monitoring of light sensor and process data
 */
dataLight ReadLight::read() {

    if (!calibrated) calibrate();
    else {
      
      if (dogBite()) return results;
      
      if (!ETX) {
        // Get char
        char b = getChar();

        // Start of text
        if (b == 0x02) {
          debugOUT(F("STX received!!!"));
          localCheckSum = 0;
          TransmittingText = true;
          feedDOG();
        } else if (b == 0x03) {
          // End of text signal
          debugOUT(F("ETX received!!!"));
          ETX = true;
          TransmittingText = false;
          feedDOG();
        } else if (b == 0x04) {
          // End of transfer
          debugOUT(F("EOT received!!!"));
          EOT = true;
          feedDOG();
        } else if (TransmittingText && b != 0x00) {
          // If received char its not a newline, we store it in buffer
          if (b != 0x0A) {
            results.lines[results.lineIndex].concat(b);
            feedDOG();
          } else {
            // If we receive a new line store it in results and restart buffer
            debugOUT(results.lines[results.lineIndex]);
            results.lineIndex++;
            feedDOG();
          }
        } 
      } else {
        // Verify checksum and finish
        if (checksum()) return results;
      }
    }
  return results;
}

/*
 *  Calibrates level color, creating a table with valid level values from 0 to levelNum.
 *  For this process to work we need the screen to send colors from black to white with all grey levels one by one.
 *  @return True if succsesfull calibration.
 */
bool ReadLight::calibrate() {

  feedDOG();

  // If we are no ready for a new reading return
  if (!getLight()) return false;

  // If the new value difference from previous is less than tolerance we use it.
  if (abs(newReading - OldReading) <= (float)(newReading / (newLevel + 1) / 3.0)) readingRepetitions++;
  else readingRepetitions = 0;
  
  // Save old reading for future comparisons
  OldReading = newReading;

  // We need MIN_REP repetitions in reading to consider it valid.
  if (readingRepetitions == MIN_REP) newValue = newReading;
  else return false;
  
  // If value is bigger than previous go up one level
  if (newValue > oldValue) {
    newLevel ++;
    levels[newLevel] = newValue;
    oldValue = newValue;
    return false;                       // Not yet there!
  } else {
    // If we reach the levelnum we are done!
    if (newLevel + 1 == levelNum) {

      debugOUT(F("Calibrated!!"));
      calibrated = true;
      for (int i = 0; i<levelNum; i++) debugOUT(String(i) + F(": ") + String(levels[i]));

      // Keep the watchdog timer cool
      feedDOG();
      return true;
    } 

    // no calibration sequence found, return false
    oldValue = newValue;
    newLevel = 0;
    return false;
  }
  return false;
}

/*
 *   Gets a raw reading from BH1730 light sensor. The resolution and speed depends on TIME0
 *   @return raw reading of the light sensor
 */
bool ReadLight::getLight() {
 /*    
  * TIME0 posible values, more time = more resolution
  * 0xA0 (~3200 values in 260 ms)
  * 0xB0 (~2100 values in 220 ms)
  * 0xC0 (~1300 values in 180 ms)
  * 0xD0 (~800 values in 130 ms)
  * 0xE0 (~350 values in 88 ms)
  * 0xF0 (~80 values in 45 ms)
  * 0xFA (12 values in 18 ms)
  * 0XFB (8 values in 15 ms)
  * 
  * The best working option is 0xFB: using 9 values and printing output from screen at intervals of 70ms
  */

  if (millis() - readyTimer > 20) {
    readyTimer = millis();
    Wire.requestFrom(BH1730, 4);
    uint16_t DATA0 = Wire.read();
    DATA0=DATA0|(Wire.read()<<8);
    newReading = DATA0;
    return true;
  } else {
    return false;
  }
}

/* 
 *  Gets a char from 3 valid values obtained from light sensor
 *  @return obtained char
 */
char ReadLight::getChar() {

  if (getLevel()) {

    // Exception for clearing INIT chars 0808
    if (!TransmittingText) {
      if (newLevel == 2) {
        octalString = "0"; //reset Container String
        return 0x02;
      }  
    }
    

    octalString.concat(newLevel);

    if (octalString.length() > 3) {

      int newInt = strtol(octalString.c_str(), NULL, 0);
      if (TransmittingText) localCheckSum = localCheckSum + newInt;
      octalString = "0"; //reset Container String

      // Convert ASCII to char
      char newChar = newInt;

      debugOUT(String F("  ") + String(newChar));

      // Return the obtained char
      return newChar;
    }
  }
  return 0x00;
}

/*
 *  Gets a valid level from a lightsensor value between the grey levels
 *  @return a grey level
 */
bool ReadLight::getLevel() {

  // If we are not yet ready for sensor reading... 
  if (!getRawLevel()) return false;
  else {

    // If we do get a reading and is different from previous, start counting repetitions
    if (newLevel != oldLevel) {
      oldLevel = newLevel;
      levelRepetitions = 1;
      return false;

    } else levelRepetitions++;
  }

  // If we have MIN_REP repetitions, we have a good reading!
  if (levelRepetitions == MIN_REP) {

    debugOUT(String(newLevel), false);

    if (newLevel+1 == levelNum) {
      newLevel = lastGoodLevel;
    } 

    // Store the new level for future comparisons
    lastGoodLevel = newLevel;
    
    return true;

  } else {
    return false;
  }
}

bool ReadLight::getRawLevel() {

  // Get light reading from sensor
  if (!getLight()) return false;

  // Find the level of this reading
  newLevel = 0;
  uint16_t minDiff = abs(levels[newLevel] - newReading);
  for (uint8_t i=1; i<levelNum; ++i) {
    uint16_t thisDiff = abs(levels[i] - newReading);
    if (thisDiff < minDiff) {
      minDiff = thisDiff;
      newLevel = i;
    }
  }

  return true;
}

/*
 *  Gets the checksum from sender and compare it with the calculated from received text.
 *  @return True is checksum is OK, False otherwise
 */
bool ReadLight::checksum(){
  
  if (!getLevel()) return false;

  sum.concat(newLevel);
  feedDOG();

  // Gets checksum digits (6) sended from the screen
  if (sum.length() > 5) {

    int receivedInt = strtol(sum.c_str(), NULL, 8);

    if (receivedInt == localCheckSum - 3) {
      debugOUT(F("  Checksum OK!!"));
      results.ok = true;
      return true;      
    } else {
      debugOUT(F("  Checksum ERROR!!"));
      reset();
    }
  }
  return false;
}

/*
 *  Avoid the watchdog timer to trigger a restart()
 */
void ReadLight::feedDOG() {
  watchDOG = millis();
}

/*
 *  Check if we need a restart
 *  @return True if timeout has been reached
 */
bool ReadLight::dogBite() {
  if (millis() - watchDOG > DOG_TIMEOUT) {
    debugOUT(F("watchdog timeout!!"));
    reset();
    return true;
  }
  return false;
} 

void ReadLight::reset() {
  debugOUT(F("Resetting readlight"));
  calibrated = false;
  newValue = 0;
  oldValue = 0;
  newLevel = 0;
  oldLevel = -1;
  octalString = "0";
  sum = "";
  EOT = false;
  ETX = false;
  TransmittingText = false;
  for (uint8_t i=0; i<8; ++i) results.lines[i] = "";
  results.ok = false;
  results.lineIndex = 0;
}

void ReadLight::debugOUT(String debugText, bool newLine) {

  if (debugFlag) {
    if (newLine) SerialUSB.println(debugText);
    else SerialUSB.print(debugText);
  }
}