Import("env")
import serial, time, sys, glob

#
# Upload actions
#

def before_upload(source, target, env):
    # print "before_upload"
	time.sleep(1)
	myPort = selectPort(serialPorts())
	ser = serial.Serial(myPort)
	print("Asking for upload bridge...")
	ser.write("")
	time.sleep(1)
	ser.write("set config mode esp flash\n")
	ser.close()
	time.sleep(1)


def after_upload(source, target, env):
    print "after_upload"
    # do some actions

print "Current build targets", map(str, BUILD_TARGETS)

env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_upload)

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
		print "\nOnly one serial port found, using it! (" + ports[0] + ")"
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
		print "No serial port available, bye!"
		sys.exit()
