# Build and upload SAM code
cd sam
pio run -t upload

# Build and upload ESP code
cd ../esp
pio run -t upload
# pio run -t uploadfs

echo "** Remember you have to reset your kit after uploading firmwares!!! **"
echo "Try clicking the button..."
