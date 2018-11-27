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
    print('actions: build, flash')
    print('targets: sam, esp, espfs')
    print('\nFor bootloader flashing you still need to use the old script build.sh')
    sys.exit()

import sck
kit = sck.sck()
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

    if 'espfs' in sys.argv or 'all' in sys.argv:
        oneLine('Building ESP filesystem... ')
        if kit.buildESPFS(sys.stdout): OK()
        else: ERROR()

if 'flash' in sys.argv:
    if 'sam' in sys.argv or 'all' in sys.argv:
        oneLine('Flashing SAM firmware...')
        if kit.flashSAM(sys.stdout): OK()
        else: ERROR()

    if 'esp' in sys.argv or 'all' in sys.argv:
        oneLine('Flashing ESP firmware...')
        if kit.flashESP(sys.stdout): OK()
        else: ERROR()

    if 'espfs' in sys.argv or 'all' in sys.argv:
        oneLine('Flashing ESP filesystem...')
        if kit.flashESPFS(sys.stdout): OK()
        else: ERROR()

if 'erase' in sys.argv:
    print('Erasing ESP flash...')
    kit.eraseESP()

# time.sleep(3)
kit.reset()

