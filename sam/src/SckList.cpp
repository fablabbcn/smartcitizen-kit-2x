#include "SckList.h"
#include "SckBase.h"

// General flash utilities
int8_t SckList::_flashStart()
{

	if (debug) base->sckOut("F: Starting memory");

	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
	if (!flash.begin()) return -1;
	flash.setClock(133000);

	_scanSectors();

	// If no current sector found that means we never used the flash before or some really bad problem has ocurred, so we format the flash and start from scratch
	if (_currSector == -1) {

		dumpSector(0);

		// Print error
		base->sckOut("Flash memory is not formated or damaged!!", PRIO_HIGH);


		// TODO recover data in case it exists and find a way to update current sector
		// maybe we can avoid format by just erasing some sectors to recover sanity

		_flashFormat();
		return 1;
	}

	if (debug) base->sckOut("F: Started OK");
	return 0;
}
bool SckList::_flashFormat()
{
	base->sckOut("Formating... be patient, don't turn off your kit!", PRIO_HIGH);

	if (!flash.eraseChip()) {
		return false;
		base->sckOut("ERROR while formating flash memory!!!", PRIO_HIGH);
	}

	_currSector = 0;
	_addr = 3;

	base->sckOut("Flash memory formated OK, please power cycle your kit. (not just reset)", PRIO_HIGH);
	return true;
}

// Read/Write functions
bool SckList::_append(char value)
{
	if (!flash.writeByte(_addr, value)) return false;

	_addr++;
	return true;
}

// Group functions
uint32_t SckList::_getGrpAddr(GroupIndex wichGroup)
{
	uint32_t startAddr = _getSectAddr(wichGroup.sector);
	uint32_t endAddr = startAddr + SECTOR_SIZE;
	uint32_t address = startAddr + 3; 	// First two bytes used for sector state and flags
	int16_t groupCount = 0;


	// Read from the byte 2 until we found 0xFFFF
	while (address < endAddr) {
		if (groupCount == wichGroup.group) return address;
		uint16_t groupSize = flash.readWord(address);

		address += groupSize;
		groupCount++;
	}

	return 0;
}
int8_t SckList::_setGrpPublished(GroupIndex wichGroup, PubFlags wichFlag)
{
	// Choose byte position depending on requesed flag
	uint8_t position = GROUP_NET;
	if (wichFlag == PUB_SD) position = GROUP_SD;

	if (debug) {
		sprintf(base->outBuff, "F: Marking group %i in sector %u as %s", wichGroup.group, wichGroup.sector, wichFlag == PUB_NET ? "network published" : "sdcard saved");
		base->sckOut();
	}

	uint32_t flagsAddr = _getGrpAddr(wichGroup) + position;

	// Sanity check
	if (flagsAddr == 0) return -1;

	// And write flags byte back
	return flash.writeByte(flagsAddr, PUBLISHED);
}
int8_t SckList::_isGrpPublished(GroupIndex wichGroup, PubFlags wichFlag)
{
	// Choose byte position depending on requesed flag
	uint8_t position = GROUP_NET;
	if (wichFlag == PUB_SD) position = GROUP_SD;

	uint32_t flagsAddr = _getGrpAddr(wichGroup) + position;

	// Sanity check
	if (flagsAddr == 0) return -1;

	byte byteFlags = flash.readByte(flagsAddr);

	if (byteFlags == PUBLISHED) return 1;

	return 0;
}
uint8_t SckList::_countReadings(GroupIndex wichGroup, uint32_t grpAddr)
{
	// Sanity check
	if (grpAddr == 0) {
		if (debug) {
			sprintf(base->outBuff, "F: Wrong address (0) for group %i on sector %u", wichGroup.group, wichGroup.sector);
			base->sckOut();
		}
		return 0;
	}

	uint32_t finalGrpAddr = grpAddr + flash.readWord(grpAddr);
	uint32_t readingAddr = grpAddr + GROUP_READINGS;
	uint8_t readingCounter = 0;


	while (readingAddr < finalGrpAddr) {
		readingAddr += flash.readByte(readingAddr);
		readingCounter++;
	}

	if (debug) {
		sprintf(base->outBuff, "F: Founded %u readings on group %i of sector %u", readingCounter, wichGroup.group, wichGroup.sector);
		base->sckOut();
	}

	return readingCounter;
}

// Sector functions
uint32_t SckList::_getSectAddr(uint16_t wichSector)
{
	// Sanity check
	if (wichSector > SCKLIST_SECTOR_NUM) return 0xFFFFFFFF;

	return (uint32_t)wichSector * SECTOR_SIZE;
}
int16_t SckList::_getSectFreeSpace(uint16_t wichSector)
{
	if (debug) {
		sprintf(base->outBuff, "F: Calculating free space on sector %u", wichSector);
		base->sckOut();
	}

	uint32_t startAddr = _getSectAddr(wichSector);

	// Sanity check
	if (startAddr > (uint32_t)(SCKLIST_SECTOR_NUM * SECTOR_SIZE)) return -1;

	uint32_t endAddr = startAddr + SECTOR_SIZE;
	uint32_t address = startAddr + 3; 	// First tree bytes used for sector state and flags

	// Read from here until we found 0xFFFF
	while (address < endAddr) {

		uint16_t groupSize = flash.readWord(address);
		if (groupSize == 0xFFFF) break;
		else if (groupSize == 0) return -1;
		address += groupSize;
	}

	uint16_t freeSpace = endAddr - address;
	return freeSpace;
}
uint8_t SckList::_getSectState(uint16_t wichSector)
{
	uint32_t startAddr = _getSectAddr(wichSector);
	return flash.readByte(startAddr);
}
int8_t SckList::_setSectPublished(uint16_t wichSector, PubFlags wichFlag)
{

	// Choose byte position depending on requesed flag
	uint8_t position = SECTOR_NET;
	if (wichFlag == PUB_SD) position = SECTOR_SD;

	// Get sector flags Index
	uint32_t flagsAddr = _getSectAddr(wichSector) + position;

	// Sanity check
	if (flagsAddr > (uint32_t)(SCKLIST_SECTOR_NUM * SECTOR_SIZE)) return -1;

	// If sector is not in SECTOR_FULL state we dont accept the flag
	if (_getSectState(wichSector) != SECTOR_USED) return -1;

	// Only set flag if ALL groups on the sector are marked as published
	if (_countSectGroups(wichSector, wichFlag, NOT_PUBLISHED) > 0) return -1;

	// And write flags byte
	if (!flash.writeByte(flagsAddr, PUBLISHED)) return -1;


	if (debug) {
		sprintf(base->outBuff, "F: Marked sector %u %s", wichSector, wichFlag == PUB_NET ? " as network published" : " as sdcard saved");
		base->sckOut();
	}

	// Update sector status
	_scanSectors();

	return 1;
}
int8_t SckList::_closeSector(uint16_t wichSector)
{
	// Mark sector as full
	flash.writeByte(_getSectAddr(wichSector), SECTOR_USED);

	// If no readings left, mark sector as published
	if (_countSectGroups(wichSector, PUB_NET, NOT_PUBLISHED) == 0) _setSectPublished(_currSector, PUB_NET);
	if (_countSectGroups(wichSector, PUB_SD, NOT_PUBLISHED) == 0) _setSectPublished(_currSector, PUB_SD);

	return 1;
}
int8_t SckList::_isSectPublished(uint16_t wichSector, PubFlags wichFlag)
{
	// Choose byte position depending on requesed flag
	uint8_t position = SECTOR_NET;
	if (wichFlag == PUB_SD) position = SECTOR_SD;

	// Get sector flags Index
	uint32_t flagsAddr = _getSectAddr(wichSector) + position;

	// Sanity check
	if (flagsAddr > (uint32_t)(SCKLIST_SECTOR_NUM * SECTOR_SIZE)) return -1;

	byte byteFlags = flash.readByte(flagsAddr);
	if (byteFlags == PUBLISHED) return 1;
	else if (byteFlags == NOT_PUBLISHED) return 0;

	return -1;
}
int16_t SckList::_getUnpubGrpIdx(uint16_t wichSector, PubFlags wichFlag)
{
	if (debug) {
		sprintf(base->outBuff, "F: Searching for group not %s on sector %u: ", wichFlag == PUB_NET ? " published to network" : " saved to sdcard", wichSector);
		base->sckOut(PRIO_MED, false);
	}

	uint32_t startAddr = _getSectAddr(wichSector);

	// Sanity check
	if (startAddr > uint32_t(SCKLIST_SECTOR_NUM * SECTOR_SIZE)) return -1;

	uint32_t endAddr = startAddr + SECTOR_SIZE;
	uint32_t address = startAddr + 3; 	// First tree bytes used for sector state and flags

	int16_t thisGroup = 0;

	// Get right addr depending on requested flag
	uint8_t addPositionFlag = GROUP_NET;
	if (wichFlag == PUB_SD) addPositionFlag = GROUP_SD;

	// Read from the byte 2 until we found 0xFFFF or an unpublished group
	// TODO aqui hay un prblema cuando el ultimo grupo termina exactamente al final del sector y no queda ningún 0xFF antes del siguiente sector
	while (address < endAddr) {

		// Find out groupSize
		uint16_t groupSize = flash.readWord(address);
		if (groupSize == 0xFFFF || (address + groupSize) >= endAddr) {
			if (debug) base->sckOut("None found!!");

			// If this setor is not marked as fully published markt it!
			if (_getSectState(wichSector) == SECTOR_USED && !_isSectPublished(wichSector, wichFlag)) _setSectPublished(wichSector, wichFlag);

			return -1; 	// If GroupSize is not yet written that means no valid group is present
		}

		// Check if group is NOT published in the requested flag
		byte byteFlags = flash.readByte(address + addPositionFlag);
		if (byteFlags == NOT_PUBLISHED) break;

		address += groupSize;
		thisGroup++;
	}

	if (debug) {
		sprintf(base->outBuff, "Found group  with index: %i", thisGroup);
		base->sckOut();
	}
	return thisGroup;
}
void SckList::_scanSectors()
{

	_currSector = _dataAvailableSect[PUB_NET] = _dataAvailableSect[PUB_SD] = -1;

	if (debug) base->sckOut("F: Scanning sectors (used/empty):  ", PRIO_MED, false);

	for (uint16_t i=0; i<SCKLIST_SECTOR_NUM; i++) {

		uint8_t thisState = _getSectState(i);

		if (debug) base->sckOut(thisState == SECTOR_USED ? "u " : "e ", PRIO_MED, false);


		switch(thisState) {

			case SECTOR_USED:
			{
				// If we still don't have any sector with available data, we check if this one has any
				if (_dataAvailableSect[PUB_NET] < 0) {
					// If sector is not marked as published, search for available groups
					if (!_isSectPublished(i, PUB_NET)) {
						if (debug) base->sckOut("");
						if (_getUnpubGrpIdx(i, PUB_NET) >= 0) {
							if (debug) {
								sprintf(base->outBuff, "F: Network unpublished data founded on sector: %u", i);
								base->sckOut();
							}
							_dataAvailableSect[PUB_NET] = i;
						// If there is no unpublished data, mark sector as published.
						} else _setSectPublished(i, PUB_NET);
					}
				}
				// The same but for sdcard
				if (_dataAvailableSect[PUB_SD] < 0) {
					if (!_isSectPublished(i, PUB_SD)) {
						if (debug) base->sckOut("");
						if (_getUnpubGrpIdx(i, PUB_SD) >= 0) {
							if (debug) {
								sprintf(base->outBuff, "Data not saved to sdcard founded on sector: %u", i);
								base->sckOut();
							}
							_dataAvailableSect[PUB_SD] = i;
						} else _setSectPublished(i, PUB_SD);
					}
				}
				break;
			}
			case SECTOR_EMPTY:
			{
				if (debug) {
					sprintf(base->outBuff, " << current sector found: %u", i);
					base->sckOut();
				}

				_currSector = i;

				// Calculate current sector freespace
				int16_t freeSpace = _getSectFreeSpace(_currSector);

				// Check for errors while reading current sector
				if (freeSpace == -1) {
					if (debug) {
						sprintf(base->outBuff, "<ERROR (%u)", i);
						base->sckOut();
					}

					// Reuse sector
					flash.eraseSector(_getSectAddr(_currSector));
					freeSpace = SECTOR_SIZE - 3;
					return;
				}

				// Find starting point to continue writing
				_addr = _getSectAddr(_currSector) + SECTOR_SIZE - freeSpace;
				return;
			}
			default:
			{
				// This sector has an error, lets reuse it
				if (debug) {
					sprintf(base->outBuff, "<ERROR (%u)", i);
					base->sckOut();
				}

				_currSector = i;
				flash.eraseSector(_getSectAddr(_currSector));
				_addr = _getSectAddr(_currSector) + 3;
				return;
			}
		}
	}

	// If we didn't find a empty sector that means the flash memory is full
	// TODO We should find and start using the sector after the one with the most recent timestamp, but to do this we need a very in deep scanning
	// Start from first sector
	_currSector = 0;
	flash.eraseSector(_getSectAddr(_currSector));
	_addr = _getSectAddr(_currSector) + 3;

}
int16_t SckList::_countSectGroups(uint16_t wichSector, PubFlags wichFlag, byte publishedState, bool getAll)
{
	if (debug) {
		if (getAll) sprintf(base->outBuff, "F: Counting ALL groups in sector %u", wichSector);
		else sprintf(base->outBuff, "F: Counting %s %s groups on sector %u", wichFlag == PUB_NET ? "network" : "sdcard", publishedState == PUBLISHED ? " published" : " un-published", wichSector);
		base->sckOut();
	}

	uint32_t startAddr = _getSectAddr(wichSector);

	// Sanity check
	if (startAddr > uint32_t(SCKLIST_SECTOR_NUM * SECTOR_SIZE)) return -1;

	uint32_t endAddr = startAddr + SECTOR_SIZE;
	uint32_t address = startAddr + 3; 	// First tree bytes used for sector state and flags

	// Get right addrs depending on requested flag
	uint32_t addPositionFlag = GROUP_NET;
	if (wichFlag == PUB_SD) addPositionFlag = GROUP_SD;

	int16_t groupTotal = 0;

	// Read from the byte 2 until we found 0xFFFF or an unpublished group
	while (address < endAddr) {

		// Find out groupSize
		uint16_t groupSize = flash.readWord(address);
		if (groupSize == 0xFFFF) break; 	// If GroupSize is not yet written that means no valid group is present

		// Check if group is NOT published
		byte byteFlags = flash.readByte(address + addPositionFlag);
		if (byteFlags == publishedState || getAll) groupTotal++;


		address += groupSize;
	}

	return groupTotal;
}


uint8_t SckList::_formatSD(GroupIndex wichGroup, char* buffer)
{
	if (debug) base->sckOut("F: Preparing group data for sdcard saving");

	uint32_t grpAddr = _getGrpAddr(wichGroup);
	uint8_t readingNum = _countReadings(wichGroup, grpAddr);

	uint32_t thisTime = flash.readULong(grpAddr + GROUP_TIME);
	base->epoch2iso(thisTime, buffer); 		// print time stamp to buffer
	base->epoch2iso(thisTime, base->ISOtimeBuff); 	// Update base time buffer for console message.

	if (debug) {
		sprintf(base->outBuff, "F: (%s) -> ", base->ISOtimeBuff);
		base->sckOut(PRIO_MED, false);
	}

	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType wichSensorType = base->sensors.sensorsPriorized(i);

		if (base->sensors[wichSensorType].enabled) {

			bool found = false;
			uint32_t pos = grpAddr + GROUP_READINGS;
			for (uint8_t ii=0; ii<readingNum; ii++) {

				uint8_t readSize = flash.readByte(pos); // Get reading size
				SensorType thisType = static_cast<SensorType>(flash.readByte(pos + 1)); 	// Get sensorType

				if (thisType == wichSensorType) {

					// Get the reading value
					String thisReading;
					for (uint32_t r=pos+2; r<pos+readSize; r++) thisReading.concat((char)flash.readByte(r));

					if (debug) {
						sprintf(base->outBuff, "%s %s %s, ", base->sensors[thisType].title, thisReading.c_str(), base->sensors[thisType].unit);
						base->sckOut(PRIO_MED, false);
					}

					sprintf(buffer + strlen(buffer), ",%s", thisReading.c_str());
					found = true;
					break;
				}
				pos += readSize;
			}
			if (!found) sprintf(buffer + strlen(buffer), ",null");
		}
	}
	sprintf(buffer + strlen(buffer), "\r\n"); 	// print newline to buffer

	if (debug) base->sckOut("<-");

	return readingNum;
}
uint8_t SckList::_formatNET(GroupIndex wichGroup, char* buffer)
{
	if (debug) base->sckOut("F: Preparing group data for network publishing");

	// /* Example
	// {	t:2017-03-24T13:35:14Z,
	// 		29:48.45,
	// 		13:66,
	// 		12:28,
	// 		10:4.45
	// }

	uint32_t grpAddr = _getGrpAddr(wichGroup);
	uint8_t readingNum = _countReadings(wichGroup, grpAddr);

	// Prepare buffer for ESP format
	sprintf(buffer, "%c", ESPMES_MQTT_PUBLISH); 	// Write command to buffer

	// Write time
	base->epoch2iso(flash.readULong(grpAddr + GROUP_TIME), base->ISOtimeBuff);
	sprintf(buffer + strlen(buffer), "{t:%s", base->ISOtimeBuff);

	if (debug) {
		sprintf(base->outBuff, "F: (%s) -> ", base->ISOtimeBuff);
		base->sckOut(PRIO_MED, false);
	}

	grpAddr += GROUP_READINGS;  // Jump to first reading
	// Write sensor readings
	for (uint8_t i=0; i<readingNum; i++) {

		uint8_t readSize = flash.readByte(grpAddr); // Get reading size (full size - size and flags)
		SensorType thisType = static_cast<SensorType>(flash.readByte(grpAddr+1)); 	// Get sensorType

		// Get the reading value
		String thisReading;
		for (uint32_t r=grpAddr+2; r<grpAddr+readSize; r++) thisReading.concat((char)flash.readByte(r));

		if (base->sensors[thisType].id > 0 && !thisReading.startsWith("null")) {

			if (debug) {
				sprintf(base->outBuff, "%s %s %s, ", base->sensors[thisType].title, thisReading.c_str(), base->sensors[thisType].unit);
				base->sckOut(PRIO_MED, false);
			}

			sprintf(buffer + strlen(buffer), ",%u:%s", base->sensors[thisType].id, thisReading.c_str());
		}

		grpAddr += readSize;
	}
	sprintf(buffer + strlen(buffer), "}");

	if (debug) base->sckOut("<-");

	return readingNum;
}

// Public fnctions
int8_t SckList::setup()
{
	debug = base->config.debug.flash;
	return _flashStart();
}
bool SckList::flashFormat()
{
	return _flashFormat();
}
void SckList::flashUpdate()
{
	_scanSectors();
}
uint8_t SckList::saveGroup()
{

	// First prepare the group to be saved (on ram buffer)

	// Init tags in NOT_PUBLISHED for all
	byte finit = 0xFF;
	memcpy(&flashBuff[GROUP_NET], &finit, 1);
	memcpy(&flashBuff[GROUP_SD], &finit, 1);

	// Store timeStamp of current group
	memcpy(&flashBuff[GROUP_TIME], &base->lastSensorUpdate, 4);  	// Write timeStamp (4 bytes)

	base->epoch2iso(base->lastSensorUpdate, base->ISOtimeBuff);
	if (debug) {
		sprintf(base->outBuff, "F: Saving group (%s) -> ", base->ISOtimeBuff);
		base->sckOut(PRIO_MED, false);
	}

	// Store sensor readings
	uint16_t pos = GROUP_READINGS; 	// Variable to store the buffer index position
	uint8_t enabledSensors = 0;
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {

		SensorType stype = static_cast<SensorType>(i);

		if (base->sensors[stype].enabled && base->sensors[stype].lastReadingTime == base->lastSensorUpdate) { 			// If sensor is enabled and a reading has been taken in last loop

			String value = base->sensors[stype].reading;

			if (!value.startsWith("null")) {

				uint8_t vsize = value.length() + 1 + 1; 				// Value.length + Sensortype + size byte
				memcpy(&flashBuff[pos], &vsize, 1); pos+=1;				// Size (1 byte)
				memcpy(&flashBuff[pos], &stype, 1); pos+=1;				// SensorType (1 byte)

				if (debug) {
					sprintf(base->outBuff, "%s %s %s, ", base->sensors[stype].title, value.c_str(), base->sensors[stype].unit);
					base->sckOut(PRIO_MED, false);
				}

				for (uint8_t c=0; c<value.length(); c++) {
					char thischar = value.charAt(c);
					memcpy(&flashBuff[pos], &thischar, 1);			// Reading value char by char
					pos+=1;
				}
				enabledSensors++;
			}
		}
		if (enabledSensors == 0) return enabledSensors;
	}
	if (debug) base->sckOut("<-");

	// Save group size at the begining of the group
	memcpy(&flashBuff[GROUP_SIZE], &pos, 2);

	// Check if new group fits in current sector
	if (_getSectFreeSpace(_currSector) < pos) {

		if (debug) {
			sprintf(base->outBuff, "F: Sector %u doesn't have enough free space, will search for a new one.", _currSector);
			base->sckOut();
		}

		// Close full sector
		_closeSector(_currSector);

		// _scanSectors will find the next availablew sector to be used
		_scanSectors();


		// If selected sector doesn't have enough space, close it and keep scanning until we found a sector that fits the data
		while (_getSectFreeSpace(_currSector) < pos) {
			_closeSector(_currSector);
			_scanSectors();
		}

		if (debug) {
			sprintf(base->outBuff, "F: Using sector %u in address %lu", _currSector, _addr);
			base->sckOut();
		}
	}

	// Copy buffer to flash memory
	for (uint16_t i=0; i<pos; i++) _append(flashBuff[i]);

	availableReadings[PUB_NET] = true;
	availableReadings[PUB_SD] = true;

	if (debug) {
		sprintf(base->outBuff, "F: Saved %u sensors on sector %u", enabledSensors, _currSector);
		base->sckOut();
	}

	return enabledSensors;
}
SckList::GroupIndex SckList::readGroup(PubFlags wichFlag, GroupIndex forceIndex)
{
	GroupIndex thisGroup;

	if (forceIndex.group < 0) {

		if (debug) {
			sprintf(base->outBuff, "F: Searching for unpublished group for tag %u", wichFlag);
			base->sckOut();
		}

		// Find a sector with available data
		if (_dataAvailableSect[wichFlag] >= 0) thisGroup.sector = _dataAvailableSect[wichFlag];
		else thisGroup.sector = _currSector;


		// try to find unpublished group inside sector
		thisGroup.group = _getUnpubGrpIdx(thisGroup.sector, wichFlag);

		// If there is no group available
		if (thisGroup.group < 0) {
			if (debug) base->sckOut("F: Can't find unpublished group!!!");
			availableReadings[wichFlag] = false;
			return thisGroup;
		}
	} else {

		thisGroup = forceIndex;
	}

	uint8_t readingNum = 0;
	if (wichFlag == PUB_SD) readingNum = _formatSD(thisGroup, flashBuff);
	else if (wichFlag == PUB_NET) readingNum = _formatNET(thisGroup, base->netBuff);

	if (readingNum > 0) {

		if (debug) {
			sprintf(base->outBuff, "F: Reading group %i in sector %u with %u readings.", thisGroup.group, thisGroup.sector, readingNum);
			base->sckOut();
		}
		return thisGroup;
	}


	if (debug) base->sckOut("F: No readings available for this group!!");

	thisGroup.group = -1;
	return thisGroup;
}
uint8_t SckList::setPublished(GroupIndex wichGroup, PubFlags wichFlag)
{
	if (debug) {
		sprintf(base->outBuff, "F: Marking group %i in sector %u %s", wichGroup.group, wichGroup.sector, wichFlag == PUB_NET ? " as network published" : " as saved on sdcard");
		base->sckOut();
	}

	_setGrpPublished(wichGroup, wichFlag);

	// Try to mark this sector as published (flag will be rejected if not all groups are published)
	_setSectPublished(wichGroup.sector, wichFlag);

	return _countReadings(wichGroup, _getGrpAddr(wichGroup));
}
uint32_t SckList::countGroups(PubFlags wichFlag)
{
	if (debug) {
		sprintf(base->outBuff, "F: Counting readings not %s", wichFlag == PUB_NET ? " published to the network" : " saved to sdcard");
		base->sckOut();
	}

	uint16_t groupTotal = 0;

	for (uint16_t i=0; i<SCKLIST_SECTOR_NUM; i++) {

		uint8_t thisState = _getSectState(i);

		if (!_isSectPublished(i, wichFlag)) {
			int16_t thisSectGroups = _countSectGroups(i, wichFlag, NOT_PUBLISHED);
			if (thisSectGroups > 0)	groupTotal += thisSectGroups;
		}

		if (thisState == SECTOR_EMPTY) break;
	}
	return groupTotal;
}

uint16_t SckList::recover(uint16_t wichSector, PubFlags wichFlag)
{
	if (debug) {
		sprintf(base->outBuff, "F: Recovering groups on sector %u and %s", wichSector, wichFlag == PUB_NET ? "publishing them to the network" : "saving them to sdcard");
		base->sckOut();
	}

	uint16_t groupNum = _countSectGroups(wichSector, PUB_NET, PUBLISHED, true);
	uint16_t totalRecovered = 0;

	for (int16_t i=0; i<groupNum; i++) {

		// prepare the flash group
		GroupIndex thisGroup = {wichSector, i};
		GroupIndex tryingGroup = readGroup(wichFlag, thisGroup);

		if (wichFlag == PUB_SD) {

			// Save to sdcard
			if (!base->sdPublish()) {
				sprintf(base->outBuff, "Group %u of sector %u saving to sd-card ERROR!!", i, wichSector);
				base->sckOut();
			} else {
				_setGrpPublished(thisGroup, wichFlag);
				totalRecovered++;
				base->epoch2iso(flash.readULong(_getGrpAddr(tryingGroup) + GROUP_TIME), base->ISOtimeBuff); 	// print time stamp to buffer
				sprintf(base->outBuff, "(%s) - Group %u of sector %u saved to sd-card OK!", base->ISOtimeBuff, i, wichSector);
				base->sckOut();
			}
		} else {
			// Make sure ESP is ready
			if (!base->st.espON) {
				base->ESPcontrol(base->ESP_ON);
				while (base->st.espBooting) base->ESPbusUpdate();
			}

			// Send MQTT and wait for response or timeout (10 seconds)
			uint32_t timeout = millis();
			while (millis() - timeout < 10000) {

				if (base->st.publishStat.retry()) base->sendMessage();

				base->ESPbusUpdate();
				if (base->st.publishStat.ok) {
					_setGrpPublished(thisGroup, wichFlag);
					totalRecovered++;
					base->st.publishStat.reset();
					base->epoch2iso(flash.readULong(_getGrpAddr(tryingGroup) + GROUP_TIME), base->ISOtimeBuff); 	// print time stamp to buffer
					sprintf(base->outBuff, "(%s) - Group %u of sector %u published OK!", base->ISOtimeBuff, i, wichSector);
					base->sckOut();
					break;
				}
				if (base->st.publishStat.error) {
					sprintf(base->outBuff, "Group %u of sector %u publish ERROR!!", i, wichSector);
					base->sckOut();
					base->st.publishStat.reset();
					break;
				}
			}
		}
	}
	return totalRecovered;
}
SckList::SectorInfo SckList::sectorInfo(uint16_t wichSector)
{
	if (debug) {
		sprintf(base->outBuff, "F: Scanning sector %u", wichSector);
		base->sckOut();
	}

	SectorInfo info;

	if (wichSector > SCKLIST_SECTOR_NUM) return info;

	info.used = _getSectState(wichSector) == SECTOR_USED ? true : false;
	info.current = wichSector == _currSector ? true : false;
	info.pubNet = _isSectPublished(wichSector, PUB_NET);
	info.pubSd = _isSectPublished(wichSector, PUB_SD);
	info.grpUnPubNet = _countSectGroups(wichSector, PUB_NET, NOT_PUBLISHED);
	info.grpPubNet = _countSectGroups(wichSector, PUB_NET, PUBLISHED);
	info.grpUnPubSd = _countSectGroups(wichSector, PUB_SD, NOT_PUBLISHED);
	info.grpPubSd = _countSectGroups(wichSector, PUB_SD, PUBLISHED);
	info.freeSpace = _getSectFreeSpace(wichSector);
	info.addr = _getSectAddr(wichSector);
	uint16_t totalGroups = _countSectGroups(wichSector, PUB_NET, PUBLISHED, true);
	if (totalGroups > 0) {
		info.firstTime = flash.readULong(_getGrpAddr(firstGroup) + GROUP_TIME);

		GroupIndex lastGroup;
		lastGroup.sector = wichSector;
		lastGroup.group = totalGroups - 1;
		info.lastTime = flash.readULong(_getGrpAddr(lastGroup) + GROUP_TIME);
	}

	return info;
}
void SckList::dumpSector(uint16_t wichSector, uint16_t howMany) // listo
{
	SerialUSB.print("F: HEX dump of sector ");
	SerialUSB.print(wichSector);
	SerialUSB.print(" starting on address ");
	SerialUSB.print(_getSectAddr(wichSector));
	SerialUSB.println(": ");

	for (uint32_t i=_getSectAddr(wichSector); i<(_getSectAddr(wichSector)+howMany); i++) {
		byte readed = flash.readByte(i);
		SerialUSB.print(readed, HEX);
		if (readed < 16) SerialUSB.print(" ");
		SerialUSB.print(" ");
	}
	SerialUSB.println("");
}
SckList::FlashInfo SckList::flashInfo()
{
	if (debug) base->sckOut("F: Scanning flash memory sectors");

	FlashInfo info;

	bool firstEmpty = true;

	for (uint16_t i=0; i<SCKLIST_SECTOR_NUM; i++) {

		byte state = _getSectState(i);
		if (state == SECTOR_USED || (state == SECTOR_EMPTY && firstEmpty)) {

			if (state == SECTOR_EMPTY) {

				firstEmpty = false;
				info.sectFree++;
				base->sckOut("U", PRIO_HIGH, false);

			} else {
				info.sectUsed++;
				base->sckOut("u", PRIO_HIGH, false);
			}

			info.grpTotal += _countSectGroups(i, PUB_NET, PUBLISHED, true);
			info.grpPubNet += _countSectGroups(i, PUB_NET, PUBLISHED);
			info.grpUnPubNet += _countSectGroups(i, PUB_NET, NOT_PUBLISHED);
			info.grpPubSd += _countSectGroups(i, PUB_SD, PUBLISHED);
			info.grpUnPubSd += _countSectGroups(i, PUB_SD, NOT_PUBLISHED);

		} else if (state == SECTOR_EMPTY)  {
			info.sectFree++;
			base->sckOut("f", PRIO_HIGH, false);
		}

		base->sckOut(".", PRIO_HIGH, false);
	}

	info.currSector = _currSector;

	return info;
}

