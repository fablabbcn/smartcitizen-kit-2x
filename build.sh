#!/bin/bash

# TODO
# Multiplatform support
# Check for errors
# Check if openocd "bootloader" command generate problems in the fuses (https://github.com/Microsoft/uf2-samdx1#fuses)

function SAM {
	echo
	echo "Remember to double click the reset button of your kit!!!"
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
	echo "Remeber to connect SAM-ICE programer to your kit!"
	echo "Press any key to continue"
	read -n 1 -s -r

	cd bootloader/uf2-samdx1
	make
	openocd -f openocd_sck2_SAMICE.cfg
	cd ../..
}

case $1 in

	sam)
		echo "Flashing SAM"
		SAM
		;;
	esp)
		echo "Flashing ESP"
		ESP
		;;
	espfs)
		echo "Flashing ESP filesystem"
		ESPFS
		;;
	boot)
		echo "Flashing SAM bootloader"
		BOOT
		;;
	all)
		echo "Flashing everything"
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

