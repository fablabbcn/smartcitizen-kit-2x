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

if '-h' in sys.argv or '--help' in sys.argv or '-help' in sys.argv:
    print('USAGE:\n\nresgister.py [options] action[s]')
    print('\noptions: -v: verbose')
    print('actions: register, inventory')
    print('register options: -n platform_name')
    print('inventory -d "description"')
    sys.exit()

import sck
kit = sck.sck()
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
    kit.platform_name = kit.platform_name + ' #' + kit.esp_macAddress[-5:].replace(':', '')
    kit.register()

    print("\r\nSerial number: " + kit.sam_serialNum)
    print("Mac address: " + kit.esp_macAddress)
    print("Device token: " + kit.token)
    print("Platform kit name: " + kit.platform_name)
    print("Platform page:" + kit.platform_url)

if 'inventory' in sys.argv:
    kit.description = sys.argv[sys.argv.index('-d')+1]
    # TODO Inventory add shouldn't be part of sck python library, we should put it here
    kit.inventory_add()
    

