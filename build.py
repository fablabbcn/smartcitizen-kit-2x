#!/usr/bin/python

import sys, time, os
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


if '-h' in sys.argv or '--help' in sys.argv or '-help' in sys.argv:
    print('USAGE:\n\nbuild.py [options] action[s] target[s]')
    print('\noptions: -v: verbose -k: keep configuration')
    print('actions: build, flash, register, inventory')
    print('targets: sam, esp')
    print('\nFor bootloader flashing you still need to use the old script build.sh')
    sys.exit()

import sck
kit = sck.sck()

if 'flash' in sys.argv or 'register' in sys.argv:
    kit.begin() 
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


verbose = False
blockPrint()
if '-v' in sys.argv: 
    verbose = True
    enablePrint()

if 'build' in sys.argv:
    if 'sam' in sys.argv or 'all' in sys.argv:
        oneLine('Building SAM firmware... ')
        if kit.buildSAM(sys.stdout): OK()
        else: ERROR()
            
    if 'esp' in sys.argv or 'all' in sys.argv:
        oneLine('Building ESP firmware... ')
        if kit.buildESP(sys.stdout): OK()
        else: ERROR()


if 'flash' in sys.argv:
    if 'sam' in sys.argv or 'all' in sys.argv:
        time.sleep(1)
        oneLine('Flashing SAM firmware...')
        if kit.flashSAM(sys.stdout): OK()
        else: ERROR()

    if 'esp' in sys.argv or 'all' in sys.argv:
        oneLine('Flashing ESP firmware')
        for i in range(4):
            mySpeed = 921600 / pow(2, i)
            oneLine(' at ' + str(mySpeed) + '...')
            if kit.flashESP(mySpeed, sys.stdout): 
                OK()
                break;
            else: 
                if i == 3: ERROR()
                else:
                    time.sleep(3)
                    if i == 0: oneLine('   Retry')
                    oneLine(' ' + str(i+1))

    if '-k' in sys.argv and len(kit.mode) > 0:
        oneLine("Reconfiguring kit...")
        if 'network' in kit.mode: 
            if kit.netConfig(): OK()
            else: ERROR()
        elif 'sdcard' in kit.mode: 
            if kit.sdConfig(): OK()
            else: ERROR()

if 'erase' in sys.argv:
    print('Erasing ESP flash...')
    kit.eraseESP()

