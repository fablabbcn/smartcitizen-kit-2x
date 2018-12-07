#!/usr/bin/python

import serial.tools.list_ports, serial, os, time, subprocess, uf2conv, shutil, sys
FNULL = open(os.devnull, 'w')


class sck:

    repoPath = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip()
    binPath = os.path.join(str(repoPath), 'bin')
    esptoolEXE = os.path.join(str(repoPath), 'tools', 'esptool.py')

    samBIN = 'SAM_firmware.bin'
    samUF2 = 'SAM_firmware.uf2'
    espBIN = 'ESP_firmware.bin'
    espfsBIN = 'ESP_filesystem.bin'

    branch = 'master'
    serial = ''
    port_name = ''
    port = None


    token = ''
    wifi_ssid = ''
    wifi_pass = ''

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
        if i == 0: print('No SKC found!!!')
        elif i > 0:
            if i == 1:
                wich_kit = 0
            else:
                wich_kit = int(raw_input('Multiple Kits found, please select one: ')) - 1
        
        self.serial = kit_list[wich_kit].serial_number
        self.port_name = kit_list[wich_kit].device

    def updateSerial(self):
        timeout = time.time() + 15
        while True:
            devList = list(serial.tools.list_ports.comports())
            found = False
            for d in devList:
                try: 
                    if self.serial in d.serial_number:
                        self.port_name = d.device
                        found = True
                    if time.time() > timeout:
                        print('Timeout waiting for device')
                        sys.exit()
                except:
                    pass
            if found: break

        timeout = time.time() + 15
        while True:
            try:
                self.port = serial.Serial(self.port_name, 115200)
            except:
                if time.time() > timeout:
                    print('Timeout waiting for serial port')
                    sys.exit()
                time.sleep(0.2)
            if self.port.is_open: return

    def bootloader(self):
        self.updateSerial()
        self.port.close()
        self.port = serial.Serial(self.port_name, 1200)
        self.port.setDTR(False)
        time.sleep(5)
        mps = uf2conv.getdrives()
        for p in mps:
            if 'INFO_UF2.TXT' in os.listdir(p):
                return p
        return False

    def updateGIT(self, out=sys.__stdout__):
        os.chdir(self.repoPath)
        subprocess.call(['git', 'checkout', self.branch], stdout=out, stderr=subprocess.STDOUT)
        subprocess.call(['git', 'remote', 'update', self.branch], stdout=out, stderr=subprocess.STDOUT)
        r = subprocess.check_output(['git', 'status'])
        if not 'up-to-date' in r:
            print('Updating branch ' + self.branch + ', kit needss update.')
            subprocess.call(['git', 'pull', '--all'], stderr=subprocess.STDOUT)                                             
        else:
            print('Branch ' + self.branch + ' doesn\'t need update.')

    def buildSAM(self, out=sys.__stdout__):
        os.chdir(self.repoPath)
        os.chdir('sam')
        piorun = subprocess.call(['pio', 'run'], stdout=out, stderr=subprocess.STDOUT)
        if piorun != 0: return False
        try:
            shutil.copyfile(os.path.join(os.getcwd(), '.pioenvs', 'sck2', 'firmware.bin'), os.path.join(self.binPath, self.samBIN))
        except: error(self)
        with open(os.path.join(self.binPath, self.samBIN), mode='rb') as myfile:
            inpbuf = myfile.read()
        outbuf = uf2conv.convertToUF2(inpbuf)       
        uf2conv.writeFile(os.path.join(self.binPath, self.samUF2), outbuf)
        os.chdir(self.repoPath)
        return True

    def flashSAM(self, out=sys.__stdout__):
        os.chdir(self.repoPath)
        mountpoint = self.bootloader()
        print(mountpoint)
        try:
            shutil.copyfile(os.path.join(self.binPath, self.samUF2), os.path.join(mountpoint, self.samUF2))
        except:
            return False
        time.sleep(2)
        return True

    def getBridge(self):
        timeout = time.time() + 15
        while True:
            self.updateSerial()
            time.sleep(0.5)
            self.port.write('\r\n')
            buff = self.port.read(self.port.in_waiting)
            if 'SCK' in buff: break
            if time.time() > timeout:
                print('Timeout waiting for SAM bridge')
                sys.exit()
            time.sleep(0.2)
        self.port.write('esp -flash\r\n')
        return True

    def buildESP(self, out=sys.__stdout__):
        os.chdir(self.repoPath)
        os.chdir('esp')
        piorun = subprocess.call(['pio', 'run'], stdout=out, stderr=subprocess.STDOUT)
        if piorun == 0:
            shutil.copyfile(os.path.join(os.getcwd() , '.pioenvs', 'esp12e', 'firmware.bin'), os.path.join(self.binPath, self.espBIN))
            return True
        return False

    def flashESP(self, out=sys.__stdout__):
        os.chdir(self.repoPath)
        if not self.getBridge(): return False
        flashedESP = subprocess.call([self.esptoolEXE, '--port', self.port_name, '--baud', '115200', 'write_flash', '0x000000', os.path.join(self.binPath, self.espBIN)], stdout=out, stderr=subprocess.STDOUT) 
        self.port.write('byebyebye')
        if flashedESP == 0:
            time.sleep(1)
            return True
        else: return False

    def buildESPFS(self, out=sys.__stdout__):
        print("ESP filesystem is no longer used!")
        os.chdir(self.repoPath)
        os.chdir('esp')
        piorun = subprocess.call(['pio', 'run', '-t', 'buildfs'], stdout=out, stderr=subprocess.STDOUT)
        if piorun == 0:
            shutil.copyfile(os.path.join(os.getcwd(), '.pioenvs', 'esp12e', 'spiffs.bin'), os.path.join(self.binPath, self.espfsBIN))
            return True
        return False

    def flashESPFS(self, out=sys.__stdout__):
        print("ESP filesystem is no longer used!")
        os.chdir(self.repoPath)
        if not self.getBridge(): return False
        flashedESPFS = subprocess.call([self.esptoolEXE, '--port', self.port_name, '--baud', '115200', 'write_flash', '0x300000', os.path.join(self.binPath, self.espfsBIN)], stdout=out, stderr=subprocess.STDOUT)
        self.port.write('byebyebye')
        if flashedESPFS == 0:
            time.sleep(1)
            return True
        else: return False

    def eraseESP(self):
        if not self.getBridge(): return False
        flashedESPFS = subprocess.call([self.esptoolEXE, '--port', self.port_name, 'erase_flash'], stderr=subprocess.STDOUT)
        self.port.write('byebyebye')
        if flashedESPFS == 0:
            time.sleep(1)
            return True
        else: return False

    def reset(self):
        self.updateSerial()
        self.port.write('\r\n')
        self.port.write('reset\r\n')

    def netConfig(self):
        if len(self.wifi_ssid) == 0 or len(self.wifi_pass) == 0 or len(self.token) != 6:
            print('WiFi and token MUST be set!!')
            return False
        self.updateSerial()
        self.port.write('\r\n')
        self.port.write('config -mode net -wifi "' + self.wifi_ssid + '" "' + self.wifi_pass + '" -token ' + self.token + '\r\n')

    def sdConfig(self):
        self.updateSerial()
        self.port.write('\r\n')
        self.port.write('time ' + str(int(time.time())) + '\r\n')
        self.port.write('config -mode sdcard\r\n')

    def resetConfig(self):
        self.updateSerial()
        self.port.write('\r\n')
        self.port.write('config -defaults\r\n')

    def end(self):
        if self.port.is_open: self.port.close()

