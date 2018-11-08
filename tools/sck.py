#!/usr/bin/python

import serial.tools.list_ports, serial, os, time, subprocess, uf2conv, shutil
FNULL = open(os.devnull, 'w')


class sck:

    repoPath = os.path.split(os.getcwd())[0]
    binPath = os.path.join(repoPath, 'bin')
    esptoolEXE = os.path.join(repoPath, 'tools', 'esptool.py')

    samBIN = 'SAM_firmware.bin'
    samUF2 = 'SAM_firmware.uf2'
    espBIN = 'ESP_firmware.bin'
    espfsBIN = 'ESP_filesystem.bin'

    branch = 'master'
    serial = ''
    port = ''

    token = ''
    wifi_ssid = ''
    wifi_pass = ''

    def updateSerial(self):
        timeout = time.time() + 15
        while True:
            devList = list(serial.tools.list_ports.comports())
            for d in devList:
                if self.serial in d.serial_number:
                    self.port = d.device
                    return
                if time.time() > timeout:
                    print 'Timeout waiting for ' + branch + ' device'
                    break

    def bootloader(self):
        print 'Calling bootloader for ' + self.branch + ' branch kit...'
        self.updateSerial()
        ser = serial.Serial(self.port, 1200)
        ser.setDTR(False)
        time.sleep(2)
        mps = uf2conv.getdrives()
        for p in mps:
            if 'INFO_UF2.TXT' in os.listdir(p):
                return p
        return False

    def updateGIT(self):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        subprocess.call(['git', 'remote', 'update', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        r = subprocess.check_output(['git', 'status'])
        if not 'up-to-date' in r:
            print 'Updating branch ' + self.branch + ', kit needss update.'
            subprocess.call(['git', 'pull', '--all'], stderr=subprocess.STDOUT)                                             
        else:
            print 'Branch ' + self.branch + ' doesn\'t need update.' 

    def buildSAM(self):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        os.chdir('sam')
        piorun = subprocess.call(['pio', 'run'], stderr=subprocess.STDOUT)
        shutil.copyfile(os.path.join(os.getcwd(), '.pioenvs', 'sck2', 'firmware.bin'), os.path.join(self.binPath, self.samBIN))
        with open(os.path.join(self.binPath, self.samBIN), mode='rb') as myfile:
            inpbuf = myfile.read()
        outbuf = uf2conv.convertToUF2(inpbuf)       
        uf2conv.writeFile(os.path.join(self.binPath, self.samUF2), outbuf)
        os.chdir(self.repoPath)

    def flashSAM(self):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        mountpoint = self.bootloader()
        shutil.copyfile(os.path.join(self.binPath, self.samUF2), os.path.join(mountpoint, self.samUF2))
        time.sleep(2)


    def buildESP(self):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        os.chdir('esp')
        piorun = subprocess.call(['pio', 'run'], stderr=subprocess.STDOUT)
        if piorun == 0:
            shutil.copyfile(os.path.join(os.getcwd() , '.pioenvs', 'esp12e', 'firmware.bin'), os.path.join(self.binPath, self.espBIN))
            return True
        return False

    def flashESP(self):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        self.updateSerial()
        ser = serial.Serial(self.port, 115200)
        ser.write('\r\n')
        ser.write('esp -flash\r\n')
        flashedESP = subprocess.call([self.esptoolEXE, '--port', self.port, '--baud', '115200', 'write_flash', '0x000000', os.path.join(self.binPath, self.espBIN)], stderr=subprocess.STDOUT) 
        # flashedESP = subprocess.call([self.esptoolEXE, '-cp', self.port, '-cb', '115200', '-ca', '0x000000', '-cf', os.path.join(self.binPath, self.espBIN)], stderr=subprocess.STDOUT) 
        if flashedESP == 0: return True
        else: return False

    def buildESPFS(self):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        os.chdir('esp')
        piorun = subprocess.call(['pio', 'run', '-t', 'buildfs'], stderr=subprocess.STDOUT)
        if piorun == 0:
            shutil.copyfile(os.path.join(os.getcwd(), '.pioenvs', 'esp12e', 'spiffs.bin'), os.path.join(self.binPath, self.espfsBIN))
            return True
        return False

    def flashESPFS(self):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=FNULL, stderr=subprocess.STDOUT)
        self.updateSerial()
        ser = serial.Serial(self.port, 115200)
        ser.write('\r\n')
        ser.write('esp -flash\r\n')
        flashedESPFS = subprocess.call([self.esptoolEXE, '--port', self.port, 'baud', '115200', 'write_flash', '0x300000', os.path.join(self.binPath, self.espfsBIN)], stderr=subprocess.STDOUT)
        # flashedESPFS = subprocess.call([self.esptoolEXE, '-cp', self.port, '-cb', '115200', '-ca', '0x300000', '-cf', os.path.join(self.binPath, self.espfsBIN)], stderr=subprocess.STDOUT)
        if flashedESPFS == 0: return True
        else: return False

    def netConfig(self):
        if len(self.wifi_ssid) == 0 or len(self.wifi_pass) == 0 or len(self.token) != 6:
            print('WiFi and token MUST be set!!')
            return False
        self.updateSerial()
        ser = serial.Serial(self.port, 115200)
        ser.write('\r\n')
        ser.write('config -mode net -wifi "' + self.wifi_ssid + '" "' + self.wifi_pass + '" -token ' + self.token + '\r\n')

    def sdConfig(self):
        self.updateSerial()
        ser = serial.Serial(self.port, 115200)
        ser.write('\r\n')
        ser.write('time ' + str(int(time.time())) + '\r\n')
        ser.write('config -mode sdcard\r\n')

    def resetConfig(self):
        self.updateSerial()
        ser = serial.Serial(self.port, 115200)
        ser.write('\r\n')
        ser.write('config -defaults\r\n')


