#include "Sensors.h"

SensorType AllSensors::getTypeFromString(String strIn)
{

	SensorType wichSensor = SENSOR_COUNT;
	uint8_t maxWordsFound = 0;

	// Get sensor type
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType thisSensor = static_cast<SensorType>(i);

		// Makes comparison lower case and not strict
		String titleCompare = list[thisSensor].title;
		titleCompare.toLowerCase();
		strIn.toLowerCase();

		// How many words match in Sensor title
		uint8_t matchedWords = countMatchedWords(titleCompare, strIn);

		if (matchedWords > maxWordsFound) {
			maxWordsFound = matchedWords;
			wichSensor = thisSensor;
		}

	}
	return wichSensor;
}
uint8_t AllSensors::countMatchedWords(String baseString, String input)
{
	
	uint8_t foundedCount = 0;
	String word;
	
	while (input.length() > 0) {

		// Get next word
		if (input.indexOf(" ") > -1) word = input.substring(0, input.indexOf(" "));
		// Or there is only one left
		else word = input;

		// If we found one
		if (baseString.indexOf(word) > -1) foundedCount += 1;
		// If next word is not part of the title we asume the rest of the input is a command or something else
		else break;

		// remove what we tested
		input.replace(word, "");
		input.trim();
	}

	return foundedCount;
}
String AllSensors::removeSensorName(String strIn)
{
	SensorType wichSensor = getTypeFromString(strIn);

	// Makes comparison lower case and not strict
	String titleCompare = list[wichSensor].title;
	titleCompare.toLowerCase();
	strIn.toLowerCase();

	uint8_t wordsToRemove = countMatchedWords(titleCompare, strIn);
	for (uint8_t i=0; i<wordsToRemove; i++) {
		if (strIn.indexOf(" ") > 0) strIn.remove(0, strIn.indexOf(" ")+1);
		else strIn.remove(0, strIn.length());
	}

	return strIn;
}
SensorType AllSensors::sensorsPriorized(uint8_t index)
{
	if (!sorted) {
		uint8_t sensorCount = 0;
		for (uint8_t i=0; i<251; i++) {
			for (uint8_t ii=0; ii<SENSOR_COUNT; ii++) {
				SensorType thisSensorType = static_cast<SensorType>(ii);
				if (list[thisSensorType].priority == i) {
					prioSortedList[sensorCount] = thisSensorType;
					sensorCount++;
				}
			}
		}
		sorted = true;
	}
	return prioSortedList[index];
}
