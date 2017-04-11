# Build and upload SAM code
cd sam
platformio run --target upload

# Big pause to let the kit boot even when it doesn't have sdcard (takes longer)
sleep 5

# Ask for flash mode (bridge Serial ports)
echo "** Asking for ESP flash mode... **"
echo "set mode esp flash" >> /dev/ttyACM0

# Build ESP code
cd ../esp
platformio run

# Upload ESP code
# esptool.py --port /dev/ttyACM0 write_flash --flash_size=detect 0x00000 esp/.pioenvs/esp12e/firmware.bin
/usr/bin/esptool -cp /dev/ttyACM0 -ca 0x00000 -cf esp/.pioenvs/esp12e/firmware.bin


echo "** Remember you have to reset your kit after uploading firmwares!!! **"
echo "Try clicking the button..."
