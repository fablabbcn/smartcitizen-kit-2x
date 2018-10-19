#!/usr/bin/python

import serial.tools.list_ports, serial, time

class bcolors:
    OKBLUE = '\033[36m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

print('Waiting for Smartcitizen Kits connected to USB...')
ff = open("inventory.csv", "r")
inventoryFile = ff.read()
ff.close()
prevDescription = ''

while True:
    SCK = {}

    devList = list(serial.tools.list_ports.comports())

    for dev in devList:
        try:
            if "Smartcitizen" in dev.product:

                print('Found' + bcolors.OKBLUE + ' Smartcitizen kit!!' + bcolors.ENDC)

                SCK['serial'] = dev.serial_number

                repeated = False
                if SCK['serial'] in inventoryFile:
                    answer = raw_input("Kit already in inventory, add i again? (Y/n): ")
                    if 'n' in answer.lower(): repeated = True

                if not repeated:
                    time.sleep(2)
                    ser = False
                    while not ser:
                        ser = serial.Serial(dev.device, 115200)
                    time.sleep(1)
                    ser.write('shell -on\r\n')

                    time.sleep(1)
                    ser.write('version\r\n')
                    while True:
                        line = ser.readline()
                        if 'SAM version' in line:
                            SCK['sam_ver'] = line.split(':')[1].strip()
                        if 'ESP version' in line:
                            SCK['esp_ver'] = line.split(':')[1].strip()
                            break

                    time.sleep(1)    
                    ser.write('netinfo\r\n')                
                    while True:
                        line = ser.readline()
                        if 'MAC address' in line:
                            SCK['mac'] = line.split('address:')[1].strip()
                            break

                    SCK['description'] = raw_input(bcolors.WARNING + 'Please enter description ' + prevDescription + ': ' + bcolors.ENDC)
                    if not len(SCK['description']) > 0:
                        SCK['description'] = prevDescription[1:-1]
                    prevDescription = '[' + SCK['description'] + ']'

                    csvFile = open("inventory.csv", "a")
                    csvFile.write(time.strftime("%Y-%m-%dT%H:%M:%SZ,", time.gmtime()))
                    csvFile.write(SCK['serial'] + ',' + SCK['mac'] + ',' + SCK['sam_ver'] + ',' + SCK['esp_ver'] + ',' + SCK['description'] + '\n')
                    csvFile.close()

                    print(bcolors.OKGREEN + 'Kit saved...' + bcolors.ENDC)

                    connected = True
                    while connected:
                        time.sleep(0.1)
                        devList = list(serial.tools.list_ports.comports())
                        connected = False
                        for dev in devList:
                            if 'Smartcitizen' in dev.product: connected = True

                print('Waiting for Smartcitizen Kits connected to USB...')
                ff = open("inventory.csv", "r")
                inventoryFile = ff.read()
                ff.close()
        except:
            pass

# TODO
# Integrate this code in fastFlash
# When the kit is already in inventory offer the option of replacing it
# put inventory online and update it via http/mqtt warever
