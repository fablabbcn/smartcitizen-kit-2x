Import("env")
import serial, time, sys, glob

#
# Upload actions
#

portName = False

def before_upload(source, target, env):
	print("\n\nSearching for a Smartcitizen kit...")
	myPort = selectPort(serialPorts())
	if myPort:
		print("Asking for upload bridge...")
		myPort.write("")
		myPort.write("esp -flash\n")
		env.Replace(UPLOAD_PORT=portName)
		time.sleep(1)

def after_upload(source, target, env):
	print("All good!!!")
	global portName

print("Current build targets", map(str, BUILD_TARGETS))

env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_upload)

env.AddPreAction("uploadfs", before_upload)
env.AddPostAction("uploadfs", after_upload)


#
# Custom actions when building program/firmware
#

# env.AddPreAction("buildprog", callback...)
# env.AddPostAction("buildprog", callback...)

# #
# # Custom actions for specific files/objects
# #

# env.AddPreAction("$BUILD_DIR/firmware.elf", [callback1, callback2,...])
# env.AddPostAction("$BUILD_DIR/firmware.hex", callback...)

# # custom action before building SPIFFS image. For example, compress HTML, etc.
# env.AddPreAction("$BUILD_DIR/spiffs.bin", callback...)

# # custom action for project's main.cpp
# env.AddPostAction("$BUILD_DIR/src/main.cpp.o", callback...)


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

	for port in ports:
	    try:
	        s = serial.Serial(port)
	        s.close()
	        result.append(port)
	    except (OSError, serial.SerialException):
	        pass
	return result

def selectPort(ports):

	for port in ports:
		try:
			s = serial.Serial(port)
			for i in range(3):
				s.write('\r\n\r\n')
				time.sleep(0.2)
				response = s.read(s.in_waiting)
				if 'SCK' in response or 'Sdcard' in response:
					print('Smartcitizen kit found on ' + port)
					global portName
					portName = port
					return s
			s.close()
		except (OSError, serial.SerialException):
			pass

	print('No Smartcitizen kit found, please check your USB connection')
	sys.exit(-1)
