cd esp
platformio run
cd ../sam
platformio run --target upload
sleep 1
echo "** Asking SCK flash mode... **"
sudo -u vico echo "esp flash" >> /dev/ttyACM0
sleep 1
esptool.py --port /dev/ttyACM0 write_flash -fm dio --flash_size=detect 0x00000 ../esp/.pioenvs/esp12e/firmware.bin
echo "** Remember you have to reset your kit after uploading frimwares!!! **"