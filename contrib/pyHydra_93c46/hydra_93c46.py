#!/usr/bin/env python3
#
# Script to communicate to 94c46 3-wire EEPROM with Hydrabus.
#
# Author: Sylvain Pelissier <sylvain.pelissier@gmail.com>
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#
import pyHydrabus

from binascii import hexlify

def write_enable():
    # Write enable is 10011XXXX
    cs.value=1
    sm.sda = 1
    sm.clock()
    sm.sda = 0
    sm.clocks(2)
    sm.sda = 1
    sm.clocks(8)
    cs.value=0

def write_disable():
    # Write disable is 10000XXXXX
    cs.value=1
    sm.sda = 1
    sm.clock()
    sm.sda = 0
    sm.clocks(10)
    cs.value=0

def read(address, num):
    address = address & 0x3f
    cs.value=1
    sm.write(bytes([0x01, 0x80 | address]))
    data = sm.read(num)
    cs.value=0
    
    return data

def write(address, data):
    
    if len(data) % 2 == 1:
        print("Data length must be even")
        exit(2)

    for i in range(0, len(data) , 2):
        cs.value=1
        sm.write(bytes([0x01, 0x40 | address+i//2]) + data[i:i+2])
        cs.value=0

if __name__ == '__main__':
    sm=pyHydrabus.RawWire('/dev/ttyACM0')
    sm.gpio_mode = 1
    sm.set_speed(50000)
    sm.wires = 3
    
    # CS pin configuration with auxiliary pin PC4
    cs = sm.AUX[0]
    cs.direction=0
    cs.value=0
    
    # Read memory cotent
    print(hexlify(read(0, 128)))
    
    # Write 4 bytes of data
    write_enable()
    data = b"\xa5\xa6\x45\x46"
    write(0,data)
    write_disable()

    # Read memory cotent
    print(hexlify(read(0, 128)))
