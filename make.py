#!/usr/bin/python

import sys, time, os
import subprocess
sys.path.append("./tools")

def ERROR(msg=''):
    enablePrint()
    sys.stdout.write("\033[1;31m")
    print('ERROR ' + msg)
    sys.stdout.write("\033[0;0m")
    if not verbose: blockPrint()
    sys.exit()

def OK(msg=''):
    enablePrint()
    sys.stdout.write("\033[1;32m")
    print('OK ' + msg)
    sys.stdout.write("\033[0;0m")
    if not verbose: blockPrint()

def oneLine(msg):
    enablePrint()
    sys.stdout.write(msg)
    sys.stdout.flush()
    if not verbose: blockPrint()

def blockPrint():
    sys.stdout = open(os.devnull, 'w')

def enablePrint():
    sys.stdout = sys.__stdout__

if '-h' in sys.argv or '--help' in sys.argv or '-help' in sys.argv or len(sys.argv) < 2 or (not 'build' in sys.argv and not 'flash' in sys.argv and not 'boot' in sys.argv):
    print('USAGE:\n\n\tpython make.py [options] action[s] target[s] [target-options]')
    print('\nOptions:')
    print('\t-v: verbose')
    print('\t-k: keep configuration')
    print('\t-p port: specify a port instead of scanning')
    print('\t-f: force flashing even if no SCK is found, (port must be specified)')
    print('\nActions:\n\tboot: flash SAM bootloader (Extra hardware is needed)\n\tbuild: build firmware\n\tflash: upload compiled code')
    print('\nTargets:\n\tsam: SAMD21 chip\n\tesp: ESP8266 (WiFi) chip')
    print('\nTarget Options (only SAM):')
    print('\t--env: Environment to build (all to build all)')
    sys.exit()

verbose = False
blockPrint()
if '-v' in sys.argv:
    verbose = True
    enablePrint()

import sck
kit = sck.sck()

if 'flash' in sys.argv:
    force = False
    port = None
    if '-p' in sys.argv:
        port = sys.argv[sys.argv.index('-p')+1]
        if '-f' in sys.argv: force = True
    elif '-f' in sys.argv: ERROR('Port must be specified to force flashing'); sys.exit()
    if not kit.begin(port=port, force=force): sys.exit()
    if '-k' in sys.argv:
        kit.getConfig()
        if kit.mode == 'network':
            print('Current mode: ' + kit.mode + ', Wifi: ' + kit.wifi_ssid + ' - ' + kit.wifi_pass + ', Token: ' + kit.token)
        elif kit.mode == 'sdcard':
            ww = ""
            if len(kit.wifi_ssid) > 0: ww = ', Wifi: ' + kit.wifi_ssid + ' - ' + kit.wifi_pass
            print('Current mode: ' + kit.mode + ww)
        else:
            print('Kit is not configured')

if 'erase' in sys.argv:
    print('Erasing ESP flash...')
    kit.eraseESP()

if 'boot' in sys.argv:
    print('Making SAM bootloader...')
    print('Remember to change Makefile.user in the bootloader folder!')
    os.chdir('bootloader/uf2-samdx1')
    make_bootloader = subprocess.call(['make'], stdout=sys.stdout, stderr=subprocess.STDOUT)
    # Default board version
    board_ver = 'sck21'
    # Check if there is an user Makefile
    user_makefile = 'Makefile.user'
    if os.path.exists(user_makefile):
        with open(user_makefile, 'r') as file:
            boards=file.readlines()[0]
        board_ver = boards.split('=')[1].replace('.', '')
    if make_bootloader == 0:
        oneLine('Flashing SAM bootloader...')
        openocd = subprocess.call(['openocd', '-f', f'openocd_{board_ver}_SAMICE.cfg'], stdout=sys.stdout, stderr=subprocess.STDOUT)
        if openocd != 0:
            ERROR("Failed flashing SCK bootloader!!!\nDo you have openocd executable in your path???");
        else:
            OK()
    else:
        ERROR('Failed building SCK bootloader!!!')
    os.chdir('../..')

if 'build' in sys.argv:
    if 'sam' in sys.argv:
        oneLine('Building SAM firmware... ')
        env = 'sck2'
        if '--env' in sys.argv:
            env = sys.argv[sys.argv.index('--env')+1]
        if kit.buildSAM(sys.stdout, env):
            buildSAMOK = True
            OK()
        else:
            buildSAMOK = False
            ERROR()

    if 'esp' in sys.argv:
        oneLine('Building ESP firmware... ')
        if kit.buildESP(sys.stdout):
            buildESPOK = True
            OK()
        else:
            buildESPOK = False
            ERROR()
else:
    # We do this to avoid preventing flashing with previously built fw
    buildSAMOK = True
    buildESPOK = True

if 'flash' in sys.argv:
    if not 'build' in sys.argv:
        if not verbose: enablePrint()
        print('\nWARNING: No build instruction received, trying to flash previous built firmware...')
        if not verbose: blockPrint()

    if 'sam' in sys.argv:
        time.sleep(1)
        oneLine('Flashing SAM firmware...')

        env = 'sck2'
        if '--env' in sys.argv:
            env = sys.argv[sys.argv.index('--env')+1]

        if buildSAMOK:
            if kit.flashSAM(sys.stdout, env): OK()
        else: ERROR()

    if 'esp' in sys.argv:
        if not buildESPOK: ERROR()
        time.sleep(0.5)
        oneLine('Flashing ESP firmware')
        for i in range(4):
            mySpeed = 115200 / pow(2, i)
            oneLine(' at ' + str(mySpeed) + '...')
            time.sleep(1)
            if kit.flashESP(mySpeed, sys.stdout):
                OK()
                break
            else:
                if i == 3: ERROR()
                else:
                    time.sleep(3)
                    if i == 0: oneLine('   Retry')
                    oneLine(' ' + str(i+1))

    if '-k' in sys.argv and len(kit.mode) > 0:
        oneLine("Reconfiguring kit...")
        time.sleep(1)

        if 'network' in kit.mode:
            if kit.netConfig(): OK()
            else: ERROR()
        elif 'sdcard' in kit.mode:
            if kit.sdConfig(): OK()
            else: ERROR()
