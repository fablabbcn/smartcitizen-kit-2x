#include "Sensors.h"

SensorType AllSensors::getTypeFromText(const char* input)
{
    SensorType whichSensor = SENSOR_COUNT;
    uint8_t maxWordsFound = 0;

    // Iterate over all posible sensor types
    for (uint8_t i=0; i<SENSOR_COUNT; i++) {

        // Get the title
        SensorType thisSensor = static_cast<SensorType>(i);

        // How many words match in Sensor title
        uint8_t matchedWords = countMatchedWords(thisSensor, input);

        // Keep the one that matched more words
        if (matchedWords > maxWordsFound) {
            maxWordsFound = matchedWords;
            whichSensor = thisSensor;
        }
    }

    return whichSensor;
}
uint8_t AllSensors::countMatchedWords(SensorType whichSensor, const char* input)
{
    uint8_t matched = 0;

    // Make a copy of sensor title in lowercase
    size_t titleLen = strlen(list[whichSensor].title) + 1;
    char titleCompare[titleLen];
    for (uint8_t i=0; i<titleLen; i++) titleCompare[i] = tolower(list[whichSensor].title[i]);

    // Make input lowercase
    size_t inputLen = strlen(input) + 1;
    char inputLow[inputLen];
    for (uint8_t i=0; i<inputLen; i++) inputLow[i] = tolower(input[i]);

    // Split the input in words, and check if they exist on the sensor title
    char* pch;
    char* rest = NULL;
    pch = strtok_r(inputLow," ", &rest);
    while (pch != NULL) {
        if (strstr(titleCompare, pch) == NULL) break;
        matched++;
        pch = strtok_r(NULL, " ", &rest);
    }

    return matched;
}
uint8_t AllSensors::sensorNameEndsIn(const char* input)
{
    // Get sensor type
    SensorType whichSensor = getTypeFromText(input);

    // Count how many words to remove
    uint8_t wordsToRemove = countMatchedWords(whichSensor, input);

    // Find the index where sensor name ends
    char *ptr = strchr(input, ' ');
    uint8_t index = ptr - input;
    uint8_t removed = 0;
    while (removed < wordsToRemove) {
        ptr = strchr(ptr, ' ');
        index = (ptr - input);
        ptr++;
        removed++;
    }
    index++;

    return index;
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
