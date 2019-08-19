#pragma once

#include <Arduino.h>
#include <SPIFlash.h> 	// https://github.com/Marzogh/SPIMemory

#include "Sensors.h"
#include "Pins.h"

/*
## Flash use
We use [SPI Memory arduino library](https://github.com/Marzogh/SPIMemory)
The minimum unit we can erase is a flash sector (4kb).

The first byte of each sector will have a byte that describes the sector state:

**SECTOR_FREE -> 0xFF (0b11111111)** -> This sector is not used (this is the state after erasing)
**SECTOR_CURRENT -> 0xFE (0b11111110)** -> This sector is **the one** being used for writing.
**SECTOR_FULL -> 0xFC (0b11111100)** -> This sector is full of usable data.
**SECTOR_TRASH -> 0xF8 (0b11111000)** -> This sector is full of non usable data (ready to be erased and reused)

Every time we start the kit we scan sector by sector and count the status of each sector, if we have saved readings we set **UsingFlash** flag.

Starting form the last byte of the sector every time we save a group we write the address of that group (each address is composed of 4 bytes). So the last 4 bytes of each sector are the address of the first group saved in that sector, the previous 4 bytes the address of the second group and so on. _Remeber we use as group index the address were the group ends_.

| **FE** 12 34 54 A2 FF FF FF FF ...... FF FF FF FF **CC CC CC CC BB BB BB BB AA AA AA AA** |

So when we find the current used sector (**FE** in the first byte of the sector) we start reading from the last byte of this sector address by address until we found an invalid address (FF FF FF FF) that way we know the last saved group index and we can start reading/writing

**Writing groups on flash**
We prepare the group to be written on RAM buffer and once the group is complete we save it on flash. If it doesnt fit in the sector we are using (0xFE) we close that sector (0xFC) and start a new one writing (0xFE to the next available sector).

**Reading groups from flash**
We scan the flash for the sector in use (0xFE) and get the available groups that aren't yet published, then we check the previous sector and if it is full of usable data (0xFD) we proceed to read all the groups in that sector. Once all the groups from a sector are succsesfully published to the network in NET mode or to SDcard in SD mode we mark that sector as used (0xF8).

When all the flash is used we will start erasing the first sector and reusing it.

Every time we flash the firmware of the SAM the flash memory should be formatted and started writing a **0xFF** to the first sector. This should be executed by the flashing script (build.py)???
*/

// Number of bytes to be used in RAM, 512 bytes can store around 6 groups with the default urban board sensors enabled.
#define SCKLIST_RAM_SIZE 1024

// Number of bytes to be used on FLASH (Always keep this a littel bit smaller than real size)
#define SCKLIST_FLASH_SIZE 8388608
#define SECTOR_SIZE 4096
#define SCKLIST_RESERVED_SECTOR_COUNT 4 	// Number of sectors to be reserved (at the end of memory) for other uses
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
		//---------- SHARED
		union u_TimeStamp {
			char b[4];
			uint32_t i;
		} uTimeStamp;

		union u_GroupSize {
			char b[2];
			uint16_t i;
		} uGroupSize;

		//---------- RAM
		char ramBuff[SCKLIST_RAM_SIZE];
		uint32_t idx_r = 0;
		uint32_t totGrp_r = 0; 			// This counts the number of groups already completed (saved)
		uint32_t lastGrpRightIdx_r = 0;  	// The last position of a group, that means were group starts (reading from right to left)

		bool write_r(uint32_t wichIndex, char value); 				// Writes a byte on a specific index of the list
		bool append_r(char value); 						// Appends a byte at the end of the list
		char read_r(uint32_t index); 						// Reads a byte of an specific index of the list

		bool lastGrpIsOpen_r(); 						// True if last created group is not yet saved
		bool delLastGrp_r();


		//---------- FLASH
		bool usingFlash = false; 						// If using flash only one group willo be stored in RAM and when it is closed moved to flash
		uint32_t idx_f = 0; 				// The address where we write the contents of groups (readings) in flash.
		uint32_t index_flash_groupAddr = SECTOR_SIZE; 	// The address where we write the address of each group we save
		uint32_t totGrp_f = 0;
		int16_t currSector = -1;
		uint32_t lastGrpRightIdx_f = 0; 	// The last position of a Flash group, that means were group starts (reading from right to left)
		SPIFlash flash = SPIFlash(pinCS_FLASH);
		bool flashStarted = false;
		bool write_f(uint32_t wichIndex, char value);
		bool append_f(char value);
		char read_f(uint32_t wichIndex);
		bool saveLastGrp_f();
		bool delLastGrp_f();

		// Sector utilities
		uint32_t getSectAddr(uint16_t wichSector);
		uint16_t getSectFree(uint16_t wichSector); 			// Returns the number of bytes free on sector
		SectorState getSectState(uint16_t wichSector);
		uint16_t getSectGrpNum(uint16_t wichSector, bool validOnly); 	// Returns the number of groups in a sector

		// General flash utilities
		bool testFlash();
		void flashSelect(); 			// Choose between sdcard or flash memory on SPI bus (this needs to be called before using flash)
		void migrateToFlash(); 			// When RAM buff is full we migrate all stored readings to flash until we are able to publish and empty the buffer.


		// SHARED
		uint16_t getGrpSize(uint32_t rightGroupIndex, bool force_ram=false); 	// Returns the group size in bytes, it requires the right index of the group
		uint32_t getGrpLeftIdx(uint32_t wichGroup);				// Returns the first index of a specific group
		uint32_t getGrpRightIdx(uint32_t wichGroup); 				// Returns the ending index of a specific group (zero if group don't exist)

		void debugOut(const char *msg, bool error=false);

	public:
		// SHARED
		enum GroupFlags {
			NET_PUBLISHED,
			SD_PUBLISHED,
			FLASH_DELETED
		};

		bool debug = false;

		FlashStatus fstatus;

		// General flash utilities
		void flashStart(); 							// Updates flash status (counts how many sectors are used, filled, trash, and the sector we are currently using)
		bool flashFormat(); 							// Format all flash memory and mark first sector as SECTOR_CURRENT
		uint32_t getFlashCapacity();

		bool appendReading(SensorType wichsensor, String value);
		bool createGroup(uint32_t timeStamp); 					// Starts a new group (new groups are always created on RAM), if there is an open gruop it will save it before.
		bool saveLastGroup(); 							// Save the last group, to be called once all sensor readings are saved.
		bool delLastGroup(); 							// Will delete the last created group (saved or not)
		uint32_t getGroupTime(uint32_t wichGroup);				// Return the timeStamp of the requested group (group index starts on the last saved group)
		uint32_t countGroups(); 						// Will return the total of saved groups (RAM and Flash)
		uint16_t countReadings(uint32_t wichGroup);
		OneReading readReading(uint32_t wichGroup, uint8_t wichReading);
		int8_t getFlag(uint32_t wichGroup, GroupFlags wichFlag); 		// Return flags or -1 on error
		void setFlag(uint32_t wichGroup, GroupFlags wichFlag, bool value);
};
