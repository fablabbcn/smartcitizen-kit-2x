#!/usr/bin/python

import serial, time, sys, glob, os, serial.tools.list_ports, subprocess, shutil
import uf2conv

class bcolors:
    OKBLUE = '\033[36m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def findSCK():
    devList = list(serial.tools.list_ports.comports())
    for dev in devList:
        try:
            if "Smartcitizen" in dev.product:
                return dev
        except:
            pass
    return False

def findSCKdrive():
    drives = uf2conv.getdrives()
    for drv in drives:
        try:
            if "SCK-20" in drv:
                return drv
        except:
            pass
    return False

def bootLoader():
    dev = findSCK()
    time.sleep(1)
    ser = serial.Serial(dev.device, 1200)
    ser.setDTR(False)
    time.sleep(2)

    while True:
        drv = findSCKdrive()
        if drv:
            return drv
    return false


def flashESP(dev):
    print(bcolors.WARNING + "Flashing ESP... "  + dev.serial_number[-4:] + bcolors.ENDC)
    flashedESP = 1
    retrys = 0
    while flashedESP != 0 and retrys < 3:
        drv = bootLoader()
        if drv: shutil.copyfile("tmp/bridge.uf2", drv + "/NEW.UF2")
        while not dev: dev = findSCK()
        time.sleep(2)
        flashedESP = subprocess.call([esptoolEXE, "-cp", dev.device, "-cb", "921600", "-ca", "0x000000", "-cf", "tmp/esp.bin"], stdout=FNULL, stderr=subprocess.STDOUT) 
        retrys = retrys + 1
    if flashedESP != 0: 
        ERROR()
        return False

    print(bcolors.WARNING + "Flashing ESP filesystem..."  + dev.serial_number[-4:] + bcolors.ENDC)
    flashedESPFS = 1
    retrys = 0
    while flashedESPFS != 0 and retrys < 3:
        drv = bootLoader()
        if drv:
            shutil.copyfile("tmp/bridge.uf2", drv + "/NEW.UF2")
        while not dev: dev = findSCK()
        time.sleep(2)
        flashedESPFS = subprocess.call([esptoolEXE, "-cp", dev.device, "-cb", "921600", "-ca", "0x300000", "-cf", "tmp/espfs.bin"], stdout=FNULL, stderr=subprocess.STDOUT)
        retrys = retrys + 1
    if flashedESPFS != 0: 
        ERROR()
        return False
    return True

def flashSAM(dev):
    print(bcolors.WARNING + "Flashing SAM..."  + dev.serial_number[-4:] + bcolors.ENDC)
    drv = bootLoader()
    if drv:
        shutil.copyfile("tmp/sam.uf2", drv + "/NEW.UF2")

   
def ERROR():
    print(bcolors.FAIL + "Something went wrong!!!" + bcolors.ENDC)
    

# Aqui actualizo el github a la ultima version
def updateGIT():
    print(bcolors.WARNING + "Updating git..." + bcolors.ENDC)
    os.chdir("..")
    gitUpdate = subprocess.call(["git", "pull"], stdout=FNULL, stderr=subprocess.STDOUT)
    if gitUpdate != 0:
        ERROR()
    os.chdir("tools")

# Aqui compilo los cuatro binarios
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


esptoolEXE = os.path.join(os.path.expanduser("~"), ".platformio/packages/tool-esptool/esptool");
FNULL = open(os.devnull, 'w')
updateGIT()
buildALL()
print("Monitoring for Smartcitizen kits connected via USB...")

kitList = []
while True:


    dev = findSCK()
    if dev:
        if dev.serial_number not in kitList:
            print("===========================================================")
            print("Found Smartcitizen on " + bcolors.OKBLUE + dev.device + bcolors.ENDC + "\r\nSerial number: " + bcolors.OKBLUE + dev.serial_number + bcolors.ENDC)
            kitList.append(dev.serial_number)
            start_time = time.time()
            if flashESP(dev):
                flashSAM(dev)

                print(bcolors.WARNING + "Verifying..."  + dev.serial_number[-4:] + bcolors.ENDC)
                while not dev: dev = findSCK()
                time.sleep(5)

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
                macReady = False
                ser.write("netinfo\r\n")
                while not macReady:
                    ll = ser.readline()
                    if "MAC address" in ll: macReady = True

                print(bcolors.OKGREEN + "Smartcitizen kit ready!!!")
                print("Serial number: " + dev.serial_number)
                print(ll + bcolors.ENDC)
                ser.write("shell -on\r\n")
                elapsed_time = time.time() - start_time
                print(time.strftime(bcolors.OKBLUE + "Finished in %H:%M:%S" + bcolors.ENDC, time.gmtime(elapsed_time)))
            # else:
            #     print("Smartcitizen " + bcolors.OKBLUE + kitList[0] + bcolors.ENDC + bcolors.WARNING + " removed" + bcolors.ENDC)
            #     print("===========================================================")
            #     kitList.pop(kitList.index(dev.serial_number))

    else:
        if len(kitList) > 0:
            print("Smartcitizen " + bcolors.OKBLUE + kitList[0] + bcolors.ENDC + bcolors.WARNING + " removed" + bcolors.ENDC)
            print("===========================================================")
        kitList = []

        

#   start
#   bootesp    
#   esp
#   bootfs
#   fs
#   sam
#   verify

    # portList = serialPorts()

    # for port in portList:
    #     if port not in flashingList:
    #         flashingList.append(port)
    #         flash(port)
