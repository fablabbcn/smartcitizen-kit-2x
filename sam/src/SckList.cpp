#include "SckList.h"

bool SckList::append(char value)
{
	// Writes value and updates de index
	if (write(index, value)) index++;
	else return false;

	return true;
}
bool SckList::write(uint32_t wichIndex, char value)
{
	if (wichIndex < SCKLIST_RAM_SIZE && !usingFlash)  {

		// Use ram to store the value
		ramBuff[wichIndex] = value;

	} else {

		return false;

		// TODO finish flash support and enable this

		// If we haven't yet, move RAM readings to flash and update the index
		/* if (!usingFlash) migrateToFlash(); */

		// Use flash to store the value
		/* flashSelect(); */
		/* if (!flash.writeByte(wichIndex, value)) return false; */

		// TODO
		// Whe migrating to flash do a sector scan to see where to start putting readings
	}
	return true;
}
char SckList::read(uint32_t wichIndex)
{
	if (!usingFlash)  {

		// Get value from RAM memory
		return ramBuff[wichIndex];

	} else {

		// Get the value from flash
		flashSelect();
		char returnValue = (char)flash.readByte(wichIndex);
		return returnValue;
	}
}
uint32_t SckList::getGroupRightIndex(uint32_t wichGroup)
{
	if (wichGroup > totalGroups) return 0;

	uint32_t groupCounter = 0;
	uint32_t thisIndex = lastGroupRightIndex;

	while (groupCounter < wichGroup) {
		thisIndex = thisIndex - readGroupSize(thisIndex);
		groupCounter++;
	}

	return thisIndex;
}
uint32_t SckList::getGroupLeftIndex(uint32_t wichGroup)
{
	// TODO que hacer respecto al error reporting porque retornar 0 es valido paraq el primer grupo
	// Get end index of the group
	uint32_t rightIndex = getGroupRightIndex(wichGroup);

	// Sanity check
	if (rightIndex == 0) return 0;

	uint32_t leftIndex = rightIndex - readGroupSize(rightIndex);

	return leftIndex;
}
uint16_t SckList::readGroupSize(uint32_t rightGroupIndex)
{
	// Read and join the first two bytes (group size)
	uGroupSize.b[1] = read(rightGroupIndex - 2);
	uGroupSize.b[0] = read(rightGroupIndex - 1);

	return uGroupSize.i;
}
bool SckList::createGroup(uint32_t timeStamp)
{
	// Check if there is an open group and delete it
	if (lastGroupIsOpen()) saveLastGroup();

	// Store timeStamp in current index
	if (!append((char)SENSOR_COUNT)) return false; 	// Sensor type (using max sensor number for timestamp)
	if (!append(4)) return false; 			// Size of the payload (uint32_t is 4 bytes)
	uTimeStamp.i = timeStamp;
	for (int8_t i=3; i>=0; i--) if (!append(uTimeStamp.b[i])) return false; // The timestamp byte per byte

	return true;
};
bool SckList::saveLastGroup()
{
	// If last created group has no readings return false
	if (lastGroupRightIndex + 6 == index) return false;

	// Init tags in 1 for all
	byte byteFlags = 0xFF;
	if (!append(byteFlags)) return false;

	// Save group size
	uGroupSize.i = index - lastGroupRightIndex + 2; // Group size in bytes (including this last two bytes that are going to be filled now)

	if (!append(uGroupSize.b[1])) return false;
	if (!append(uGroupSize.b[0])) return false;

	totalGroups++;
	lastGroupRightIndex = index;

	// Save power until the next group of readings is stored
	if (usingFlash) {
		if (flash.powerDown()) flashSleeping = true;
	}

	return true;
}
bool SckList::lastGroupIsOpen()
{
	if (lastGroupRightIndex < index) return true;
	return false;
}
bool SckList::delLastGroup()
{

	debugOut("Deleting last group...");

	debugOut("Full readinglist before deleting last group:");
	if (debug) for (uint32_t i=0; i<index; i++) SerialUSB.print(read(i));
	debugOut(" ");

	uint32_t openGroupEndIndex = 0;
	uint32_t openGroupStartIndex = 0;

	// If last group is open we need to move it to the new end
	if (lastGroupIsOpen()) {
		debugOut("The last group is open!!");
		openGroupEndIndex = index;
		openGroupStartIndex = lastGroupRightIndex;
		index = lastGroupRightIndex;
		/* return true; */
	}

	// Change index to remove last Group
	index = index - readGroupSize(index);

	// Copy open group to the new end
	if (lastGroupIsOpen()) {
		debugOut("Moving open group to the new end");
		for (uint8_t i=openGroupStartIndex; i<openGroupEndIndex; i++) {
			append(read(i));
		}
		lastGroupRightIndex = openGroupStartIndex;
	} else {
		lastGroupRightIndex = index;
	}

	debugOut("Full readinglist after deleting last group:");
	if (debug) for (uint32_t i=0; i<index; i++) SerialUSB.print(read(i));
	debugOut(" ");

	totalGroups--;

	// After deleting the last group if we are using flash we should return to RAM
	if (totalGroups == 0 && usingFlash) usingFlash = false;

	return true;
}
uint32_t SckList::countGroups()
{
	return totalGroups;
}
uint32_t SckList::getTime(uint32_t wichGroup)
{
	// Get the index where the timeStamp starts
	uint32_t timeIndex = getGroupLeftIndex(wichGroup) + 2;

	// Read and join the 4 bytes
	for (int8_t i=3; i>=0; i--) {
		uTimeStamp.b[i] = read(timeIndex);
		timeIndex++;
	}

	return uTimeStamp.i;
}
uint16_t SckList::countReadings(uint32_t wichGroup)
{
	// TODO optimizar la obtencion de indices, cuando pides left tambien calcula right, como esta ahora lo hace doble.
	uint32_t rightIndex = getGroupRightIndex(wichGroup);
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);
	uint16_t counter = 0;

	while (leftIndex < rightIndex - 3) { // The minus 3 are: 1 byte:flags, and 2 bytes: group size
		uint8_t readingSize = read(leftIndex + 1); // Get reading size byte
		leftIndex += readingSize + 2; // Add readingSize + 1 byte from sensorType + 1 byte from where the size is written
		counter++; // Count how many readings exist until we reach the end of the group
	}

	return counter - 1; // The first reading is TimeStamp so we don't count it as a normal reading
}
bool SckList::appendReading(SensorType wichSensor, String value)
{
	// Be sure a group has been already created
	if (!lastGroupIsOpen()) return false;

	// Write Sensor Type
	if (!append(wichSensor)) return false;

	// Write size of reading (in bytes)
	uint8_t valueSize = value.length();
	if (!append(valueSize)) return false;

	// Write reading
	for (uint8_t i=0; i<valueSize; i++) if (!append(value.charAt(i))) return false;

	return true;
}
OneReading SckList::readReading(uint32_t wichGroup, uint8_t wichReading)
{
	// TODO proteccion para cuando piden lecturas que no existen

	// Get first reading index
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);

	uint16_t counter = 0;

	// Get to the reading
	while (counter < wichReading + 1) {
		uint8_t readingSize = read(leftIndex + 1); // Get reading size byte
		leftIndex += readingSize + 2; // Add readingSize + 1 byte from sensorType + 1 byte from where the size is written
		counter++;
	}

	OneReading thisReading;

	// Get sensorType
	thisReading.type = static_cast<SensorType>(read(leftIndex));
	leftIndex++;

	// Get the size in bytes of the reading
	uint32_t readingEnd = leftIndex + (uint8_t)read(leftIndex);
	leftIndex++;

	// Get the value
	while (leftIndex <= readingEnd) {
		thisReading.value.concat(read(leftIndex));
		leftIndex++;
	}

	return thisReading;
}
void SckList::setFlag(uint32_t wichGroup, GroupFlags wichFlag, bool value)
{
	// Get group Index
	uint32_t flagsIndex = getGroupRightIndex(wichGroup) - 3;
	
	// Read de full flags byte
	byte byteFlags = read(flagsIndex);

	// Set or clear the requested flag (flags are negated)
	if (value) bitClear(byteFlags, wichFlag);
	else bitSet(byteFlags, wichFlag);

	// And write flags byte back
	write(flagsIndex, byteFlags);
}
bool SckList::getFlag(uint32_t wichGroup, GroupFlags wichFlag)
{
	// Get group Index
	uint32_t flagsIndex = getGroupRightIndex(wichGroup) - 3;

	// Read de full flags byte
	byte byteFlags = read(flagsIndex);

	bool result = bitRead(byteFlags, wichFlag);
	return !result;
}
void SckList::flashStart()
{
	flashSelect();
	flash.begin();
	flash.setClock(133000);
	flashStarted = true;

	if (flash.powerDown()) flashSleeping = true;
}
void SckList::flashSelect()
{
	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
	if (flashSleeping) flash.powerUp();
}
void SckList::migrateToFlash()
{
	flashSelect();
	for (uint32_t i=0; i<index; i++) flash.writeByte(i, ramBuff[i]);
	usingFlash = true;
}
void SckList::debugOut(const char *text)
{
	if (debug) SerialUSB.println(text);
}
