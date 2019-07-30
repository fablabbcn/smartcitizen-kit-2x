#include "SckList.h"

bool SckList::append(char value)
{
	// Writes value and updates de index
	if (write(index_RAM, value)) index_RAM++;
	else {
		debugOut("Failed appending in RAM", true);
		return false;
	}

	return true;
}
bool SckList::write(uint32_t wichIndex, char value)
{
	// TODO esta funcion siempre retorna true, no tiene sentido que sea bool y tampoco append

	if (wichIndex == SCKLIST_RAM_SIZE) {
		
		// If there is no ram space left, migrate to flash
		if (!usingFlash) migrateToFlash();

		// Since the index in RAM has changed we need to append it again based on new index
		append(value);

	} else ramBuff[wichIndex] = value;

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
	if (wichGroup > totalGroups_RAM) return 0;

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
	if (wichGroup > totalGroups_RAM) return 0;

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
	if (rightGroupIndex > index_RAM) return 0;

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
	debugOut("Creating new reading group");
	bool error = false;

	// Check if there is an open group and delete it
	if (lastGroupIsOpen()) saveLastGroup();

	uint32_t preErrorIndex = index_RAM;

	// Store timeStamp in current index
	if (!append((char)SENSOR_COUNT)) error = true; 	// Sensor type (using max sensor number for timestamp)
	if (!append(4)) error = true; 			// Size of the payload (uint32_t is 4 bytes)
	uTimeStamp.i = timeStamp;
	for (int8_t i=3; i>=0; i--) if (!append(uTimeStamp.b[i])) error = true; // The timestamp byte per byte

	if (error) {
		debugOut("Failed creating reading group", true);
		index_RAM = preErrorIndex;
		return false;
	}

	return true;
};
bool SckList::saveLastGroup()
{
	debugOut("Saving last group");

	// If last created group has no readings we delete it (this function is only called from createGroup())
	if (lastGroupRightIndex + 6 == index_RAM) { 	// 6 bytes is always the size of the first reading: the timestamp. (0:type, 1:size: 2-6:epoch time)
		debugOut("No readings found", true);
		index_RAM = lastGroupRightIndex;
		return false;
	}

	// Init tags in 1 for all
	byte byteFlags = 0xFF;

	if (!append(byteFlags)) return false;

	// Save group size
	uGroupSize.i = index_RAM - lastGroupRightIndex + 2; // Group size in bytes (including this last two bytes that are going to be filled now)

	if (!append(uGroupSize.b[1])) return false;
	if (!append(uGroupSize.b[0])) return false;

	totalGroups_RAM++;

	lastGroupRightIndex = index_RAM;

	if (usingFlash) flashSaveLastGroup();

	return true;
}
bool SckList::lastGroupIsOpen()
{
	if (lastGroupRightIndex < index_RAM) return true;
	return false;
}
bool SckList::delLastGroup()
{
	// TODO cuando se llama esto y estamos usando flash que pasa????
	
	// deberiamos tener una funcion para ram y otra para flash separadas
	// y ademas una generica que decida a cual llamar cuando nos es idiferente a cual se llame.

	debugOut("Deleting last group");

	if (totalGroups_RAM == 0) return false;

	// If last group is open we need to move it to the new end
	if (lastGroupIsOpen()) {

		uint32_t openGroupEndIndex = 0;
		uint32_t openGroupStartIndex = 0;

		debugOut("The last group is open!");

		openGroupEndIndex = index_RAM;
		openGroupStartIndex = lastGroupRightIndex;

		// The new index should be were deleted group started
		index_RAM =  lastGroupRightIndex - readGroupSize(lastGroupRightIndex);

		// And if there are groups before they end in the same place were we start.
		lastGroupRightIndex = index_RAM;

		// Copy the open group to the place where deleted group was
		for (uint32_t i=openGroupStartIndex; i<openGroupEndIndex; i++) append(read(i));

	} else {

		// Change index to remove last Group
		index_RAM =  lastGroupRightIndex - readGroupSize(lastGroupRightIndex);
		lastGroupRightIndex = index_RAM;
	}

	totalGroups_RAM--;

	// TODO esto no parece tener sentido...
	// After deleting the last group if we are using flash we should return to RAM
	/* if (totalGroups_RAM == 0 && usingFlash) { */
	/* 	debugOut("Memory is empty, returning to RAM"); */
	/* 	usingFlash = false; */
	/* } */

	return true;
}
uint32_t SckList::countGroups()
{
	return totalGroups_RAM + totalGroups_flash;
}
uint32_t SckList::getTime(uint32_t wichGroup)
{
	if (debug) {
		SerialUSB.print("Getting time from group ");
		SerialUSB.println(wichGroup);
	}

	if (wichGroup > totalGroups_RAM) return 0;

	// Get the index where the timeStamp starts
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totalGroups_RAM-1 && leftIndex == 0) return 0;


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

	if (wichGroup > totalGroups_RAM) return 0;

	// TODO optimize getting indexes, now there is too much duplicated work done.
	uint32_t rightIndex = getGroupRightIndex(wichGroup);
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totalGroups_RAM-1 && leftIndex == 0) return 0;

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
	debugOut("Appending reading");

	// Be sure a group has been already created
	if (!lastGroupIsOpen()) createGroup();

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

	if (wichGroup > totalGroups_RAM) return thisReading;
	if (countReadings(wichGroup) < wichReading) return thisReading;

	// Get first reading index
	uint32_t leftIndex = getGroupLeftIndex(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totalGroups_RAM-1 && leftIndex == 0) return thisReading;

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
	if (wichGroup > totalGroups_RAM) return -1;
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
	debugOut("Starting");
	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
	flash.begin();
	flash.setClock(133000);

	// Scan sector state
	for (uint16_t i=0; i<SCKLIST_SECTOR_NUM; i++) {
		switch(getSectorState(i))
	{
			case SECTOR_FREE: fstatus.freeSecCount++; break;
			case SECTOR_FULL: fstatus.fullSecCount++; break;
			case SECTOR_TRASH: fstatus.trashSecCount++; break;
			case SECTOR_CURRENT: currSector = i; break;
	}
	}

	// If no current sector found, we format the flash and start from scratch
	if (currSector == -1) {
		debugOut("Memory is not formated or damaged!!", true);
		if (fstatus.fullSecCount == 0) {
			flashFormat();
			return;
		}
		// TODO we can try a more in deep scan to recover data in case it exists and for some reason we loose the currsector index
	}

	// Check if we have any valid readings in flash
	uint16_t currSectorGroupNum = countSectorGroups(currSector);
	if (fstatus.fullSecCount > 0 || currSectorGroupNum > 0) {
		debugOut("Founded readings on memory");
		usingFlash = true;
	}

	// Update flash_index
	uint32_t currSectorAddress = getSectorAddress(currSector);
	if (currSectorGroupNum > 0) {
		index_flash = flash.readULong(SECTOR_SIZE - (currSectorGroupNum * 4));
	} else {
		index_flash = currSectorAddress + 1;
	}

	// Update flash index to store the address of groups
	index_flash_groupAddr = currSectorAddress + (SECTOR_SIZE - (currSectorGroupNum * 4)) - 4;

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
	debugOut("RAM is full migrating to flash");
	flashSelect();
	while (totalGroups_RAM > 0) flashSaveLastGroup();
	usingFlash = true;
}
void SckList::debugOut(const char *text, bool error)
{
	if (debug) {
		SerialUSB.print("FLASH: ");
		if (error) SerialUSB.print("ERROR ");
		SerialUSB.println(text);
	}
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
bool SckList::flashWrite(uint32_t wichIndex, char value)
{
	return flash.writeChar(wichIndex, value);
}
bool SckList::flashAppend(char value)
{
	if (flashWrite(index_flash, value)) index_flash++;
	else return false;

	return true;
}
char SckList::flashRead(uint32_t wichIndex)
{

}
bool SckList::flashFormat()
{
	debugOut("Formating");
	flashSelect();

	if (!flash.eraseChip()) {
		debugOut("Failed resing memory", true);
		return false;
	}

	if (!flash.writeByte(0, SECTOR_CURRENT)) {
		debugOut("Failed writing on memory", true);
		return false;
	}

	fstatus.freeSecCount = SCKLIST_SECTOR_NUM;
	fstatus.fullSecCount = 0;
	fstatus.trashSecCount = 0;
	currSector = 0;
	index_flash = 1;
	index_flash_groupAddr = SECTOR_SIZE - 4;

	return true;
}
uint32_t SckList::getSectorAddress(uint16_t wichSector)
{
	return wichSector * SECTOR_SIZE;
}
SectorState SckList::getSectorState(uint16_t wichSector)
{
	SectorState wichState = static_cast<SectorState>(flash.readByte(getSectorAddress(wichSector)));
	return wichState;
}
uint16_t SckList::countSectorGroups(uint16_t wichSector)
{
	uint32_t startAddr = getSectorAddress(wichSector);
	uint32_t endAddr = startAddr + 4096;

	uint16_t counter = 0;
	for (uint16_t i=endAddr-4; i>1; i-=4) {
		uint32_t address = flash.readULong(i);
		if (address == 0xFFFFFFFF) break;
		counter ++;
	}
	return counter;
}
bool SckList::flashSaveLastGroup()
{
	debugOut("Saving last group to flash");

	uint16_t groupSize = readGroupSize(totalGroups_RAM);

	uint32_t currSectorAddress = getSectorAddress(currSector);
	uint16_t currSectorGroupNum = countSectorGroups(currSector);

	// Get current sector free space
	uint16_t freeSpace = SECTOR_SIZE -
		((currSectorAddress - index_flash)  	// The space used by saved data in current sector
		 + (currSectorGroupNum * 4)); 		// The space used by the addresses of each group in current sector

	// Check if new group fits in current sector
	if (freeSpace - 4 < groupSize) {
		// Close current sector and start a new one
		flash.writeByte(currSectorAddress, SECTOR_FULL);
		currSector ++;
		flash.writeByte(currSectorAddress, SECTOR_CURRENT);
	}

	// Copy RAM data to flash
	for (uint32_t i=lastGroupRightIndex-groupSize; i<lastGroupRightIndex; i++) {
		flashAppend(read(i));
	}

	// Save flash groupIndex at the end of the sector
	flash.writeULong(index_flash_groupAddr, index_flash);

	// Update RAM index
	delLastGroup();

	return true;
}
