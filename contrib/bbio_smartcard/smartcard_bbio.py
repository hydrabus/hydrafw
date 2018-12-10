#!/usr/bin/env python3
#
# Script to communicate with ISO 7816-3 smartcard with Hydrabus.
#
# Author: Sylvain Pelissier <sylvain.pelissier@gmail.com>
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#
import sys
import serial
import time

from binascii import hexlify

BBIO_SMARTCARD = 0b00001011
BBIO_SMARTCARD_RST_LOW = 0b00000010
BBIO_SMARTCARD_RST_HIGH = 0b00000011
BBIO_SMARTCARD_WRITE_READ = 0b00000100
BBIO_SMARTCARD_PRESCALER = 0b00000110
BBIO_SMARTCARD_GUARDTIME = 0b00000111
BBIO_SMARTCARD_SET_SPEED = 0b01100000
BBIO_SMARTCARD_CONFIG = 0b10000000

DEV_CONVENTION_INVERSE = 0x00
DEV_CONVENTION_DIRECT = 0x01

speed_dict = {640:0, 1200:1, 2:2400, 4800:3, 9600:4, 19200:5, 31250:6, 38400:7, 57600:8, 115200:10}

hydrabus = None

Fi = [372, 372, 558, 744, 1116, 1488, 1860, "RFU", "RFU", 512, 768, 1024, 1536, 2048, "RFU", "RFU"];
Di = ["RFU", 1, 2, 4, 8, 16, 32, 64, 12, 20, "RFU", "RFU", "RFU", "RFU", "RFU", "RFU"];

def error(msg):
    print(msg)
    
    global hydrabus
    
    if hydrabus != None:
        # cleanup the hydrabus so that we don't have to reset it
        hydrabus_cleanup
        
    quit()

def hydrabus_cleanup():
    global hydrabus
    
    #Return to main binary mode
    hydrabus.write(b'\x00')

    #Reset to console mode
    hydrabus.write(b'\x0F\n')

def hydrabus_setup():
    global hydrabus
    
    #Serial port
    hydrabus = serial.Serial('/dev/ttyACM0', 115200, timeout=0.5)

    #Binary mode
    for i in range(20):
        hydrabus.write(b'\x00')
        
    rsp = hydrabus.read(5)
    if b"BBIO1" not in rsp:
        error("Could not get into binary mode, try again or reset hydrabus.")
    
    hydrabus.readline()
    hydrabus.reset_input_buffer()

    #Smartcard mode
    hydrabus.write(bytes([BBIO_SMARTCARD]))
    rsp = hydrabus.read(4)

    if b"CRD1" not in rsp:
        error("Cannot set smartcard mode, try again or reset hydrabus.")

def smartcard_set_speed(speed):
    hydrabus.write(bytes([BBIO_SMARTCARD_SET_SPEED, (speed >> 24) & 0xff, (speed >> 16) & 0xff, (speed >> 8) & 0xff, speed & 0xff]))
    rsp = hydrabus.read(1)
    if b"\x01" not in rsp:
        error("Cannot set reset low, try again or reset hydrabus.")

def smartcard_set_parity(parity):

    if parity == "even":
        hydrabus.write(bytes([BBIO_SMARTCARD_CONFIG | 0b010]))
    else:
        hydrabus.write(bytes([BBIO_SMARTCARD_CONFIG | 0b110]))
    
    rsp = hydrabus.read(1)
    if b"\x01" not in rsp:
        hydrabus_cleanup()
        error("Cannot change parity, try again or reset hydrabus.")
        
def smartcard_reverse(value):
    value = (value & 0xcc) >> 2 | (value & 0x33) << 2
    value = (value & 0xaa) >> 1 | (value & 0x55) << 1
    value = (value >> 4 | value << 4) & 0xff
    return value

def smartcard_apply_convention(data, convention):
    result = b""
    if convention == DEV_CONVENTION_INVERSE:
        for i in range(len(data)):
            result += bytes([smartcard_reverse(data[i] ^ 0xff)])
    else:
        result = data
        
    return result   

def smartcard_write_read(data, size, convention):
    hydrabus.write(bytes([BBIO_SMARTCARD_WRITE_READ, len(data) >> 4, len(data) & 0xff, size >> 4, size & 0xff]))
    hydrabus.write(smartcard_apply_convention(data, convention))
    rsp = hydrabus.read(1)
    if b"\x01" not in rsp:
        hydrabus_cleanup()
        error("Cannot write or read data, try again or reset hydrabus.")
    
    data = smartcard_apply_convention(hydrabus.read(size), convention)
    return data

def smartcard_read(size, convention):
    hydrabus.write(bytes([BBIO_SMARTCARD_WRITE_READ, 0, 0, size >> 4, size & 0xff]))
    
    rsp = hydrabus.read(1)
    if b"\x01" not in rsp:
        hydrabus_cleanup()
        error("Cannot read data, try again or reset hydrabus.")

    data = smartcard_apply_convention(hydrabus.read(size), convention)
    return data

def smartcard_crc(data):
    crc = 0
    for i in data:
        crc ^= i
    return bytes([crc])

def smartcard_warmreset():
    hydrabus.write(bytes([BBIO_SMARTCARD_RST_HIGH]))
    rsp = hydrabus.read(1)

    smartcard_set_speed(9600)
    
    if b"\x01" not in rsp:
        error("Cannot set reset low, try again or reset hydrabus.")

    hydrabus.write(bytes([BBIO_SMARTCARD_RST_LOW]))
    rsp = hydrabus.read(1)
    if b"\x01" not in rsp:
        error("Cannot set reset high, try again or reset hydrabus.")
    time.sleep(0.2)
    hydrabus.reset_input_buffer()

    hydrabus.write(bytes([BBIO_SMARTCARD_RST_HIGH]))
    rsp = hydrabus.read(1)
    if b"\x01" not in rsp:
        error("Cannot set reset low, try again or reset hydrabus.")

def smartcard_parse_atr():
    print("Parsing ATR:")
    convention = DEV_CONVENTION_DIRECT
    data = smartcard_read(1, convention)
    
    if data[0] == 0x03:
        print("Convention inverse: " + hex(data[0]))
        convention = DEV_CONVENTION_INVERSE
        #smartcard_set_parity(convention)
    elif data[0] == 0x3b:
        print("Convention direct: " + hex(data[0]))
        convention = DEV_CONVENTION_DIRECT
    else:
        error("Non standard convention: " + hex(data[0]))
    
    data = smartcard_read(1, convention)
    
    # We may have a reading problem with the first byte
    if data[0] == 0x3b:
        data = smartcard_read(1, convention)
        
    print("T0:  " + hex(data[0]))
    y1 = data[0] >> 4
    y = 0
    k = data[0] & 0xf
    
    if y1 & 1:
        data = smartcard_read(1, convention)
        F = Fi[data[0] >> 4];
        D = Di[data[0] & 0x0F];
        E = F // D;
        print("TA1: " + hex(data[0]) + ", Fi={:d}, Di={:d}, {:d} cycles/ETU".format(F, D, E))
    if y1 & 2:
        data = smartcard_read(1, convention)
        print("TB1: " + hex(data[0]))
    if y1 & 4:
        data = smartcard_read(1, convention)
        print("TC1: " + hex(data[0]) + ", extra guard time integer N={:d}".format(data[0]))
    if y1 & 8:
        data = smartcard_read(1, convention)
        print("TD1: " + hex(data[0]) + ", protocol T={:d}".format(data[0] & 0xf))
        y = data[0] >> 4
    else:
        y = 0
        
    if y & 1:
        data = smartcard_read(1, convention)
        print("TA2: " + hex(data[0]) + ", card in specific mode")
    else:
        print("TA2: absent, card in negotiable mode.")

    if y & 2:
        data = smartcard_read(1, convention)
        print("TB2: " + hex(data[0]))
    if y & 4:
        data = smartcard_read(1, convention)
        print("TC2: " + hex(data[0]))
    if y & 8:
        data = smartcard_read(1, convention)
        protocol = data[0] & 0xf
        print("TD2: " + hex(data[0]) + ", protocol T={:d}".format(protocol))
        y = data[0] >> 4
        i = 3
    else:
        y = 0
    
    while y > 0:
        if y & 1:
            data = smartcard_read(1, convention)
            if protocol == 15:
                param = ", X={:d}, Y={:d}".format(data[0] >> 6, data[0] & 0x3f)
            else:
                param = ""
            print("TA" + str(i) + ": " + hex(data[0]) + param)
        if y & 2:
            data = smartcard_read(1, convention)
            print("TB" + str(i) + ": " + hex(data[0]))
        if y & 4:
            data = smartcard_read(1, convention)
            print("TC" + str(i) + ": " + hex(data[0]))
        if y & 8:
            data = smartcard_read(1, convention)
            protocol = data[0] & 0xf
            print("TD" + str(i) + ": " + hex(data[0]) + ", protocol T={:d}".format(protocol))
            y = data[0] >> 4
            i += 1
        else:
            y = 0

    if k > 0:
        data = smartcard_read(k, convention)
        print("Historical bytes: " + hexlify(data).decode())
    
    if protocol != 0:
        data = smartcard_read(1, convention)
        print("TCK: " + hex(data[0]))
    
    return convention

def smartcard_t1(nad, pcb, data, rcv_length):
    cmd = bytes([nad, pcb, len(data)]) + data
    cmd += smartcard_crc(cmd)
    
    print("Sending:   " + hexlify(cmd).decode())
    rsp = smartcard_write_read(cmd, rcv_length, convention)
    print("Receiving: " + hexlify(rsp).decode())
    
def smartcard_send_pps(F, D, T, convention):
    ppss = b"\xff"
    pps0 = bytes([0x10 | T])
    pps1 = bytes([Fi.index(F) << 4 | Di.index(D)])
    pps = ppss + pps0 + pps1
    pps = pps + smartcard_crc(pps)
    print("Sending PPS: " + hexlify(pps).decode())
    data = smartcard_write_read(pps, len(pps), convention)
    print("Reponse PPS: " + hexlify(data).decode())

if __name__ == '__main__':
    hydrabus_setup()
    smartcard_warmreset()
    convention = smartcard_parse_atr()
    smartcard_set_parity("even")
    smartcard_send_pps(372, 2, 1, convention)
    smartcard_set_speed(18817)
    
    # IFS
    smartcard_t1(0, 0xc1, b"\xfe", 5)
    hydrabus_cleanup()
    sys.exit()
