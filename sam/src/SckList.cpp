#include "SckList.h"

// RAM
bool SckList::write_r(uint32_t wichIndex, char value)
{
	// TODO esta funcion siempre retorna true

	if (wichIndex == SCKLIST_RAM_SIZE) {

		// If there is no ram space left, migrate to flash
		if (!usingFlash) migrateToFlash();

		// Since the index in RAM has changed we need to append it again based on new index
		append_r(value);

	} else ramBuff[wichIndex] = value;

	return true;
}
bool SckList::append_r(char value)
{
	// Writes value and updates de index
	if (write_r(idx_r, value)) idx_r++;

	// Writes value and updates de index
	if (write_r(idx_r, value)) idx_r++;
	else {
		debugOut("Failed appending in RAM", true);
		return false;
	}

	return true;
}
char SckList::read_r(uint32_t wichIndex)
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
bool SckList::createGroup(uint32_t timeStamp)
{
	debugOut("Creating new reading group");
	bool error = false;

	// Check if there is an open group and save it
	if (lastGrpIsOpen_r()) saveLastGroup();

	uint32_t preErrorIndex = idx_r;

	// Store timeStamp in current index
	if (!append_r((char)SENSOR_COUNT)) error = true; 	// Sensor type (using max sensor number for timestamp)
	if (!append_r(4)) error = true; 			// Size of the payload (uint32_t is 4 bytes)
	uTimeStamp.i = timeStamp;
	for (int8_t i=3; i>=0; i--) if (!append_r(uTimeStamp.b[i])) error = true; // The timestamp byte per byte

	if (error) {
		debugOut("Failed creating reading group", true);
		idx_r = preErrorIndex;
		return false;
	}

	return true;
};
bool SckList::lastGrpIsOpen_r()
{
	if (lastGrpRightIdx_r < idx_r) return true;
	return false;
}
bool SckList::delLastGrp_r()
{
	debugOut("Deleting last RAM group");

	if (totGrp_r == 0) return false;

	// If last group is open we need to move it to the new end
	if (lastGrpIsOpen_r()) {

		uint32_t openGrpEndIdx = 0;
		uint32_t openGrpStartIdx = 0;

		debugOut("The last group is open!");

		openGrpEndIdx = idx_r;
		openGrpStartIdx = lastGrpRightIdx_r;

		// The new index should be were deleted group started
		idx_r =  lastGrpRightIdx_r - getGrpSize(lastGrpRightIdx_r, true);

		// And if there are groups before they end in the same place were we start.
		lastGrpRightIdx_r = idx_r;

		// Copy the open group to the place where deleted group was
		for (uint32_t i=openGrpStartIdx; i<openGrpEndIdx; i++) append_r(read_r(i));

	} else {

		// Change index to remove last Group
		idx_r =  lastGrpRightIdx_r - getGrpSize(lastGrpRightIdx_r, true);
		lastGrpRightIdx_r = idx_r;
	}

	totGrp_r--;

	return true;
}


// FLASH
bool SckList::write_f(uint32_t wichIndex, char value)
{
	// TODO When all the flash is used we will start erasing the first sector and reusing it.

	return flash.writeChar(wichIndex, value);
}
bool SckList::append_f(char value)
{
	if (write_f(idx_f, value)) idx_f++;
	else return false;

	return true;
}
char SckList::read_f(uint32_t wichIndex)
{

}
bool SckList::saveLastGrp_f()
{
	debugOut("Saving last group to flash");

	uint16_t groupSize = getGrpSize(totGrp_r, true);

	uint32_t currSectorAddress = getSectAddr(currSector);

	// Get current sector free space
	uint16_t freeSpace = getSectFree(currSector);

	// Check if new group fits in current sector
	if (freeSpace - 4 < groupSize) {

		// If current sector has no space we close it and start a new one
		// TODO check if there is still space in flash memory  (SCKLIST_SECTOR_NUM)

		// Mark current sector as full
		if (!flash.writeByte(currSectorAddress, SECTOR_FULL)) return false;

		// Start a new sector
		currSector++;

		// Update index
		idx_f = getSectAddr(currSector);
		index_flash_groupAddr = idx_f + SECTOR_SIZE - 4;

		// Mark sector as SECTOR_CURRENT
		if (!append_f(SECTOR_CURRENT)) return false;
	}

	// Copy RAM data to flash
	for (uint32_t i=lastGrpRightIdx_r-groupSize; i<lastGrpRightIdx_r; i++) {
		if (!append_f(read_r(i))) return false;
	}

	// Update flash group count
	totGrp_f++;

	// Save flash groupIndex at the end of the sector
	if (!flash.writeULong(index_flash_groupAddr, idx_f)) return false;

	// Update RAM index
	if (!delLastGrp_r()) return false;

	return true;
}
bool SckList::delLastGrp_f()
{
	debugOut("Deleting last flash group");

	if (totGrp_f == 0) return false;

	// Wich is the last group in flash????
	/// TODO keep last flash group stored some place
	/* setFlag(lastGroup, FLASH_DELETED, true); */

	totGrp_f--;

	return true;
}

// Sector utilities
uint32_t SckList::getSectAddr(uint16_t wichSector)
{
	return wichSector * SECTOR_SIZE;
}
uint16_t SckList::getSectFree(uint16_t wichSector)
{
	uint32_t startAddr = getSectAddr(wichSector);
	uint32_t endAddr = startAddr + 4096;

	uint16_t counter = 0;
	uint32_t address = 0;
	for (uint16_t i = (endAddr - 4); i > (startAddr + 1); i -= 4) { 	// startAddr +1 because the first byte is for storing sectorState
		address = flash.readULong(i);
		if (address == 0xFFFFFFFF) {
			break;
		}
		counter++;
	}

	// Get current sector free space
	uint16_t freeSpace = SECTOR_SIZE -
		(address - startAddr 	 	// The space used by saved data in current sector
		 + (counter * 4)); 			// The space used by the addresses of each group in current sector

	return freeSpace;
}
SectorState SckList::getSectState(uint16_t wichSector)
{
	SectorState wichState = static_cast<SectorState>(flash.readByte(getSectAddr(wichSector)));
	return wichState;
}
uint16_t SckList::getSectGrpNum(uint16_t wichSector, bool validOnly)
{
	uint32_t startAddr = getSectAddr(wichSector);
	uint32_t endAddr = startAddr + 4096;

	uint16_t counter = 0;
	for (uint16_t i = (endAddr - 4); i > (startAddr + 1); i -= 4) { 	// startAddr +1 because the first byte is for storing sectorState
		uint32_t address = flash.readULong(i);
		if (address == 0xFFFFFFFF) break;

		if (validOnly) {
			if (getFlag(wichSector, FLASH_DELETED))	counter++;
		} else {
			counter ++;
		}
	}

	return counter;
}

// General flash utilities
void SckList::flashStart()
{
	debugOut("Starting");
	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
	flash.begin();
	flash.setClock(133000);

	// Scan sector state
	for (uint16_t i=0; i<SCKLIST_SECTOR_NUM; i++) {
		switch(getSectState(i))
	{
			case SECTOR_FREE: fstatus.freeSecCount++; break;
			case SECTOR_FULL:
					  {
						fstatus.fullSecCount++;
						totGrp_f += getSectGrpNum(i, true);
						break;
					  }
			case SECTOR_TRASH: fstatus.trashSecCount++; break;
			case SECTOR_CURRENT:
					   {
						currSector = i;
						totGrp_f += getSectGrpNum(i, true);
						break;
					   }
	}
	}

	// If no current sector found, we format the flash and start from scratch
	if (currSector == -1) {
		debugOut("Flash memory is not formated or damaged!!", true);

		if (fstatus.fullSecCount > 0) {
			// TODO recover data in case it exists and  find a way to update current sector
		}

		flashFormat();
		return;
	}

	// Check if we have any valid readings in flash
	if (totGrp_f > 0) {
		debugOut("Founded readings on flash memory");
		usingFlash = true;
	}

	// Update flash_index and flash_groupAddr
	uint32_t startAddr = getSectAddr(currSector);
	uint32_t endAddr = startAddr + 4096;
	uint16_t counter = 0;
	uint32_t address = 0;
	for (uint16_t i = (endAddr - 4); i > (startAddr + 1); i -= 4) { 	// startAddr +1 because the first byte is for storing sectorState
		address = flash.readULong(i);
		if (address == 0xFFFFFFFF) break;
		counter ++;
	}

	idx_f = address + 1;
	index_flash_groupAddr = address + (SECTOR_SIZE - (counter * 4)) - 4;

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
	while (totGrp_r > 0) saveLastGrp_f();
	usingFlash = true;
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
bool SckList::flashFormat()
{
	// TODO Every time we flash the firmware of the SAM the flash memory should be formatted and started

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
	idx_f = 1; 					// Address 0 is reserved to the sectorState flag
	index_flash_groupAddr = SECTOR_SIZE - 4;

	if (!flash.writeByte(0, SECTOR_CURRENT)) return false; 	// Mark first sector as SECTOR_CURRENT

	return true;
}


// SHARED
uint16_t SckList::getGrpSize(uint32_t rightGroupIndex, bool force_ram)
{
	if (!usingFlash || force_ram) {

		if (rightGroupIndex > idx_r) return 0;

		// Read and join the first two bytes (group size)
		uGroupSize.b[1] = read_r(rightGroupIndex - 2);
		uGroupSize.b[0] = read_r(rightGroupIndex - 1);

	} else {

		if (rightGroupIndex > idx_f) return 0;

		// Read and join the first two bytes (group size)
		uGroupSize.b[1] = read_f(rightGroupIndex - 2);
		uGroupSize.b[0] = read_f(rightGroupIndex - 1);
	}

	if (debug) {
		SerialUSB.print("Size of group: ");
		SerialUSB.println(uGroupSize.i);
	}

	return uGroupSize.i;
}
uint32_t SckList::getGrpLeftIdx(uint32_t wichGroup)
{

	uint32_t leftIndex = 0;
	uint16_t totGrp = 0;

	if (!usingFlash) totGrp = totGrp_r;
	else totGrp = totGrp_f;

	if (wichGroup > totGrp) return 0;

	// Get end index of the group
	uint32_t rightIndex = getGrpRightIdx(wichGroup);

	// Sanity check
	if (rightIndex == 0) return 0;

	leftIndex = rightIndex - getGrpSize(rightIndex);

	if (debug) {
		SerialUSB.print("Left index of group ");
		SerialUSB.print(wichGroup);
		SerialUSB.print(" is ");
		SerialUSB.println(leftIndex);
	}

	return leftIndex;
}
uint32_t SckList::getGrpRightIdx(uint32_t wichGroup)
{
	uint32_t thisIndex = 0;
	uint32_t groupCounter = 0;
	uint16_t totGrp = 0;

	if (!usingFlash) {
		totGrp = totGrp_r;
		thisIndex = lastGrpRightIdx_r;
	} else {
		totGrp_r = totGrp_f;
		thisIndex = lastGrpRightIdx_f;
	}

	if (wichGroup > totGrp) return 0;

	while (groupCounter < wichGroup) {
		thisIndex = thisIndex - getGrpSize(thisIndex);
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


// PUBLIC
bool SckList::saveLastGroup()
{
	debugOut("Saving last group");

	// If last created group has no readings we delete it (this function is only called from CreateGrp())
	if (lastGrpRightIdx_r + 6 == idx_r) { 	// 6 bytes is always the size of the first reading: the timestamp. (0:type, 1:size: 2-6:epoch time)
		debugOut("No readings found", true);
		idx_r = lastGrpRightIdx_r;
		return false;
	}

	// Init tags in 1 for all
	byte byteFlags = 0xFF;

	if (!append_r(byteFlags)) return false;

	// Save group size
	uGroupSize.i = idx_r - lastGrpRightIdx_r + 2; // Group size in bytes (including this last two bytes that are going to be filled now)

	if (!append_r(uGroupSize.b[1])) return false;
	if (!append_r(uGroupSize.b[0])) return false;

	totGrp_r++;

	lastGrpRightIdx_r = idx_r;

	if (usingFlash) saveLastGrp_f();

	return true;
}
bool SckList::delLastGroup()
{
	if (!usingFlash) {
		if (!delLastGrp_r()) return false;
	} else {
		if (!delLastGrp_f()) return false;
	}

	return true;
}
bool SckList::appendReading(SensorType wichSensor, String value)
{
	debugOut("Appending reading");

	// Be sure a group has been already created
	if (!lastGrpIsOpen_r()) return false;

	// Write Sensor Type
	if (!append_r(wichSensor)) return false;

	// Write size of reading (in bytes)
	uint8_t valueSize = value.length();
	if (!append_r(valueSize)) return false;

	// Write reading
	for (uint8_t i=0; i<valueSize; i++) if (!append_r(value.charAt(i))) return false;

	return true;
}
uint32_t SckList::getGroupTime(uint32_t wichGroup)
{
	if (debug) {
		SerialUSB.print("Getting time from group ");
		SerialUSB.println(wichGroup);
	}

	if (!usingFlash) if (wichGroup > totGrp_r) return 0;
	else if (wichGroup > totGrp_f) return 0;

	// Get the index where the timeStamp starts
	uint32_t leftIndex = getGrpLeftIdx(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totGrp_r-1 && leftIndex == 0) return 0;

	uint32_t timeIndex = leftIndex + 2;

	// Read and join the 4 bytes
	for (int8_t i=3; i>=0; i--) {
		uTimeStamp.b[i] = read_r(timeIndex);
		timeIndex++;
	}

	if (debug) {
		SerialUSB.print("epoch: ");
		SerialUSB.println(uTimeStamp.i);
	}

	return uTimeStamp.i;
}
uint32_t SckList::countGroups()
{
	return totGrp_r + totGrp_f;
}
uint16_t SckList::countReadings(uint32_t wichGroup)
{
	if (debug) {
		SerialUSB.print("Counting readings on group: ");
		SerialUSB.println(wichGroup);
	}

	uint16_t totGrp = 0;

	if (!usingFlash) totGrp = totGrp_r;
	else totGrp = totGrp_f;

	if (wichGroup > totGrp) return 0;

	// TODO optimize getting indexes, now there is too much duplicated work done.
	uint32_t rightIndex = getGrpRightIdx(wichGroup);
	uint32_t leftIndex = getGrpLeftIdx(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totGrp-1 && leftIndex == 0) return 0;

	uint16_t counter = 0;

	while (leftIndex < rightIndex - 3) { // The minus 3 are: 1 byte:flags, and 2 bytes: group size

		uint8_t readingSize = 0;
		if (!usingFlash) readingSize = read_r(leftIndex + 1); 	// Get reading size byte
		else readingSize = read_f(leftIndex + 1); 		// Get reading size byte

		leftIndex += readingSize + 2; // Add readingSize + 1 byte from sensorType + 1 byte from where the size is written
		counter++; // Count how many readings exist until we reach the end of the group
	}

	return counter - 1; // The first reading is TimeStamp so we don't count it as a normal reading
}
OneReading SckList::readReading(uint32_t wichGroup, uint8_t wichReading)
{
	OneReading thisReading;
	thisReading.type = SENSOR_COUNT;
	thisReading.value = "null";

	uint16_t totGrp = 0;

	if (!usingFlash) totGrp = totGrp_r;
	else totGrp = totGrp_f;


	if (wichGroup > totGrp) return thisReading;
	if (countReadings(wichGroup) < wichReading) return thisReading;

	// Get first reading index
	uint32_t leftIndex = getGrpLeftIdx(wichGroup);

	// Return error in case of wrong left index
	if (wichGroup != totGrp-1 && leftIndex == 0) return thisReading;

	uint16_t counter = 0;

	// Get to the reading
	while (counter < wichReading + 1) {
		uint8_t readingSize;
		if (!usingFlash) readingSize = read_r(leftIndex + 1); // Get reading size byte
		else readingSize = read_f(leftIndex + 1); // Get reading size byte
		leftIndex += readingSize + 2; // Add readingSize + 1 byte from sensorType + 1 byte from where the size is written
		counter++;
	}

	uint32_t readingEnd;
	if (!usingFlash) {
		thisReading.type = static_cast<SensorType>(read_r(leftIndex)); 	// Get sensorType
		leftIndex++;
		readingEnd = leftIndex + (uint8_t)read_r(leftIndex); // Get the size in bytes of the reading
		leftIndex++;
	} else {
		thisReading.type = static_cast<SensorType>(read_f(leftIndex)); // Get sensorType
		leftIndex++;
		readingEnd = leftIndex + (uint8_t)read_f(leftIndex); // Get the size in bytes of the reading
		leftIndex++;
	}

	// Clear null value
	thisReading.value = "";

	// Get the value
	while (leftIndex <= readingEnd) {
		if (!usingFlash) thisReading.value.concat(read_r(leftIndex));
		else thisReading.value.concat(read_f(leftIndex));
		leftIndex++;
	}

	return thisReading;
}
int8_t SckList::getFlag(uint32_t wichGroup, GroupFlags wichFlag)
{
	uint16_t totGrp = 0;
	if (!usingFlash) totGrp = totGrp_r;
	else totGrp =totGrp_f;

	if (wichGroup > totGrp) return -1;
	if (wichFlag > 7) return -1;

	// Get group Index
	uint32_t flagsIndex = getGrpRightIdx(wichGroup) - 3;

	// Read de full flags byte
	byte byteFlags;
	if (!usingFlash) byteFlags = read_r(flagsIndex);
	else byteFlags = read_f(flagsIndex);

	uint8_t result = bitRead(byteFlags, wichFlag);
	return !result;
}
void SckList::setFlag(uint32_t wichGroup, GroupFlags wichFlag, bool value)
{
	// Get group Index
	uint32_t flagsIndex = getGrpRightIdx(wichGroup) - 3;

	// Read de full flags byte
	byte byteFlags;
	if (!usingFlash) byteFlags = read_r(flagsIndex);
	else byteFlags = read_f(flagsIndex);

	// Set or clear the requested flag (flags are negated)
	if (value) bitClear(byteFlags, wichFlag);
	else bitSet(byteFlags, wichFlag);

	// And write flags byte back
	if (!usingFlash) write_r(flagsIndex, byteFlags);
	else write_f(flagsIndex, byteFlags);
}


void SckList::debugOut(const char *text, bool error)
{
	if (debug) {
		SerialUSB.print("FLASH: ");
		if (error) SerialUSB.print("ERROR ");
		SerialUSB.println(text);
	}
}
