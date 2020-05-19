#include "SckList.h"
#include "SckBase.h"

#define debug_print(__x);  if (debug) Serial.print(__x);
#define debug_println(__x);  if (debug) Serial.println(__x);

// General flash utilities
int8_t SckList::_flashStart()
{

	debug_println("F: Starting memory");

	digitalWrite(pinCS_SDCARD, HIGH);	// disables SDcard
	digitalWrite(pinCS_FLASH, LOW);
	if (!flash.begin()) return -1;
	flash.setClock(133000);

	_scanSectors();

	// If no current sector found that means we never used the flash before or some really bad problem has ocurred, so we format the flash and start from scratch
	if (_currSector == -1) {

		dumpSector(0);

		debug_println("F: Memory is not formated or damaged!!");

		// TODO recover data in case it exists and find a way to update current sector
		// maybe we can avoid format by just erasing some sectors to recover sanity

		_flashFormat();
		return 1;
	}

	debug_println("F: Started OK");
	return 0;
}
bool SckList::_flashFormat()
{
	debug_println("F: Formating...");

	if (!flash.eraseChip()) return false;

	_currSector = 0;
	_addr = 3;

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

	debug_print("F: Marking group ");
	debug_print(wichGroup.group);
	debug_print(" in sector ");
	debug_print(wichGroup.sector);
	debug_print(" as ");
	debug_println(wichFlag == PUB_NET ? "network published" : "sdcard saved");

	uint32_t flagsAddr = _getGrpAddr(wichGroup) + position;

	// Sanity check
	if (flagsAddr == 0) return -1;

	byte byteFlags = flash.readByte(flagsAddr);

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
uint8_t SckList::_countReadings(GroupIndex wichGroup)
{
	uint32_t grpAddr = _getGrpAddr(wichGroup);

	// Sanity check
	if (grpAddr == 0) {
		debug_print("Wrong address (0) for group ");
		debug_print(wichGroup.group);
		debug_print(" on sector ");
		debug_println(wichGroup.sector);
		return 0;
	}

	uint32_t finalGrpAddr = grpAddr + flash.readWord(grpAddr);
	uint32_t readingAddr = grpAddr + 2 + 2 + 6; // 2: grp size, 2: grpFlags, 6: timestamp
	uint8_t readingCounter = 0;


	while (readingAddr < finalGrpAddr) {
		readingAddr += flash.readByte(readingAddr);
		readingCounter++;
	}

	debug_print("F: Founded ");
	debug_print(readingCounter)
	debug_print(" readings in group ");
	debug_print(wichGroup.group);
	debug_print(" of sector ");
	debug_println(wichGroup.sector);

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
	debug_print("F: Calculating free space on sector ");
	debug_println(wichSector);

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

	debug_print("F: Marked sector ")
	debug_print(wichSector);
	debug_println(wichFlag == PUB_NET ? " as network published" : " as sdcard saved");

	// Update sector status
	_scanSectors();

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
	debug_print("F: Searching for group not");
	debug_print(wichFlag == PUB_NET ? " published to network" : " saved to sdcard");
	debug_print(" on sector ");
	debug_print(wichSector);
	debug_print(": ");

	uint32_t startAddr = _getSectAddr(wichSector);

	// Sanity check
	if (startAddr > uint32_t(SCKLIST_SECTOR_NUM * SECTOR_SIZE)) return -1;

	uint32_t endAddr = startAddr + SECTOR_SIZE;
	uint32_t address = startAddr + 3; 	// First tree bytes used for sector state and flags

	int16_t thisGroup = 0;

	// Get right addrs depending on requested flag
	uint8_t addPositionFlag = GROUP_NET;
	if (wichFlag == PUB_SD) addPositionFlag = GROUP_SD;

	// Read from the byte 2 until we found 0xFFFF or an unpublished group
	// TODO aqui hay un prblema cuando el ultimo grupo termina exactamente al final del sector y no queda ningún 0xFF antes del siguiente sector
	while (address < endAddr) {

		// Find out groupSize
		uint16_t groupSize = flash.readWord(address);
		if (groupSize == 0xFFFF || (address + groupSize) >= endAddr) {
			debug_println("None found!!");

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

	debug_print("Found group  with index: ");
	debug_println(thisGroup);
	return thisGroup;
}
void SckList::_scanSectors()
{

	_currSector = _dataAvailableSect[PUB_NET] = _dataAvailableSect[PUB_SD] = -1;

	debug_print("F: Scanning sectors (used/empty):  ");

	for (uint16_t i=0; i<SCKLIST_SECTOR_NUM; i++) {

		uint8_t thisState = _getSectState(i);

		debug_print(thisState == SECTOR_USED ? "u " : "e ");


		switch(thisState) {

			case SECTOR_USED:
			{

				if (_dataAvailableSect[PUB_NET] < 0) if (!_isSectPublished(i, PUB_NET)) _dataAvailableSect[PUB_NET] = i;
				if (_dataAvailableSect[PUB_SD] < 0) if (!_isSectPublished(i, PUB_SD)) _dataAvailableSect[PUB_SD] = i;
				break;
			}
			case SECTOR_EMPTY:
			{
				debug_print(" << current sector found: ");
				debug_println(i);

				_currSector = i;

				// Calculate current sector freespace
				int16_t freeSpace = _getSectFreeSpace(_currSector);

				// Check for errors while reading current sector
				if (freeSpace == -1) {
					debug_print("<ERROR (");
					debug_print(i);
					debug_println(") ");
					_currSector = -1;
					return;
				}

				// Find starting point to continue writing
				_addr = _getSectAddr(_currSector) + SECTOR_SIZE - freeSpace;
				return;
			}
			default:

				// Something went wrong!
				debug_print("<ERROR (");
				debug_print(i);
				debug_println(") ");
				_currSector = -1;
				return;
		}
	}
}
int16_t SckList::_countSectGroups(uint16_t wichSector, PubFlags wichFlag, byte publishedState, bool getAll)
{
	if (getAll) {
		debug_print("F: Counting ALL groups in sector ");
		debug_println(wichSector);
	} else {
		debug_print("F: Counting ");
		debug_print(wichFlag == PUB_NET ? "network" : "sdcard");
		debug_print(publishedState == PUBLISHED ? " published" : " un-published");
		debug_print(" groups on sector ");
		debug_println(wichSector);
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
	debug_print("F: Saving group (");

	// First prepare the group to be saved (on ram buffer)
	// Variable to store the buffer index position
	uint16_t pos = 2; 	// The first two bytes are a placeholder to store the size of the group once we know it

	// Init tags in NOT_PUBLISHED for all
	byte finit = 0xFF;
	memcpy(&flashBuff[pos], &finit, 1); pos+=1;
	memcpy(&flashBuff[pos], &finit, 1); pos+=1;

	// Store timeStamp of current group
	uint8_t timeSize = 6; 						// 1: size + 1: SensorType + 4: reading = 6 total
	memcpy(&flashBuff[pos], &timeSize, 1); pos+=1; 			// Write Size of value (1 byte)
	SensorType stype = SENSOR_COUNT;
	memcpy(&flashBuff, &stype, 1); pos+=1; 				// Write SensorType (1 byte)
	memcpy(&flashBuff[pos], &base->lastSensorUpdate, 4); pos+=4; 	// Write timeStamp (4 bytes)

	base->epoch2iso(base->lastSensorUpdate, base->ISOtimeBuff);
	debug_print(base->ISOtimeBuff);
	debug_print(") -> ");

	// Store sensor readings
	uint8_t enabledSensors = 0;
	for (uint8_t i=0; i<SENSOR_COUNT; i++) {
		if (base->sensors[static_cast<SensorType>(i)].enabled) { 			// If sensor is enabled

			String value = base->sensors[static_cast<SensorType>(i)].reading;
			uint8_t vsize = value.length() + 1 + 1; 				// Value.length + Sensortype + size byte
			memcpy(&flashBuff[pos], &vsize, 1); pos+=1;				// Size (1 byte)
			stype = static_cast<SensorType>(i);
			memcpy(&flashBuff[pos], &stype, 1); pos+=1;				// SensorType (1 byte)

			debug_print(base->sensors[stype].title);
			debug_print(" ");
			debug_print(value);
			debug_print(base->sensors[stype].unit);
			debug_print(", ")

			for (uint8_t c=0; c<value.length(); c++) {
				char thischar = value.charAt(c);
				memcpy(&flashBuff[pos], &thischar, 1);			// Reading value char by char
				pos+=1;
			}
			enabledSensors++;
		}
		if (enabledSensors == 0) return enabledSensors;
	}
	debug_println("<-");

	// Save group size at the begining of the group
	memcpy(&flashBuff[0], &pos, 2);

	// Check if new group fits in current sector
	if (_getSectFreeSpace(_currSector) < pos) {

		debug_print("F: Sector");
		debug_print(_currSector);
		debug_println(" doesn't have enough free space, will search for a new one.");

		// Mark current sector as full
		flash.writeByte(_getSectAddr(_currSector), SECTOR_USED);

		// If no readings left mark sector as published
		if (_countSectGroups(_currSector, PUB_NET, NOT_PUBLISHED) == 0) _setSectPublished(_currSector, PUB_NET);
		if (_countSectGroups(_currSector, PUB_SD, NOT_PUBLISHED) == 0) _setSectPublished(_currSector, PUB_SD);

		// Start a new sector
		if (_currSector == SCKLIST_SECTOR_NUM) _currSector = 0; // Flash is full... start again.
		else _currSector++;

		_addr = _getSectAddr(_currSector);

		// If sector is not empty, erase it
		if (_getSectState(_currSector) == SECTOR_USED) flash.eraseSector(_addr);

		// Jump sector state and flag bytes
		_addr += 3;
		debug_print("Using sector ");
		debug_print(_currSector);
		debug_print(" in address ");
		debug_println(_addr);
	}

	// Copy buffer to flash memory
	for (uint16_t i=0; i<pos; i++) _append(flashBuff[i]);

	availableReadings[PUB_NET] = true;
	availableReadings[PUB_SD] = true;

	debug_print("F: Saved ");
	debug_print(enabledSensors);
	debug_print(" sensors on sector ");
	debug_println(_currSector);

	return enabledSensors;
}
SckList::GroupIndex SckList::readGroup(PubFlags wichFlag, GroupIndex forceIndex)
{
	GroupIndex thisGroup;

	if (forceIndex.group < 0) {

		debug_print("F: Searching for unpublished group for tag ");
		debug_println(wichFlag);

		// Find a sector with available data
		if (_dataAvailableSect[wichFlag] >= 0) thisGroup.sector = _dataAvailableSect[wichFlag];
		else thisGroup.sector = _currSector;


		// try to find unpublished group inside sector
		thisGroup.group = _getUnpubGrpIdx(thisGroup.sector, wichFlag);

		// If there is no group available
		if (thisGroup.group < 0) {
			debug_println("F: Can't find unpublished group!!!");
			availableReadings[wichFlag] = false;
			return thisGroup;
		}
	} else {

		thisGroup = forceIndex;
	}

	// prepare group data depending on provided flag
	uint8_t readingNum = _countReadings(thisGroup);

	debug_print("F: Reading group ");
	debug_print(thisGroup.group);
	debug_print(" in sector ");
	debug_print(thisGroup.sector);
	debug_print(" with ");
	debug_print(readingNum);
	debug_println(" readings.");

	if (readingNum > 0) {

		uint32_t grpAddr = _getGrpAddr(thisGroup);

		if (wichFlag == PUB_SD) {

			debug_println("F: Preparing group data for sdcard saving");

			uint32_t thisTime = flash.readULong(grpAddr + 6);
			base->epoch2iso(thisTime, flashBuff); 		// print time stamp to buffer
			base->epoch2iso(thisTime, base->ISOtimeBuff); 	// Update base time buffer for console message.
			grpAddr += 10;  // Jump to first reading (2 size + 2 flags + 6 timestamp = 10)

			debug_print("F: Reading group (");
			for (uint8_t i=0; i<19; i++) {
				debug_print(flashBuff[i]);
			}
			debug_print(") -> ");

			for (uint8_t i=0; i<SENSOR_COUNT; i++) {
				SensorType wichSensorType = base->sensors.sensorsPriorized(i);

				if (base->sensors[wichSensorType].enabled) {

					bool found = false;
					uint32_t tempAddr = grpAddr;
					for (uint8_t ii=0; ii<readingNum; ii++) {

						SensorType thisType = static_cast<SensorType>(flash.readByte(tempAddr + 1)); 	// Get sensorType
						uint8_t readSize = flash.readByte(tempAddr); // Get reading size

						if (thisType == wichSensorType) {

							// Get the reading value
							String thisReading;
							for (uint32_t r=tempAddr+2; r<=tempAddr+readSize; r++) thisReading.concat((char)flash.readByte(r));

							debug_print(base->sensors[thisType].title);
							debug_print(" ");
							debug_print(thisReading);
							debug_print(" ");
							debug_print(base->sensors[thisType].unit);
							debug_print(", ");

							sprintf(flashBuff + strlen(flashBuff)-1, ",%s", thisReading.c_str());
							found = true;
							break;
						}
						tempAddr += readSize;
					}
					if (!found) sprintf(flashBuff + strlen(flashBuff)-1, ",null");
				}
			}
			sprintf(flashBuff + strlen(flashBuff)-1, "\r\n"); 	// print newline to flashBuff

			debug_println("<-");

		} else if (wichFlag == PUB_NET) {

			debug_println("F: Preparing group data for network publishing");

			// /* Example
			// {	t:2017-03-24T13:35:14Z,
			// 		29:48.45,
			// 		13:66,
			// 		12:28,
			// 		10:4.45
			// }

			// Prepare netBuff for ESP format
			memset(base->netBuff, 0, sizeof(base->netBuff)); 		// Clear netBuff
			sprintf(base->netBuff, "%c", ESPMES_MQTT_PUBLISH); 		// Write command to netBuff

			// Write time
			base->epoch2iso(flash.readULong(grpAddr + 6), base->ISOtimeBuff);
			sprintf(base->netBuff + strlen(base->netBuff), "{t:%s", base->ISOtimeBuff);
			grpAddr += 10;  // Jump to first reading (2 size + 2 flags + 6 timestamp = 10)

			debug_print("F: Reading group (");
			debug_print(base->ISOtimeBuff);
			debug_print(") ->");

			// Write sensor readings
			for (uint8_t i=0; i<readingNum; i++) {

				SensorType thisType = static_cast<SensorType>(flash.readByte(grpAddr+1)); 	// Get sensorType
				uint8_t readSize = flash.readByte(grpAddr); // Get reading size (full size - size and flags)

				// Get the reading value
				String thisReading;
				for (uint32_t r=grpAddr+2; r<grpAddr+readSize; r++) thisReading.concat((char)flash.readByte(r));

				if (base->sensors[thisType].id > 0 && !thisReading.startsWith("null")) {

					debug_print(base->sensors[thisType].title);
					debug_print(" ");
					debug_print(thisReading);
					debug_print(" ");
					debug_print(base->sensors[thisType].unit);
					debug_print(", ");

					sprintf(base->netBuff + strlen(base->netBuff), ",%u:%s", base->sensors[thisType].id, thisReading.c_str());
				}

				grpAddr += readSize;
			}
			sprintf(base->netBuff + strlen(base->netBuff), "}");

			debug_println("<-");
		}

		return thisGroup;
	}

	debug_println("F: No readings available for this group!!");

	thisGroup.group = -1;
	return thisGroup;
}
uint8_t SckList::setPublished(GroupIndex wichGroup, PubFlags wichFlag)
{
	debug_print("F: Marking group ");
	debug_print(wichGroup.group);
	debug_print(" in sector ");
	debug_print(wichGroup.sector);
	debug_println(wichFlag == PUB_NET ? " as network published" : " as saved on sdcard");

	_setGrpPublished(wichGroup, wichFlag);

	// Try to mark this sector as published (flag will be rejected if not all groups are published)
	_setSectPublished(wichGroup.sector, wichFlag);

	return _countReadings(wichGroup);
}
uint32_t SckList::countGroups(PubFlags wichFlag)
{
	debug_print("F: Counting readings not");
	debug_println(wichFlag == PUB_NET ? " published to the network" : " saved to sdcard");

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
	debug_print("F: Recovering groups on sector ");
	debug_print(wichSector);
	debug_print(" and ");
	debug_println(wichFlag == PUB_NET ? "publishing them to the network" : "saving them to sdcard");

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
				base->epoch2iso(flash.readULong(_getGrpAddr(tryingGroup) + 6), base->ISOtimeBuff); 	// print time stamp to buffer
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
					base->epoch2iso(flash.readULong(_getGrpAddr(tryingGroup) + 6), base->ISOtimeBuff); 	// print time stamp to buffer
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
	debug_print("F: Scanning sector ");
	debug_println(wichSector);

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
		info.firstTime = flash.readULong(_getSectAddr(wichSector) + 9);
		GroupIndex lastGroup;
		lastGroup.sector = wichSector;
		lastGroup.group = totalGroups - 1;
		info.lastTime = flash.readULong(_getGrpAddr(lastGroup) + 6);
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
	debug_println("F: Scanning flash memory sectors");

	FlashInfo info;

	bool firstEmpty = true;

	for (uint16_t i=0; i<SCKLIST_SECTOR_NUM; i++) {

		byte state = _getSectState(i);
		if (state == SECTOR_USED || (state == SECTOR_EMPTY && firstEmpty)) {

			if (state == SECTOR_EMPTY) {

				firstEmpty = false;
				info.sectFree++;

			} else info.sectUsed++;

			info.grpTotal += _countSectGroups(i, PUB_NET, PUBLISHED, true);
			info.grpPubNet += _countSectGroups(i, PUB_NET, PUBLISHED);
			info.grpUnPubNet += _countSectGroups(i, PUB_NET, NOT_PUBLISHED);
			info.grpPubSd += _countSectGroups(i, PUB_SD, PUBLISHED);
			info.grpUnPubSd += _countSectGroups(i, PUB_SD, NOT_PUBLISHED);

		} else if (state == SECTOR_EMPTY) info.sectFree++;
	}

	info.currSector = _currSector;

	return info;
}
