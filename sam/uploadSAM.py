Import("env")
import serial, time, os, subprocess

# def create_UF2(source, target, env):
#         # create UF2 file
#         createUF2 = subprocess.Popen(["uf2conv.py", "-o", ".pioenvs/sck2/firmware.uf2", ".pioenvs/sck2/firmware.bin"])
#         createUF2.wait()
#         print "Created UF2 firmware file"

# env.AddPostAction("$BUILD_DIR/firmware.bin", create_UF2)
#
# Upload actions
#
# def after_upload(source, target, env):
        # time.sleep(1)
	# print "Setting default config..."
	# myPort = serial.Serial("/dev/" + env.get("UPLOAD_PORT"))
	# myPort.write("\r\nconfig -defaults\r\n")

print("Current build targets", map(str, BUILD_TARGETS))

# env.AddPreAction("upload", before_upload)
# env.AddPostAction("upload", after_upload)

#
# Custom actions when building program/firmware
#
def before_build():
    if not os.path.isdir(".platformio/packages/framework-arduino-samd/variants/sck2"):
        checkout = subprocess.Popen(["git", "checkout", ".platformio/packages/framework-arduino-samd/variants/sck2"])
        checkout.wait()
        checkout = subprocess.Popen(["git", "checkout", ".platformio/packages/framework-arduino-samd/libraries/__cores__/samd/I2S/src/utility/SAMD21_I2SDevice.h"])
        checkout.wait()

before_build() 
# env.AddPreAction("buildprog", before_build)
# env.AddPostAction("buildprog", callback...)

# #
# # Custom actions for specific files/objects
# #

# env.AddPreAction("$BUILD_DIR/firmware.elf", before_build)
# env.AddPostAction("$BUILD_DIR/firmware.hex", callback...)

# # custom action before building SPIFFS image. For example, compress HTML, etc.
# env.AddPreAction("$BUILD_DIR/spiffs.bin", callback...)

# # custom action for project's main.cpp
# env.AddPostAction("$BUILD_DIR/src/main.cpp.o", callback...)
