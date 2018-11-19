#!/usr/bin/python

import sys
sys.path.append("./tools")

import sck
kit = sck.sck()
kit.begin() 

if 'sam' in sys.argv:
    kit.buildSAM()
    kit.flashSAM()

if 'esp' in sys.argv:
    kit.buildESP()
    kit.flashESP()

if 'espfs' in sys.argv:
    kit.buildESPFS()
    kit.flashESPFS()


