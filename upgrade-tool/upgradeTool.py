#!/usr/bin/python
import glob, sys, serial, time

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

	print "\nSelect a serial port from the list:\n"

	if len(ports) > 0:
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

def getKitVersion(ser):

	print "Trying to communicate with your kit..."

	# clean buffer
	ser.reset_input_buffer()

	ser.write("get version\n")
	ser.flush()

	# read two lines (echo from kit)
	ser.readline()
	ser.readline()

	# read version
	version = ser.readline()

	# print version.strip("\r\n")
	return version.strip("\r\n")

# Ask user to select serial port
myPort = False
while not myPort:
	myPort = selectPort(serialPorts())

# Open selected serial port
# TODO feedback in case of port error
ser = serial.Serial(myPort, 115200)
ser.setDTR(False)
time.sleep(0.5)

# Put kit in setup mode
ser.write("set mode apmode\n")
time.sleep(0.5)

# Comunicate with kit and get version
print "Your kit firmware version -->\t"getKitVersion(ser)



ser.close();