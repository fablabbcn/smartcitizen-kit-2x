#!/usr/bin/python

import requests, json, binascii
import serial, time, sys, glob, os, serial.tools.list_ports, subprocess, shutil
import uf2conv

def timeout(started):
    if started + 30 < time.time():
        return True
    else: 
        return False

class bcolors:
    OKBLUE = '\033[36m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def findSCK():
    dev = False
    devList = list(serial.tools.list_ports.comports())
    for tested in devList:
        try:
            if "Smartcitizen" in tested.product:
                return tested
        except:
            pass
    return False

def getSerial(dev, speed):
    started = time.time()
    ser = False
    while not ser and not timeout(started):
        try:
            ser = serial.Serial(dev.device, speed)
        except:
            pass

    if ser: return ser
    else: return False

def findSCKdrive():
    drv = False
    started = time.time()
    while not drv and not timeout(started):
        drives = uf2conv.getdrives()
        for drv in drives:
            try:
                if "CURRENT.UF2" in os.listdir(drv):
                    return drv
            except:
                pass
    return False

def bootLoader(dev):
    ser = getSerial(dev, 1200)
    ser.setDTR(False)
    drv = findSCKdrive()
    if drv: return drv
    else: return False

def flashESP(dev):
    print(bcolors.WARNING + "Flashing ESP...   "  + dev.serial_number[-4:] + bcolors.ENDC)
    flashedESP = 1
    retrys = 0
    while flashedESP != 0 and retrys < 3:
        drv = bootLoader(dev)
        if drv: shutil.copyfile("tmp/bridge.uf2", drv + "/NEW.UF2")
        else: ERROR()
        while not dev: dev = findSCK()
        time.sleep(2)
        flashedESP = subprocess.call([esptoolEXE, "-cp", dev.device, "-cb", "921600", "-ca", "0x000000", "-cf", "tmp/esp.bin"], stdout=FNULL, stderr=subprocess.STDOUT) 
        retrys = retrys + 1
    if flashedESP != 0: 
        ERROR()
        return False

    print(bcolors.WARNING + "Flashing ESP filesystem...   "  + dev.serial_number[-4:] + bcolors.ENDC)
    flashedESPFS = 1
    retrys = 0
    while flashedESPFS != 0 and retrys < 3:
        drv = bootLoader(dev)
        if drv: shutil.copyfile("tmp/bridge.uf2", drv + "/NEW.UF2")
        else: ERROR()
        while not dev: dev = findSCK()
        time.sleep(2)
        flashedESPFS = subprocess.call([esptoolEXE, "-cp", dev.device, "-cb", "921600", "-ca", "0x300000", "-cf", "tmp/espfs.bin"], stdout=FNULL, stderr=subprocess.STDOUT)
        retrys = retrys + 1
    if flashedESPFS != 0: 
        ERROR()
        return False
    return True

def flashSAM(dev):
    print(bcolors.WARNING + "Flashing SAM...   "  + dev.serial_number[-4:] + bcolors.ENDC)
    drv = bootLoader(dev)
    if drv:
        shutil.copyfile("tmp/sam.uf2", drv + "/NEW.UF2")

def getMAC(ser):
    ser.write("netinfo\r\n")
    while True:
        ll = ser.readline()
        if "MAC address" in ll:
            if len(ll) > 15:
                return ll.strip()[-17:]
            else: 
                ser.write("netinfo\r\n")

def ERROR():
    print(bcolors.FAIL + "Something went wrong!!!" + bcolors.ENDC)
    
def updateGIT():
    print(bcolors.WARNING + "Updating git..." + bcolors.ENDC)
    os.chdir("..")
    gitUpdate = subprocess.call(["git", "pull"], stdout=FNULL, stderr=subprocess.STDOUT)
    if gitUpdate != 0:
        ERROR()
    os.chdir("tools")

def buildALL():
    tempdir = "tmp"
    if os.path.exists(tempdir):
        shutil.rmtree(tempdir)
    os.makedirs(tempdir)

    print(bcolors.WARNING + "Building binaries..." + bcolors.ENDC);

    os.chdir("fastBridge")
    buildSAM = subprocess.call(["pio", "run"], stdout=FNULL, stderr=subprocess.STDOUT)
    if buildSAM == 0:
        ff = open(".pioenvs/sck2/firmware.bin", mode='rb')
        inpbuf = ff.read()
        outbuf = uf2conv.convertToUF2(inpbuf)
        with open("../tmp/bridge.uf2", "wb") as f:
            f.write(outbuf)
    else:
        ERROR()
    print("Bridge binary ready")

    os.chdir("../../sam")
    buildSAM = subprocess.call(["pio", "run"], stdout=FNULL, stderr=subprocess.STDOUT)
    if buildSAM == 0:
        ff = open(".pioenvs/sck2/firmware.bin", mode='rb')
        inpbuf = ff.read()
        outbuf = uf2conv.convertToUF2(inpbuf)
        with open("../tools/tmp/sam.uf2", "wb") as f:
            f.write(outbuf)
    else:
        ERROR()
    print("SAM binary ready")

    os.chdir("../esp")
    buildESP = subprocess.call(["pio", "run"], stdout=FNULL, stderr=subprocess.STDOUT)
    if buildESP == 0:
        shutil.copyfile(".pioenvs/esp12e/firmware.bin", "../tools/tmp/esp.bin")
    else:
        ERROR()
    print("ESP binary ready")

    buildESPFS = subprocess.call(["pio", "run", "-t", "buildfs"], stdout=FNULL, stderr=subprocess.STDOUT)
    if buildESPFS == 0:
        shutil.copyfile(".pioenvs/esp12e/spiffs.bin", "../tools/tmp/espfs.bin")
    else:
        ERROR()
    print("ESP filesystem ready")

    os.chdir("../tools")

def registerSCK(bearer, name, description=None, kit_id=None, latitude=None, longitude=None, exposure=None, user_tags=None):
    headers = {'Authorization':'Bearer ' + bearer, 'Content-type': 'application/json',}
    device = {}
    device['name'] = name
    device['device_token'] = binascii.b2a_hex(os.urandom(3))
    if description is None: device['description'] = 'Iscape Citizen Kit test'
    if kit_id is None: device['kit_id'] = 20
    if latitude is None: device['latitude'] = 41.396867
    if longitude is None: device['longitude'] = 2.194351
    if exposure is None: device['exposure'] = 'indoor'
    if user_tags is  None: device['user_tags'] = 'Lab, iSCAPE, Research, Experimental'

    device_json = json.dumps(device)

    backed_device = requests.post('https://api.smartcitizen.me/v0/devices', data=device_json, headers=headers)
    
    device['id'] = str(backed_device.json()['id'])

    return device

def csvAppend(SCK):
    # serial_number,MAC,token,name,url
    csvFile = open("kits.csv", "a")
    csvFile.write(SCK['serial'] + ',' + SCK['mac'] + ',' + SCK['device_token'] + ',' + SCK['name'] + ',' + 'https://smartcitizen.me/kits/' + SCK['id'] + '\r\n')
    csvFile.close()


bearer = 'YOUR_TOKEN_HERE'
wifi = 'WIFI_NETWORK_SSID'
password = 'WIFI_PASSWORD'

esptoolEXE = os.path.join(os.path.expanduser("~"), ".platformio/packages/tool-esptool/esptool");
FNULL = open(os.devnull, 'w')

    

if not "-nobuild" in sys.argv:
    updateGIT()
    buildALL()
else:
    print("Skipping building binaries")


print("Monitoring for Smartcitizen kits connected via USB...")
kitList = []
while True:
    dev = findSCK()
    if dev:
        if dev.serial_number not in kitList:
            print("===========================================================")
            print("Found Smartcitizen on " + bcolors.OKBLUE + dev.device + bcolors.ENDC)
            kitList.append(dev.serial_number)
            start_time = time.time()
            if flashESP(dev):
                flashSAM(dev)

                time.sleep(5)
                print(bcolors.WARNING + "Verifying...   "  + dev.serial_number[-4:] + bcolors.ENDC)
                while not dev: dev = findSCK()

                ser = False
                while not ser:
                    try:
                        ser = serial.Serial(dev.device, 115200)
                    except: pass

                savedConf = False
                while not savedConf:
                    ll = ser.readline()
                    if "Saved configuration on ESP" in ll: savedConf = True

                time.sleep(2)
                MAC = getMAC(ser)
                name = 'iSCAPE Citizen Kit #' + MAC[-5:-3] + MAC[-2:]

                print(bcolors.WARNING + "Registering Kit on platform...   "  + dev.serial_number[-4:] + bcolors.ENDC)
                SCK = registerSCK(bearer, name)
        
                ser.write('config -mode net -wifi "' + wifi + '" "' + password + '" -token ' + SCK['device_token'] + '\r\n')
                
                SCK['serial'] = dev.serial_number
                SCK['mac'] = MAC

                print(bcolors.WARNING + "Saving kit data on CSV...   "  + dev.serial_number[-4:] + bcolors.ENDC)
                csvAppend(SCK)

                print(bcolors.OKGREEN + "Smartcitizen kit ready!!!" + bcolors.ENDC)
                print("\r\nSerial number: " + bcolors.OKBLUE +  SCK['serial'] + bcolors.ENDC)
                print("Mac address: " + bcolors.OKBLUE + SCK['mac'] + bcolors.ENDC)
                print("Device token: " + bcolors.OKBLUE + SCK['device_token'] + bcolors.ENDC)
                print("Access Point name:" + bcolors.OKBLUE + " SmartCitizen" + SCK['mac'][-5:-3] + SCK['mac'][-2:] + bcolors.ENDC)
                print("Platform kit name: " + bcolors.OKBLUE + SCK['name'] + bcolors.ENDC)
                print("Platform page:" + bcolors.OKBLUE + " https://smartcitizen.me/kits/" + SCK['id'] + bcolors.ENDC)

                elapsed_time = time.time() - start_time
                print("Finished in: " + bcolors.OKBLUE + time.strftime("%H:%M:%S" + bcolors.ENDC, time.gmtime(elapsed_time)))
    else:
        if len(kitList) > 0:
            print("Smartcitizen " + bcolors.OKBLUE + kitList[0] + bcolors.ENDC + bcolors.WARNING + " removed" + bcolors.ENDC)
            print("===========================================================")
        kitList = []

        
# TODO
# put name and bearer in a external variables file
# Only build binaries if git pull got something new
# argument for flashing only sam, esp or espfs
# multiple simultaneos kits support
# BUG sometimes it hangs on flashing esp or espfs
