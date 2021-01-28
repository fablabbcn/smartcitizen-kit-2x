#pragma once

#include <Arduino.h>
#include <SPIFlash.h> 	// https://github.com/Marzogh/SPIMemory

#include "Sensors.h"
#include "Pins.h"
#include "Shared.h"

// Number of bytes to be used on FLASH (Always keep this a littel bit smaller than real size)
#define SCKLIST_FLASH_SIZE 8388608
#define SECTOR_SIZE 4096
#define SCKLIST_RESERVED_SECTOR_COUNT 8 	// Number of sectors to be reserved (at the end of memory) for other uses
#define SCKLIST_SECTOR_NUM (SCKLIST_FLASH_SIZE/SECTOR_SIZE)-SCKLIST_RESERVED_SECTOR_COUNT 	// 2048 - 8 = 2040

#define SECTOR_EMPTY 0xFF
#define SECTOR_USED 0x00
#define NOT_PUBLISHED 0xFF
#define PUBLISHED 0x00

#define FLASH_DEBUG  	// Uncoment to enable debug messages

class SckBase;

class SckList
{
	public:

		struct GroupIndex {
			int16_t sector;
			int16_t group;
			uint32_t address;
		};

		enum PubFlags {
			PUB_NET,
			PUB_SD,
			PUB_BOTH
		};

		enum GroupAddr {
			GROUP_SIZE = 		0x00,
			GROUP_NET  = 		0x02,
			GROUP_SD   = 		0x03,
			GROUP_TIME = 		0x04,
			GROUP_READINGS = 	0x08
		};

		struct SectorInfo {
			bool used = false;
			bool current = false;
			bool pubNet = false;
			bool pubSd = false;
			uint16_t grpUnPubNet = 0;
			uint16_t grpPubNet = 0;
			uint16_t grpUnPubSd = 0;
			uint16_t grpPubSd = 0;
			uint16_t grpTotal = 0;
			uint16_t freeSpace = 0;
			uint32_t addr;
			uint32_t firstTime = 0;
			uint32_t lastTime = 0;
		};

		struct FlashInfo {
			uint16_t sectFree = 0;
			uint16_t sectUsed = 0;
			uint32_t grpUnPubNet = 0;
			uint32_t grpPubNet = 0;
			uint32_t grpUnPubSd = 0;
			uint32_t grpPubSd = 0;
			uint32_t grpTotal = 0;
			uint16_t currSector = 0;
		};

		SckList(SckBase* nBase) {
			base = nBase;
		};

		bool availableReadings[2] = {false, false}; 	// " places for PUB_NET and PUB_SD"
		bool debug = false;
		char flashBuff[NETBUFF_SIZE];

		int8_t setup(); 			// Starts flash memory. returns 0 on success, 1 on success but flash has been formated or -1 on error
		bool flashFormat(); 			// Erases the flash memory and starts it again
		void flashUpdate(); 			// After SCK config changes (ej. from sd card mode to net mode) updating flash indexes (or reseting the kit) is mandatory to find all readings.
		GroupIndex saveGroup(); 		// Saves group to flash memory, return group index
		GroupIndex readGroup(PubFlags wichFlag, GroupIndex forceIndex={-1,-1,0}); // Copies to buffer the next available group that hasn't been published to NET or SD. The format of data is dependant on the provided flag. If no group available it returns index.group = -1 unless forceIndex parameter exists and is a valid group index
		uint8_t setPublished(GroupIndex wichGroup, PubFlags wichFlag); // Set the group as published depending on the flag (accepted flags: NET, SD). Return the number of readings on published group.
		uint32_t countGroups(PubFlags wichFlag); // Returns the number of non published (for the specified flag) groups founded on flash.


		uint16_t recover(uint16_t wichSector, PubFlags wichFlag); 	// Recover all the readings on specified sector and outputs them to the specified media, returns de number of recovered groups
		SectorInfo sectorInfo(uint16_t wichSector);
		void dumpSector(uint16_t wichSector, uint16_t howMany=SECTOR_SIZE);
		FlashInfo flashInfo();

	private:


		enum SectorAddr {
			SECTOR_STATE = 0x00,
			SECTOR_NET   = 0x01,
			SECTOR_SD    = 0x02
		};

		SPIFlash flash = SPIFlash(pinCS_FLASH);
		SckBase* base;

		uint32_t _addr = 0; 				// The address where we write the contents of groups (readings) in flash.
		int16_t _currSector = -1;
		int16_t _dataAvailableSect[2] = {-1, -1};	// next sector with readings not published to network
		GroupIndex potencialNextGroup = {-1,-1,0}; 	// To store potencial next groups to be published in batch mode

		bool _append(char value);			// Appends a byte at the end of the list

		// Flash memory functions
		int8_t _flashStart();
		bool _flashFormat();

		// Sector functions
		uint32_t _getSectAddr(uint16_t wichSector);
		uint8_t _getSectState(uint16_t wichSector);
		int8_t _setSectPublished(uint16_t wichSector, PubFlags wichFlag); 		// Sector flags are only  marked if SectorState = SECTOR_FULL and all groups in the sector are already published in the correspondant flag
		int8_t _closeSector(uint16_t wichSector); 					// Marks sector as SECTOR_FULL and if there is no unpublished data in the sector also mark it as published, searchs for the next usable sector and updates _currSector to save next readings.
		int8_t _isSectPublished(uint16_t wichSector, PubFlags wichFlag);
		int16_t _getSectFreeSpace(uint16_t wichSector);
		bool _countSectGroups(uint16_t wichSector, SectorInfo* info); 			// Counts groups in sector (indepently of their state). To count more that one type of groups in the sector this is more efficient.
		int16_t _countSectGroups(uint16_t wichSector, PubFlags wichFlag, byte publishedState, bool getAll=false);  	// Count specified groups on sector. To count different type of groups this is not efficient, try the previous function.
		GroupIndex _getUnpubGrpIdx(uint16_t wichSector, PubFlags wichFlag); 		// Returns the index of the first non-published group on the sector and aflag specified
		void _scanSectors();

		// Group functions
		bool _getGrpAddr(GroupIndex* wichGroup);
		int8_t _setGrpPublished(GroupIndex wichGroup, PubFlags wichFlag);
		int8_t _isGrpPublished(GroupIndex wichGroup, PubFlags wichFlag);
		uint8_t _countReadings(GroupIndex wichGroup); 	// Returns the number of readings inside a group

		uint8_t _formatSD(GroupIndex wichGroup, char* buffer); 			// Return the number of readings found on formated group
		uint8_t _formatNET(GroupIndex wichGroup, char* buffer); 		// Return the number of readings found on formated group
};
