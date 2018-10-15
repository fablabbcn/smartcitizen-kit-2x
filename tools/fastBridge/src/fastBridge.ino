
#define SERIAL_SPEED 921600
#define pinRED 6
#define pinGREEN 12
#define pinBLUE 10

#define pinBUTTON 7

#define pinESP_CH_PD 15
#define pinPOWER_ESP 30
#define pinESP_GPIO0 16

/* uint32_t flashTimeout = millis(); */
/* uint32_t startTimeout = millis(); */

void ISR_button()
{
	NVIC_SystemReset();
}

void setup()
{
	// LED setup
	pinMode(pinRED, OUTPUT);
	pinMode(pinGREEN, OUTPUT);
	pinMode(pinBLUE, OUTPUT);

	digitalWrite(pinRED, LOW);
	digitalWrite(pinGREEN, LOW);
	digitalWrite(pinBLUE, LOW);

	//ESP setup
	pinMode(pinESP_CH_PD, OUTPUT);
	pinMode(pinPOWER_ESP, OUTPUT);
	pinMode(pinESP_GPIO0, OUTPUT);
	
	digitalWrite(pinESP_CH_PD, LOW);
	digitalWrite(pinPOWER_ESP, HIGH);
	digitalWrite(pinESP_GPIO0, LOW);	// LOW for flash mode
	delay(100);
	digitalWrite(pinESP_CH_PD, HIGH);
	digitalWrite(pinPOWER_ESP, LOW);

	pinMode(pinBUTTON, INPUT_PULLUP);
	attachInterrupt(pinBUTTON, ISR_button, CHANGE);

	SerialUSB.begin(SERIAL_SPEED);
	SerialESP.begin(SERIAL_SPEED);

	while (SerialUSB.available()) SerialUSB.read();
	while (SerialESP.available()) SerialESP.read();
}

void loop()
{
	if (SerialUSB.available()) {
		SerialESP.write(SerialUSB.read());
		/* flashTimeout = millis();	// If something is received restart timer */
	}
	if (SerialESP.available()) {
		SerialUSB.write(SerialESP.read());
	}
	/* if (millis() - flashTimeout > 3000) { */
		/* if (millis() - startTimeout > 5000) NVIC_SystemReset(); */
	/* } */
}
