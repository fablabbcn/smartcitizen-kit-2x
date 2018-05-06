Import("env")
import serial, time

#
# Upload actions
#

def after_upload(source, target, env):
	time.sleep(1);
	print "Setting default config..."
	myPort = serial.Serial("/dev/" + env.get("UPLOAD_PORT"))
	myPort.write("config -defaults")

print "Current build targets", map(str, BUILD_TARGETS)

# env.AddPreAction("upload", before_upload)
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
