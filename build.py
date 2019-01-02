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
    print('\noptions: -v: verbose')
    print('actions: build, flash, register, inventory')
    print('targets: sam, esp')
    print('\nFor bootloader flashing you still need to use the old script build.sh')
    sys.exit()

import sck
kit = sck.sck()

if 'flash' or 'register' in sys.argv:
    kit.begin() 

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
        oneLine('Flashing ESP firmware...')
        for i in range(4):
            if kit.flashESP(sys.stdout): 
                OK()
                break;
            else: 
                if i == 3: ERROR()
                else:
                    if i == 0: oneLine('   Retry: ')
                    oneLine(str(i+1) + '... ')
                    time.sleep(3)


if 'erase' in sys.argv:
    print('Erasing ESP flash...')
    kit.eraseESP()

