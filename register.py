#!/usr/bin/python

import sys, time, os
sys.path.append("./tools")

def blockPrint():
    sys.stdout = open(os.devnull, 'w')

def oneLine(msg):
    enablePrint()
    sys.stdout.write(msg)
    sys.stdout.flush()
    if not verbose: blockPrint()

def enablePrint():
    sys.stdout = sys.__stdout__

import sck
kit = sck.sck()

if 'flash' or 'register' in sys.argv:
    kit.begin() 

verbose = False
blockPrint()
if '-v' in sys.argv: 
    verbose = True
    enablePrint()

if 'register' in sys.argv:
    kit.getInfo()
    if '-n' not in sys.argv:
        kit.platform_name = 'test #'
    else:
        kit.platform_name = sys.argv[sys.argv.index('-n')+1]
    kit.platform_name = kit.platform_name + ' #' + kit.mac[-5:].replace(':', '')
    kit.register()

    print("\r\nSerial number: " + kit.serial_num)
    print("Mac address: " + kit.mac)
    print("Device token: " + kit.token)
    print("Platform kit name: " + kit.platform_name)
    print("Platform page:" + kit.platform_url)

if 'inventory' in sys.argv:
    kit.description = sys.argv[sys.argv.index('-d')+1]
    kit.inventory_add()
    
if '-h' in sys.argv or '--help' in sys.argv or '-help' in sys.argv:
    print('USAGE:\n\nresgister.py [options] action[s]')
    print('\noptions: -v: verbose')
    print('actions: register, inventory')
    print('register options: -n platform_name')
    print('inventory -d "description"')
    sys.exit()

