#!/usr/bin/python

import serial
import serial.tools.list_ports
import os
import time
import subprocess
import uf2conv
import shutil
import sys
import binascii
import json
import requests

'''
Smartcitizen Kit python library.
This library is meant to be run inside the firmware repository folder.
'''

class sck:

    # paths
    paths = {}
    paths['base'] = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip()
    paths['binFolder'] = os.path.join(str(paths['base']), 'bin')
    paths['esptoolPy'] = os.path.join(str(paths['base']), 'tools', 'esptool.py')
    if not os.path.exists(paths['binFolder']):
        os.makedirs(paths['binFolder'])
    os.chdir('esp')
    paths['pioHome'] = [s.split()[1].strip(',').strip("'") for s in subprocess.check_output(['pio', 'run', '-t', 'envdump']).split('\n') if "'PIOHOME_DIR'" in s][0]
    os.chdir(paths['base'])
    paths['esptool'] = os.path.join(str(paths['pioHome']), 'packages', 'tool-esptool', 'esptool')

    # filenames
    files = {}
    files['samBin'] = 'SAM_firmware.bin'
    files['samUf2'] = 'SAM_firmware.uf2'
    files['espBin'] = 'ESP_firmware.bin'

    # Serial port
    serialPort = None
    serialPort_name = None

    # chips and firmware info
    infoReady = False
    sam_serialNum = ''
    sam_firmVer = ''
    sam_firmCommit = ''
    sam_firmBuildDate = ''
    esp_macAddress = ''
    esp_firmVer = ''
    esp_firmCommit = ''
    esp_firmBuildDate = ''

    # WiFi and platform info
    mode = ''
    token = ''
    wifi_ssid = ''
    wifi_pass = ''

    verbose = 2     # 0 -> never print anything, 1 -> print only errors, 2 -> print everything

    def begin(self):
        devList = list(serial.tools.list_ports.comports())
        i = 0
        kit_list = []
        for d in devList:
            try:
                if 'Smartcitizen' in d.description:
                    i+=1
                    print('['+str(i)+'] Smartcitizen Kit S/N: ' + d.serial_number)
                    kit_list.append(d)
            except:
                pass
        if i == 0: self.err_out('No SKC found!!!'); return False
        elif i > 0:
            if i == 1:
                wich_kit = 0
            else:
                wich_kit = int(raw_input('Multiple Kits found, please select one: ')) - 1

        self.sam_serialNum = kit_list[wich_kit].serial_number
        self.serialPort_name = kit_list[wich_kit].device

    def updateSerial(self, speed=115200):
        timeout = time.time() + 15
        while True:
            devList = list(serial.tools.list_ports.comports())
            found = False
            for d in devList:
                try:
                    if self.sam_serialNum in d.serial_number:
                        self.serialPort_name = d.device
                        found = True
                    if time.time() > timeout:
                        self.err_out('Timeout waiting for device')
                        sys.exit()
                except:
                    pass
            if found: break

        timeout = time.time() + 15
        while True:
            try:
                time.sleep(0.1)
                self.serialPort = serial.Serial(self.serialPort_name, speed)
            except:
                if time.time() > timeout:
                    self.err_out('Timeout waiting for serial port')
                    sys.exit()
            time.sleep(0.1)
            if self.serialPort.is_open: return

    def checkConsole(self):
        timeout = time.time() + 15
        while True:
            self.serialPort.write('\r\n')
            time.sleep(0.1)
            buff = self.serialPort.read(self.serialPort.in_waiting)
            if 'SCK' in buff: return True
            if time.time() > timeout:
                self.err_out('Timeout waiting for kit console response')
                return False
            time.sleep(0.5)

    def getInfo(self):
        if self.infoReady: return
        self.updateSerial()
        self.serialPort.write('\r\nversion\r\n')
        time.sleep(0.5)
        m = self.serialPort.read(self.serialPort.in_waiting).split()
        self.esp_macAddress = m[m.index('address:')+1]
        m.remove('SAM')
        self.sam_firmVer = m[m.index('SAM')+2]
        m.remove('ESP')
        self.esp_firmVer = m[m.index('ESP')+2]
        self.infoReady = True

    def getConfig(self):
        self.updateSerial()
        self.checkConsole()
        self.serialPort.write('\r\nconfig\r\n')
        time.sleep(0.5)
        m = self.serialPort.read(self.serialPort.in_waiting).split('\r\n')
        for line in m:
            if 'Mode' in line:
                mm = line.split('Mode: ')[1].strip()
                if mm != 'not configured': self.mode = mm
            if 'Token:' in line:
                tt = line.split(':')[1].strip()
                if tt != 'not configured' and len(tt) == 6: self.token = tt
            if 'credentials:' in line:
                ww = line.split('credentials: ')[1].strip()
                if ww.count(' - ') == 1:
                    self.wifi_ssid, self.wifi_pass = ww.split(' - ')
                    if self.wifi_pass == 'null': self.wifi_pass = ""

    def setBootLoaderMode(self):
        self.updateSerial()
        self.serialPort.close()
        self.serialPort = serial.Serial(self.serialPort_name, 1200)
        self.serialPort.setDTR(False)
        time.sleep(5)
        mps = uf2conv.getdrives()
        for p in mps:
            if 'INFO_UF2.TXT' in os.listdir(p):
                return p
        self.err_out('Cant find the mount point fo the SCK')
        return False

    def buildSAM(self, out=sys.__stdout__):
        os.chdir(self.paths['base'])
        os.chdir('sam')
        piorun = subprocess.call(['pio', 'run'], stdout=out, stderr=subprocess.STDOUT)
        if piorun == 0:
            try:
                shutil.copyfile(os.path.join(os.getcwd(), '.pioenvs', 'sck2', 'firmware.bin'), os.path.join(self.paths['binFolder'], self.files['samBin']))
            except:
                self.err_out('Failed building SAM firmware')
                return False
        else:
            return False
        with open(os.path.join(self.paths['binFolder'], self.files['samBin']), mode='rb') as myfile:
            inpbuf = myfile.read()
        outbuf = uf2conv.convertToUF2(inpbuf)
        uf2conv.writeFile(os.path.join(self.paths['binFolder'], self.files['samUf2']), outbuf)
        os.chdir(self.paths['base'])
        return True

    def flashSAM(self, out=sys.__stdout__):
        os.chdir(self.paths['base'])
        mountpoint = self.setBootLoaderMode()
        try:
            shutil.copyfile(os.path.join(self.paths['binFolder'], self.files['samUf2']), os.path.join(mountpoint, self.files['samUf2']))
        except:
            self.err_out('Failed transferring firmware to SAM')
            return False
        time.sleep(2)
        return True

    def getBridge(self, speed=921600):
        timeout = time.time() + 15
        while True:
            self.updateSerial(speed)
            self.serialPort.write('\r\n')
            time.sleep(0.1)
            buff = self.serialPort.read(self.serialPort.in_waiting)
            if 'SCK' in buff: break
            if time.time() > timeout:
                self.err_out('Timeout waiting for SAM bridge')
                return False
            time.sleep(2.5)
        buff = self.serialPort.read(self.serialPort.in_waiting)
        self.serialPort.write('esp -flash ' + str(speed) + '\r\n')
        time.sleep(0.2)
        buff = self.serialPort.read(self.serialPort.in_waiting)
        return True

    def buildESP(self, out=sys.__stdout__):
        os.chdir(self.paths['base'])
        os.chdir('esp')
        piorun = subprocess.call(['pio', 'run'], stdout=out, stderr=subprocess.STDOUT)
        if piorun == 0:
            shutil.copyfile(os.path.join(os.getcwd() , '.pioenvs', 'esp12e', 'firmware.bin'), os.path.join(self.paths['binFolder'], self.files['espBin']))
            return True
        self.err_out('Failed building ESP firmware')
        return False

    def flashESP(self, speed=921600, out=sys.__stdout__):
        os.chdir(self.paths['base'])
        if not self.getBridge(speed): return False
        flashedESP = subprocess.call([self.paths['esptool'], '-cp', self.serialPort_name, '-cb', str(speed), '-ca', '0x000000', '-cf', os.path.join(self.paths['binFolder'], self.files['espBin'])], stdout=out, stderr=subprocess.STDOUT)
        if flashedESP == 0:
            time.sleep(1)
            return True
        else:
            self.err_out('Failed transferring ESP firmware')
            return False

    def eraseESP(self):
        if not self.getBridge(): return False
        flashedESPFS = subprocess.call([self.paths['esptoolPy'], '--port', self.serialPort_name, 'erase_flash'], stderr=subprocess.STDOUT)
        if flashedESPFS == 0:
            time.sleep(1)
            return True
        else: return False

    def reset(self):
        self.updateSerial()
        self.checkConsole();
        self.serialPort.write('\r\n')
        self.serialPort.write('reset\r\n')

    def netConfig(self):
        if len(self.wifi_ssid) == 0 or len(self.token) != 6:
            print('WiFi and token MUST be set!!')
            return False
        self.updateSerial()
        self.checkConsole();
        self.serialPort.write('\r\nconfig -mode net -wifi "' + self.wifi_ssid + '" "' + self.wifi_pass + '" -token ' + self.token + '\r\n')
        # TODO verify config success
        return True

    def sdConfig(self):
        self.updateSerial()
        self.checkConsole();
        self.serialPort.write('\r\ntime ' + str(int(time.time())) + '\r\n')
        if len(self.wifi_ssid) == 0:
            self.serialPort.write('config -mode sdcard\r\n')
        else:
            self.serialPort.write('config -mode sdcard -wifi "' + self.wifi_ssid + '" "' + self.wifi_pass + '"\r\n')
        # TODO verify config success
        return True

    def resetConfig(self):
        self.updateSerial()
        self.checkConsole();
        self.serialPort.write('\r\nconfig -defaults\r\n')
        # TODO verify config success
        return True

    def end(self):
        if self.serialPort.is_open: self.serialPort.close()

    def register(self):
        try:
            import secret
            print("Founded secrets.py:")
            print("bearer: " + secret.bearer)
            print("Wifi ssid: " + secret.wifi_ssid)
            print("Wifi pass: " + secret.wifi_pass)
            bearer = secret.bearer
            wifi_ssid = secret.wifi_ssid
            wifi_pass = secret.wifi_pass
        except:
            bearer = raw_input("Platform bearer: ")
            wifi_ssid = raw_input("WiFi ssid: ")
            wifi_pass = raw_input("WiFi password: ")

        headers = {'Authorization':'Bearer ' + bearer, 'Content-type': 'application/json',}
        device = {}
        try:
            device['name'] = self.platform_name
        except:
            print('Your device needs a name!')
            # TODO ask for a name
            sys.exit()
        device['device_token'] = binascii.b2a_hex(os.urandom(3))
        self.token = device['device_token']
        device['description'] = ''
        device['kit_id'] = 26
        device['latitude'] = 41.396867
        device['longitude'] = 2.194351
        device['exposure'] = 'indoor'
        device['user_tags'] = 'Lab, Research, Experimental'

        device_json = json.dumps(device)
        backed_device = requests.post('https://api.smartcitizen.me/v0/devices', data=device_json, headers=headers)
        self.id = str(backed_device.json()['id'])
        self.platform_url = "https://smartcitizen.me/kits/" + self.id

        self.serialPort.write('\r\nconfig -mode net -wifi "' + wifi_ssid + '" "' + wifi_pass + '" -token ' + self.token + '\r\n')
        time.sleep(1)

    def inventory_add(self):

        self.getInfo()

        if not hasattr(self, 'token'):
            self.token = ''
        if not hasattr(self, 'platform_name'):
            self.platform_name = ''
        if not hasattr(self, 'platform_url'):
            self.platform_url = ''

        inv_path = "inventory.csv"

        if os.path.exists(inv_path):
            shutil.copyfile(inv_path, inv_path+".BAK")
            csvFile = open("inventory.csv", "a")
        else:
            csvFile = open(inv_path, "w")
            # time,serial,mac,sam_firmVer,esp_firmVer,description,token,platform_name,platform_url
            csvFile.write("time,serial,mac,sam_firmVer,esp_firmVer,description,token,platform_name,platform_url\n")

        csvFile.write(time.strftime("%Y-%m-%dT%H:%M:%SZ,", time.gmtime()))
        csvFile.write(self.sam_serialNum + ',' + self.esp_macAddress + ',' + self.sam_firmVer + ',' + self.esp_firmVer + ',' + self.description + ',' + self.token + ',' + self.platform_name + ',' + self.platform_url + '\n')
        csvFile.close()

    def std_out(self, msg):
        if self.verbose >= 2: print(msg)

    def err_out(self, msg):
        if self.verbose >= 1:
            sys.stdout.write("\033[1;31m")
            print('ERROR ' + msg)
            sys.stdout.write("\033[0;0m")

