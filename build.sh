# Build and upload SAM code
echo " *** Uploading SAM firmware *** "
cd sam
pio run -t upload

# Build and upload ESP code
cd ../esp
echo " *** Uploading ESP firmware *** "
pio run -t upload
echo " *** Uploading ESP firmware *** "
pio run -t uploadfs