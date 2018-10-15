#!/bin/bash

function PORT_WAIT {
	echo "waiting for port to appear..."
	while :
	do 
		echo $(ls /dev/ttyACM0);
	done
}


# build sam, esp and espfs
pio run
cd ../../esp
pio run
pio run -t buildfs

cd ../tools/fastBridge
../touch_port1200.py
PORT_WAIT

sleep 1
../uf2conv.py -d SCK-2.0 .pioenvs/sck2/firmware.bin

~/.platformio/packages/tool-esptool/esptool -cp /dev/ttyACM0 -cb 921600 -ca 0x00000 -cf ../../esp/.pioenvs/esp12e/firmware.bin -v

# ../touch_port1200.py
# sleep 1
# ../uf2conv.py -d SCK-2.0 .pioenvs/sck2/firmware.bin
#  ~/.platformio/packages/tool-esptool/esptool -cp /dev/ttyACM0 -cb 921600 -ca 0x30000 -cf ../../esp/.pioenvs/esp12e/spiffs.bin -v
