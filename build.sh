#!/bin/bash

GREEN='\033[1;32m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m' # No Color

function SAM {
	echo
	echo -e "Remember to ${YELLOW}double click the reset button ${NC}of your kit!!!"
	echo "Press any key to continue..." 
	echo
	read -n 1 -s -r 

	cd sam
	pio run
	cd ..
	tools/uf2conv.py -o SAM_firmware.uf2 -d SCK-2.0 sam/.pioenvs/sck2/firmware.bin
}

function ESP {
	cd esp
	pio run -t upload
	cd ..
}

function ESPFS {
	cd esp
	pio run -t uploadfs
	cd ..
}

function BOOT {
	echo
	echo -e "Remeber to ${YELLOW}connect SAM-ICE programer${NC} to your kit!"
	echo "Press any key to continue"
	read -n 1 -s -r

	cd bootloader/uf2-samdx1
	make
	openocd -f openocd_sck2_SAMICE.cfg
	cd ../..
}

case $1 in

	sam)
		echo -e "${NC}Flashing SAM${NC}"
		SAM
		;;
	esp)
		echo -e "${NC}Flashing ESP${NC}"
		ESP
		;;
	espfs)
		echo -e "${NC}Flashing ESP filesystem${NC}"
		ESPFS
		;;
	boot)
		echo -e "${NC}Flashing SAM bootloader${NC}"
		BOOT
		;;
	all)
		echo -e "${NC}Flashing everything${NC}"
		SAM
		sleep 3
		ESP
		sleep 3
		ESPFS
		;;
	*)
		echo "Options:"
		echo "[sam] Flashes the SAM firmware only."
		echo "[esp] Flashes the ESP firmware only."
		echo "[espfs] Flashes the ESP filesystem only."
		echo "[all] Flashes all of the above (not the bootlader)."
		echo "[boot] Flashes SAM bootloader" 
		;;
esac

