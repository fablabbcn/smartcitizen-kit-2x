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
    print(bcolors.WARNING + "Flashing ESP...   "  + bcolors.ENDC)
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
    return True

def flashESPFS(dev):
    print(bcolors.WARNING + "Flashing ESP filesystem...   "  + bcolors.ENDC)
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
    print(bcolors.WARNING + "Flashing SAM...   "  + bcolors.ENDC)
    drv = bootLoader(dev)
    if drv:
        shutil.copyfile("tmp/sam.uf2", drv + "/NEW.UF2")

def getSCKdata(ser, SCK):
    ser.write('shell -on\r\n')

    time.sleep(1)
    ser.write("netinfo\r\n")
    while True:
        ll = ser.readline()
        if "MAC address" in ll:
            if len(ll) > 15:
                SCK['mac'] = ll.strip()[-17:]
                break
            else: 
                ser.write("netinfo\r\n")

    time.sleep(1)
    ser.write('version\r\n')
    while True:
        line = ser.readline()
        if 'SAM version' in line:
            SCK['sam_ver'] = line.split(':')[1].strip()
        if 'ESP version' in line:
            SCK['esp_ver'] = line.split(':')[1].strip()
            break

    return SCK
    
def ERROR():
    print(bcolors.FAIL + "Something went wrong!!!" + bcolors.ENDC)
    print(bcolors.WARNING + "Please disconnect and reconnect your kit to retry..." + bcolors.ENDC)

def checkGITbranch():
    currentBranch = subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], stderr=subprocess.STDOUT)
    if not "master" in currentBranch:
        goOn = raw_input(bcolors.OKBLUE + "You are not on master branch\r\nAre you sure you want to continue? " + bcolors.ENDC + "[Y/n]")
    if "n" in goOn:
        print("Bye")
        sys.exit()

def updateGIT():
    print(bcolors.WARNING + "Updating git..." + bcolors.ENDC)
    os.chdir("..")
    gitUpdate = subprocess.call(["git", "pull"], stdout=FNULL, stderr=subprocess.STDOUT)
    if gitUpdate != 0:
        ERROR()
    os.chdir("tools")

def build(dsam=True, desp=True, despfs=True):
    tempdir = "tmp"
    if os.path.exists(tempdir):
        shutil.rmtree(tempdir)
    os.makedirs(tempdir)

    print(bcolors.WARNING + "Building binaries..." + bcolors.ENDC);

    if desp or despfs:
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
        os.chdir("..")

    if dsam:
        os.chdir("../sam")
        buildSAM = subprocess.call(["pio", "run"], stdout=FNULL, stderr=subprocess.STDOUT)
        if buildSAM == 0:
            ff = open(".pioenvs/sck2/firmware.bin", mode='rb')
            inpbuf = ff.read()
            outbuf = uf2conv.convertToUF2(inpbuf)
            with open("../tools/tmp/sam.uf2", "wb") as f:
                f.write(outbuf)
        else:
            ERROR()
        print(bcolors.OKBLUE + "SAM" + bcolors.WARNING + " binary ready" + bcolors.ENDC)
        os.chdir("../tools")

    if desp:
        os.chdir("../esp")
        buildESP = subprocess.call(["pio", "run"], stdout=FNULL, stderr=subprocess.STDOUT)
        if buildESP == 0:
            shutil.copyfile(".pioenvs/esp12e/firmware.bin", "../tools/tmp/esp.bin")
        else:
            ERROR()
        print(bcolors.OKBLUE + "ESP" + bcolors.WARNING + " binary ready" + bcolors.ENDC)
        os.chdir("../tools")

    if despfs:
        os.chdir("../esp")
        buildESPFS = subprocess.call(["pio", "run", "-t", "buildfs"], stdout=FNULL, stderr=subprocess.STDOUT)
        if buildESPFS == 0:
            shutil.copyfile(".pioenvs/esp12e/spiffs.bin", "../tools/tmp/espfs.bin")
        else:
            ERROR()
        print(bcolors.OKBLUE + "ESP" + bcolors.WARNING + " file system ready" + bcolors.ENDC)
        os.chdir("../tools")

def registerSCK(bearer, SCK): 
    headers = {'Authorization':'Bearer ' + bearer, 'Content-type': 'application/json',}
    device = {}
    device['name'] = SCK['platform_name']
    device['device_token'] = binascii.b2a_hex(os.urandom(3))
    SCK['token'] = device['device_token']
    device['description'] = SCK['description']
    device['kit_id'] = 20
    device['latitude'] = 41.396867
    device['longitude'] = 2.194351
    device['exposure'] = 'indoor'
    device['user_tags'] = 'Lab, iSCAPE, Research, Experimental'

    device_json = json.dumps(device)

    backed_device = requests.post('https://api.smartcitizen.me/v0/devices', data=device_json, headers=headers)
    
    SCK['id'] = str(backed_device.json()['id'])

    return SCK

def inventory(SCK):
    inv_path = "inventory.csv"

    if os.path.exists(inv_path): 
        shutil.copyfile(inv_path, inv_path+".BAK")
        csvFile = open("inventory.csv", "a")
    else:
        csvFile = open(inv_path, "w")
        # time,serial,mac,sam_ver,esp_ver,description,token,platform_name,platform_url
        csvFile.write("time,serial,mac,sam_ver,esp_ver,description,token,platform_name,platform_url\n")

    csvFile.write(time.strftime("%Y-%m-%dT%H:%M:%SZ,", time.gmtime()))
    csvFile.write(SCK['serial'] + ',' + SCK['mac'] + ',' + SCK['sam_ver'] + ',' + SCK['esp_ver'] + ',' + SCK['description'] + ',' + SCK['token'] + ',' + SCK['platform_name'] + ',' + SCK['platform_url'] + '\n')
    csvFile.close()



esptoolEXE = os.path.join(os.path.expanduser("~"), ".platformio/packages/tool-esptool/esptool");
FNULL = open(os.devnull, 'w')

# Check if we are on master branch
print("")
checkGITbranch()

# Update git?
if not "n" in raw_input(bcolors.OKBLUE + "Update to latest git?" + bcolors.ENDC + " [Y/n] "): updateGIT() 

# What do you want to build ?
dsam = True if not "n" in raw_input(bcolors.OKBLUE + "flash SAM?" + bcolors.ENDC + " [Y/n] ") else False
desp = True if not "n" in raw_input(bcolors.OKBLUE + "flash ESP?" + bcolors.ENDC + " [Y/n] ") else False
despfs = True if not "n" in raw_input(bcolors.OKBLUE + "flash ESP file system?" + bcolors.ENDC + " [Y/n] ") else False
if not dsam and not desp and not despfs:
    print("Bye")
    sys.exit()
build(dsam, desp, despfs)

def_Descrition = ""

# Add kit to inventory? default description?
dinventory = True if not "n" in raw_input(bcolors.OKBLUE + "Add kit to inventory?" + bcolors.ENDC + " [Y/n] ") else False
if dinventory:
    def_Descrition = raw_input(bcolors.OKBLUE + "Default description for this kit(s): " + bcolors.ENDC)

# Register kit on platform? default base for name? in case there is no bearer and wifi get them
dregister = True if not "n" in raw_input(bcolors.OKBLUE + "Register kit on platform?" + bcolors.ENDC + " [Y/n] ") else False
if dregister:
    def_baseName = raw_input(bcolors.OKBLUE + "Default name for the kit(s): " + bcolors.ENDC)
    if len(def_Descrition) == 0: def_Descrition = raw_input(bcolors.OKBLUE + "Default description for this kit(s): " + bcolors.ENDC)
    try:
        import secret
        print(bcolors.WARNING + "Founded secrets.py:" + bcolors.ENDC)
        print("bearer: " + bcolors.OKBLUE + secret.bearer + bcolors.ENDC)
        print("Wifi ssid: " + bcolors.OKBLUE + secret.wifi_ssid + bcolors.ENDC)
        print("Wifi pass: " + bcolors.OKBLUE + secret.wifi_pass + bcolors.ENDC)
        bearer = secret.bearer
        wifi_ssid = secret.wifi_ssid
        wifi_pass = secret.wifi_pass
    except:
        bearer = raw_input(bcolors.OKBLUE + "Platform bearer: " + bcolors.ENDC)
        wifi_ssid = raw_input(bcolors.OKBLUE + "WiFi ssid: " + bcolors.ENDC)
        wifi_pass = raw_input(bcolors.OKBLUE + "WiFi password: " + bcolors.ENDC)

# Start...
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

            SCK = {}
            SCK['serial'] = dev.serial_number
            SCK['description'] = def_Descrition
            
            if desp: flashESP(dev)
            if despfs: flashESPFS(dev)
            if dsam: flashSAM(dev)

            time.sleep(5)
            print(bcolors.WARNING + "Verifying...   "  + bcolors.ENDC)
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
            SCK = getSCKdata(ser, SCK)


            if dregister:
                SCK['platform_name'] = def_baseName +  SCK['mac'][-5:-3] + SCK['mac'][-2:]
                print(bcolors.WARNING + "Registering Kit on platform...   " + bcolors.ENDC)
                SCK = registerSCK(bearer, SCK)
        
                ser.write('config -mode net -wifi "' + wifi_ssid + '" "' + wifi_pass + '" -token ' + SCK['token'] + '\r\n')
                time.sleep(1)
                ser.write('shell -off\r\n')

                SCK['platform_url'] = "https://smartcitizen.me/kits/" + SCK['id']
                
            else:
                SCK['token'] = ""
                SCK['platform_name'] = ""
                SCK['platform_url'] = ""

            if dinventory:
                print(bcolors.WARNING + "Saving kit data on CSV...   " + bcolors.ENDC)
                inventory(SCK)

            print(bcolors.OKGREEN + "Smartcitizen kit ready!!!" + bcolors.ENDC)
            print("\r\nSerial number: " + bcolors.OKBLUE +  SCK['serial'] + bcolors.ENDC)
            print("Mac address: " + bcolors.OKBLUE + SCK['mac'] + bcolors.ENDC)
            print("Device token: " + bcolors.OKBLUE + SCK['token'] + bcolors.ENDC)
            print("Access Point name:" + bcolors.OKBLUE + " SmartCitizen" + SCK['mac'][-5:-3] + SCK['mac'][-2:] + bcolors.ENDC)
            print("Platform kit name: " + bcolors.OKBLUE + SCK['platform_name'] + bcolors.ENDC)
            print("Platform page:" + bcolors.OKBLUE + SCK['platform_url'] + bcolors.ENDC)

            elapsed_time = time.time() - start_time
            print("Finished in: " + bcolors.OKBLUE + time.strftime("%H:%M:%S" + bcolors.ENDC, time.gmtime(elapsed_time)))
    else:
        if len(kitList) > 0:
            print("Smartcitizen " + bcolors.OKBLUE + kitList[0] + bcolors.ENDC + bcolors.WARNING + " removed" + bcolors.ENDC)
            print("===========================================================")
        kitList = []

