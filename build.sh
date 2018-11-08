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
	read -n 1 -s -r key

	if [ "$key" = "u" ]
	then
		./tools/touch_port1200.py
	fi

	cd sam
	pio run
	cd ..
	tools/uf2conv.py -o bin/SAM_firmware.uf2 -d SCK-2.0 sam/.pioenvs/sck2/firmware.bin
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

	update)
		echo -e "${NC}Updating  binaries${NC}"
		cd sam && pio run && cd .. && cp sam/.pioenvs/sck2/firmware.bin bin/SAM_firmware.bin && tools/uf2conv.py -o bin/SAM_firmware.uf2 bin/SAM_firmware.bin 
		cd esp && pio run && cp .pioenvs/esp12e/firmware.bin ../bin/ESP_firmware.bin && pio run -t buildfs && cp .pioenvs/esp12e/spiffs.bin ../bin/ESP_filesystem.bin && cd ..
		cd bootloader/uf2-samdx1 && make && cd ../.. && cp bootloader/uf2-samdx1/build/sck2.0/bootloader-sck2.0.bin bin/SAM_bootloader.bin
		;;
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
		echo "[update] Build all binary files and saves them to bin folder."
		echo "[sam] Flashes the SAM firmware only."
		echo "[esp] Flashes the ESP firmware only."
		echo "[espfs] Flashes the ESP filesystem only."
		echo "[all] Flashes all of the above (not the bootlader)."
		echo "[boot] Flashes SAM bootloader" 
		;;
esac

