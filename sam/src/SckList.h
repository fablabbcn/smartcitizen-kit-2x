#pragma once

#include <Arduino.h>
#include <SPIFlash.h>

#include "Sensors.h"
#include "Pins.h"

// Number of bytes to be used in RAM, 512 bytes can store around 6 groups with the default urban board sensors enabled.
#define SCKLIST_RAM_SIZE 1024

// Number of bytes to be used on FLASH (Always keep this a littel bit smaller than real size)
#define SCKLIST_FLASH_SIZE 8388608
#define SECTOR_SIZE 4096
#define SCKLIST_RESERVED_SECTOR_COUNT 4 	// Number of sectors to be reserved (at the end of mnemory) for other uses
#define SCKLIST_SECTOR_NUM (SCKLIST_FLASH_SIZE/SECTOR_SIZE)-SCKLIST_RESERVED_SECTOR_COUNT

struct OneReading {
	SensorType type;
	String value;
};

enum SectorState {
	SECTOR_FREE 	= 0xFF, 	// This sector is not used (this is the state after erasing a sector)
	SECTOR_CURRENT 	= 0xFE, 	// This sector is the one being used for writing.
	SECTOR_FULL 	= 0xFC, 	// This sector is full of usable data.
	SECTOR_TRASH 	= 0xF8 		// This sector is full of non usable data (ready to be erased and reused)
};

struct FlashStatus {
	uint16_t freeSecCount = 0;
	uint16_t fullSecCount = 0;
	uint16_t trashSecCount = 0;
};

class SckList
{
	private:
		char ramBuff[SCKLIST_RAM_SIZE];

		uint32_t index_RAM = 0;
		uint32_t totalGroups_RAM = 0; 		// This counts the number of groups already completed (saved)
		uint32_t lastGroupRightIndex = 0;  	// The last position of a group, that means were group starts (reading from right to left)

		// Flash memory
		uint32_t index_flash = 1;
		uint32_t index_flash_groupAddr = SECTOR_SIZE;
		uint32_t totalGroups_flash = 0;
		int16_t currSector = -1;
		SPIFlash flash = SPIFlash(pinCS_FLASH);
		bool flashStarted = false;
		void flashSelect(); 			// Choose between sdcard or flash memory on SPI bus (this needs to be called before using flash)
		void migrateToFlash(); 			// When RAM buff is full we migrate all stored readings to flash until we are able to publish and empty the buffer.
		bool flashWrite(uint32_t wichIndex, char value);
		bool flashAppend(char value);
		char flashRead(uint32_t wichIndex);
		uint32_t getSectorAddress(uint16_t wichSector);
		SectorState getSectorState(uint16_t wichSector);
		uint16_t countSectorGroups(uint16_t wichSector);
		bool flashDelLastGroup();
		bool ramDelLastGroup();
		bool flashSaveLastGroup();

		union u_TimeStamp {
			char b[4];
			uint32_t i;
		} uTimeStamp;

		union u_GroupSize {
			char b[2];
			uint16_t i;
		} uGroupSize;

		void debugOut(const char *msg, bool error=false);
		bool append(char value); 						// Appends a byte at the end of the list
		bool write(uint32_t wichIndex, char value); 				// Writes a byte on a specific index of the list
		char read(uint32_t index); 						// Reads a byte of an specific index of the list
		uint32_t getGroupRightIndex(uint32_t wichGroup); 			// Returns the ending index of a specific group (zero if group don't exist)
		uint32_t getGroupLeftIndex(uint32_t wichGroup);				// Returns the first index of a specific group
		uint16_t readGroupSize(uint32_t rightGroupIndex); 			// Returns the group size in bytes, it requires the right index of the group
		bool lastGroupIsOpen(); 						// True if last created group is not yet saved

	public:

		enum GroupFlags {
			NET_PUBLISHED,
			SD_PUBLISHED
		};

		bool debug = false;
		void flashStart(); 							// Updates flash status (counts how many sectors are used, filled, trash, and the sector we are currently using)
		bool usingFlash = false; 						// If using flash only one group willo be stored in RAM and when it is closed moved to flash

		bool createGroup(uint32_t timeStamp); 					// Starts a new group, if there is an open gruop it will delete it before.
		bool saveLastGroup(); 							// Save the last group, to be called once all sensor readings are saved.
		bool delLastGroup(); 							// Will delete the last created group (saved or not)
		uint32_t countGroups(); 						// Will return the total of saved groups (RAM and Flash)
		uint32_t getTime(uint32_t wichGroup); 					// Return the timeStamp of the requested group (group index starts on the last saved group)
		uint16_t countReadings(uint32_t wichGroup);
		bool appendReading(SensorType wichsensor, String value);
		OneReading readReading(uint32_t wichGroup, uint8_t wichReading);
		void setFlag(uint32_t wichGroup, GroupFlags wichFlag, bool value);
		int8_t getFlag(uint32_t wichGroup, GroupFlags wichFlag); 		// Return flags or -1 on error

		FlashStatus fstatus;
		bool flashFormat(); 							// Format all flash memory and mark first sector as SECTOR_CURRENT
		uint32_t getFlashCapacity();
		bool testFlash();
};
