#pragma once

#include <Arduino.h>
#include <SPIFlash.h>

#include "Sensors.h"
#include "Pins.h"

// Number of bytes to be used in RAM, 512 bytes can store around 6 groups with the default urban board sensors enabled.
#define SCKLIST_RAM_SIZE 1024

// Number of bytes to be used on FLASH (Always keep this a littel bit smaller than real size)
#define SCKLIST_FLASH_SIZE 4190000 // 4194304 is the real size (last 4304 bytes reserved for other uses)
// it seems 2.1 has a 8mb flash!

struct OneReading {
	SensorType type;
	String value;
};

class SckList
{
	private:
		char ramBuff[SCKLIST_RAM_SIZE];

		uint32_t index = 0;
		uint32_t totalGroups = 0;
		uint32_t lastGroupRightIndex = 0;  	// The last position of a group that means were group starts (reading from right to left)

		// Flash memory
		SPIFlash flash = SPIFlash(pinCS_FLASH);
		bool flashStarted = false;
		bool flashSleeping = false;
		void flashSelect(); 			// Choose between sdcard or flash memory on SPI bus (this needs to be called before using flash)
		void migrateToFlash(); 			// When RAM buff is full we migrate all stored readings to flash until we are able to publish and empty the buffer.


		union u_TimeStamp {
			char b[4];
			uint32_t i;
		} uTimeStamp;

		union u_GroupSize {
			char b[2];
			uint16_t i;
		} uGroupSize;

		void debugOut(const char *msg);
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

		// TODO change this to false for production
		bool debug = true;
		void flashStart();
		
		bool usingFlash = false;

		bool createGroup(uint32_t timeStamp); 					// Starts a new group, if there is an open gruop it will delete it before.
		bool saveLastGroup(); 							// Save the last group, to be called once all sensor readings are saved
		bool delLastGroup(); 							// Will delete the last created group (saved or not)
		uint32_t countGroups(); 						// Will return the total of saved groups
		uint32_t getTime(uint32_t wichGroup); 					// Return the timeStamp of the requested group (group index starts on the last saved group)
		uint16_t countReadings(uint32_t wichGroup);
		bool appendReading(SensorType wichsensor, String value);
		OneReading readReading(uint32_t wichGroup, uint8_t wichReading);
		void setFlag(uint32_t wichGroup, GroupFlags wichFlag, bool value);
		bool getFlag(uint32_t wichGroup, GroupFlags wichFlag);
};
