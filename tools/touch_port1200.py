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

def selectPort(ports):
	if len(ports) == 1:
		print "\nOnly one serial port found, using it! (" + bcolors.OKBLUE + ports[0] + bcolors.ENDC + ")"
		return ports[0]
	if len(ports) > 0:
		print "\nSelect a serial port from the list:\n"
		for p in ports:
			print "\t" + str(ports.index(p) + 1) + ". " + p

		i = raw_input("\n[" + ports[0] + "]: ")

		try:
		 	val = int(i)
		except:
			if i == "": val = 1
			else: val = -1

		if val-1 >= len(ports) or val-1 < 0:
			print "\n*** please input the port number! ***"
		 	return False
		else:
			return ports[val -1]
	else:
		print bcolors.FAIL + "No serial port available, bye!" + bcolors.ENDC
		sys.exit()


myPort = selectPort(serialPorts())

if myPort:
    ser = serial.Serial(myPort, 1200)
    ser.setDTR(False)
    time.sleep(2)
else:
    print("No serial port found, bye")

