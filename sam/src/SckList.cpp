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

	if (debug) {
		SerialUSB.print("Right index of group ");
		SerialUSB.print(wichGroup);
		SerialUSB.print(" is ");
		SerialUSB.println(thisIndex);
	}

	return thisIndex;
}
uint32_t SckList::getGroupLeftIndex(uint32_t wichGroup)
{
	if (wichGroup > totalGroups) return 0;

	// Get end index of the group
	uint32_t rightIndex = getGroupRightIndex(wichGroup);

	// Sanity check
	if (rightIndex == 0) return 0;

	uint32_t leftIndex = rightIndex - readGroupSize(rightIndex);

	if (debug) {
		SerialUSB.print("Left index of group ");
		SerialUSB.print(wichGroup);
		SerialUSB.print(" is ");
		SerialUSB.println(leftIndex);
	}

	return leftIndex;
}
uint16_t SckList::readGroupSize(uint32_t rightGroupIndex)
{
	if (rightGroupIndex > index) return 0;

	// Read and join the first two bytes (group size)
	uGroupSize.b[1] = read(rightGroupIndex - 2);
	uGroupSize.b[0] = read(rightGroupIndex - 1);

	if (debug) {
		SerialUSB.print("Size of group: ");
		SerialUSB.println(uGroupSize.i);
	}

	return uGroupSize.i;
}
bool SckList::createGroup(uint32_t timeStamp)
{
	bool error = false;

	// Check if there is an open group and delete it
	if (lastGroupIsOpen()) saveLastGroup();

	uint32_t preErrorIndex = index;

	// Store timeStamp in current index
	if (!append((char)SENSOR_COUNT)) error = true; 	// Sensor type (using max sensor number for timestamp)
	if (!append(4)) error = true; 			// Size of the payload (uint32_t is 4 bytes)
	uTimeStamp.i = timeStamp;
	for (int8_t i=3; i>=0; i--) if (!append(uTimeStamp.b[i])) error = true; // The timestamp byte per byte

	if (error) {
		index = preErrorIndex;
		return false;
	}

	return true;
};
bool SckList::saveLastGroup()
{
	debugOut("Saving last group");

	// If last created group has no readings return false
	if (lastGroupRightIndex + 6 == index) return false;

	// Init tags in 1 for all
	byte byteFlags = 0xFF;
	if (!append(byteFlags)) return false;

	// Save group size
	uGroupSize.i = index - lastGroupRightIndex + 2; // Group size in bytes (including this last two bytes that are going to be filled now)

	if (debug) {
		SerialUSB.print("Current index: ");
		SerialUSB.println(index);
		SerialUSB.print("last Group Right index: ");
		SerialUSB.println(lastGroupRightIndex);
		SerialUSB.print("Saved group size is: ");
		SerialUSB.println(uGroupSize.i);
	}

	if (!append(uGroupSize.b[1])) return false;
	if (!append(uGroupSize.b[0])) return false;

	totalGroups++;

	if (debug) {
		SerialUSB.print("Total groups: ");
		SerialUSB.println(totalGroups);
	}

	lastGroupRightIndex = index;

	return true;
}
bool SckList::lastGroupIsOpen()
{
	if (lastGroupRightIndex < index) return true;
	return false;
}
bool SckList::delLastGroup()
{
	if (totalGroups == 0) return false;


	// If last group is open we need to move it to the new end
	if (lastGroupIsOpen()) {

		uint32_t openGroupEndIndex = 0;
		uint32_t openGroupStartIndex = 0;

		debugOut("The last group is open!!");

		openGroupEndIndex = index;
		openGroupStartIndex = lastGroupRightIndex;

		// The new index should be were deleted group started
		index =  lastGroupRightIndex - readGroupSize(lastGroupRightIndex);

		// And if there are groups before they end in the same place were we start.
		lastGroupRightIndex = index;

		// Copy the open group to the place where deleted group was
		for (uint32_t i=openGroupStartIndex; i<openGroupEndIndex; i++) append(read(i));

	} else {

		// Change index to remove last Group
		index =  lastGroupRightIndex - readGroupSize(lastGroupRightIndex);
		lastGroupRightIndex = index;
	}

	totalGroups--;

	// After deleting the last group if we are using flash we should return to RAM
	if (totalGroups == 0 && usingFlash) usingFlash = false;

	return true;
}
uint32_t SckList::countGroups()
{
	if (debug) {
		SerialUSB.print("Counting groups: ");
		SerialUSB.println(totalGroups);
	}
	return totalGroups;
}
uint32_t SckList::getTime(uint32_t wichGroup)
{
	if (debug) {
		SerialUSB.print("Getting time from group ");
		SerialUSB.println(wichGroup);
	}

	if (wichGroup > totalGroups) return 0;

	// Get the index where the timeStamp starts
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totalGroups-1 && leftIndex == 0) return 0;


	uint32_t timeIndex = leftIndex + 2;

	// Read and join the 4 bytes
	for (int8_t i=3; i>=0; i--) {
		uTimeStamp.b[i] = read(timeIndex);
		timeIndex++;
	}

	if (debug) {
		SerialUSB.print("epoch: ");
		SerialUSB.println(uTimeStamp.i);
	}

	return uTimeStamp.i;
}
uint16_t SckList::countReadings(uint32_t wichGroup)
{
	if (debug) {
		SerialUSB.print("Counting readings on group: ");
		SerialUSB.println(wichGroup);
	}

	if (wichGroup > totalGroups) return 0;

	// TODO optimize getting indexes, now there is too much duplicated work done.
	uint32_t rightIndex = getGroupRightIndex(wichGroup);
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totalGroups-1 && leftIndex == 0) return 0;

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
	OneReading thisReading;
	thisReading.type = SENSOR_COUNT;
	thisReading.value = "null";

	if (wichGroup > totalGroups) return thisReading;
	if (countReadings(wichGroup) < wichReading) return thisReading;

	// Get first reading index
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totalGroups-1 && leftIndex == 0) return thisReading;

	uint16_t counter = 0;

	// Get to the reading
	while (counter < wichReading + 1) {
		uint8_t readingSize = read(leftIndex + 1); // Get reading size byte
		leftIndex += readingSize + 2; // Add readingSize + 1 byte from sensorType + 1 byte from where the size is written
		counter++;
	}


	// Get sensorType
	thisReading.type = static_cast<SensorType>(read(leftIndex));
	leftIndex++;

	// Get the size in bytes of the reading
	uint32_t readingEnd = leftIndex + (uint8_t)read(leftIndex);
	leftIndex++;

	// Clear null value
	thisReading.value = "";

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
int8_t SckList::getFlag(uint32_t wichGroup, GroupFlags wichFlag)
{
	if (wichGroup > totalGroups) return -1;
	if (wichFlag > 7) return -1;

	// Get group Index
	uint32_t flagsIndex = getGroupRightIndex(wichGroup) - 3;

	// Read de full flags byte
	byte byteFlags = read(flagsIndex);

	uint8_t result = bitRead(byteFlags, wichFlag);
	return !result;
}
void SckList::flashStart()
{
	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
	flash.begin();
	flash.setClock(133000);
	flashStarted = true;
}
void SckList::flashSelect()
{
	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
	if (!flashStarted) flashStart();
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
uint32_t SckList::getFlashCapacity()
{
	if (!flashStarted) flashStart();

	flashSelect();
	return flash.getCapacity();
}
bool SckList::testFlash()
{
	flashSelect();

	String writeSRT = "testing the flash!";
	uint32_t fAddress = flash.getAddress(flash.sizeofStr(writeSRT));

	flash.writeStr(fAddress, writeSRT);

	String readSTR;
	flash.readStr(fAddress, readSTR);

	if (!readSTR.equals(writeSRT)) return false;
	return true;
}
