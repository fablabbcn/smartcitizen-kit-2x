#!/usr/bin/python

import serial, time, sys, glob, os

class bcolors:
    OKBLUE = '\033[36m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def serialPorts():
    """Lists serial ports

    :raises EnvironmentError:
        On unsupported or unknown platforms
    :returns:
        A list of available serial ports
    """
    if sys.platform.startswith('win'):
        ports = ['COM' + str(i + 1) for i in range(256)]

    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this is to exclude your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')

    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')

    else:
        raise EnvironmentError('Unsupported platform')

    result = []

    #TODO validar que quien responde es un SCK
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result


print("Monitoring for Smartcitizen kits connected via USB...")

def flash(wichPort):
    print("flashing SCK in port " + wichPort)
    ser = serial.Serial(wichPort, 1200)
    ser.setDTR(False)
    time.sleep(2)

   
    pass

def follow(syslog_file):
  syslog_file.seek(0,2)
  while True:
     line = syslog_file.readline()
     print(line)

flashingList = []

syslog = open("/var/log/syslog", "r")

while True:

    follow(syslog)
    # portList = serialPorts()

    # for port in portList:
    #     if port not in flashingList:
    #         flashingList.append(port)
    #         flash(port)
