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
import pyHydrabus

from binascii import hexlify

BBIO_SMARTCARD = 0b00001011
BBIO_SMARTCARD_CONFIG = 0b10000000

DEV_CONVENTION_INVERSE = 0x00
DEV_CONVENTION_DIRECT = 0x01

speed_dict = {640:0, 1200:1, 2:2400, 4800:3, 9600:4, 19200:5, 31250:6, 38400:7, 57600:8, 115200:10}

Fi = [372, 372, 558, 744, 1116, 1488, 1860, "RFU", "RFU", 512, 768, 1024, 1536, 2048, "RFU", "RFU"];
Di = ["RFU", 1, 2, 4, 8, 16, 32, 64, 12, 20, "RFU", "RFU", "RFU", "RFU", "RFU", "RFU"];

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

def smartcard_crc(data):
    crc = 0
    for i in data:
        crc ^= i
    return bytes([crc])

def parse_atr(data):
    """
    Basic smartcard answer to reset (ATR) parser.

    :example:

    >>> atr = bytes([0x3B, 0x04, 0x92, 0x23, 0x10, 0x91])
    >>> convention = parse_atr(atr)
    Parsing ATR:
    TS:  0x3b, direct convention
    T0:  0x4
    TD1: absent, protocol T=0
    TA2: absent, card in negotiable mode.
    Historical bytes: 92231091
    >>>

    """
    print("Parsing ATR:")
    if len(data) < 2:
        return 0

    if data[0] == 0x3b:
        print("TS:  " + hex(data[0]) + ", direct convention")
        convention = DEV_CONVENTION_DIRECT
    elif data[0] == 0x3f:
        print("TS:  " + hex(data[0]) + ", inverse convention")
        convention = DEV_CONVENTION_INVERSE
    else:
        print("TS:  " + hex(data[0]) + ", Non standard ATR")
        return DEV_CONVENTION_DIRECT

    print("T0:  " + hex(data[1]))
    y1 = data[1] >> 4
    y = 0
    k = data[1] & 0xf
    i = 1

    if y1 & 1:
        i += 1
        F = Fi[data[i] >> 4];
        D = Di[data[i] & 0x0F];
        E = F // D;
        print("TA1: " + hex(data[i]) + ", Fi={:d}, Di={:d}, {:d} cycles/ETU".format(F, D, E))
    if y1 & 2:
        i += 1
        print("TB1: " + hex(data[i]))
    if y1 & 4:
        i += 1
        print("TC1: " + hex(data[i]) + ", extra guard time integer N={:d}".format(data[i]))
    if y1 & 8:
        i += 1
        print("TD1: " + hex(data[i]) + ", protocol T={:d}".format(data[i] & 0xf))
        y = data[i] >> 4
    else:
        print("TD1: absent, protocol T=0")
        protocol = 0
        y = 0
        
    if y & 1:
        i += 1
        print("TA2: " + hex(data[i]) + ", card in specific mode")
    else:
        print("TA2: absent, card in negotiable mode.")

    if y & 2:
        i += 1
        print("TB2: " + hex(data[i]))
    if y & 4:
        i += 1
        print("TC2: " + hex(data[i]))
    if y & 8:
        i += 1
        protocol = data[i] & 0xf
        print("TD2: " + hex(data[i]) + ", protocol T={:d}".format(protocol))
        y = data[i] >> 4
    else:
        y = 0
    
    ind = 3
    while y > 0:
        if y & 1:
            i += 1
            if protocol == 15:
                param = ", X={:d}, Y={:d}".format(data[i] >> 6, data[i] & 0x3f)
            else:
                param = ""
            print("TA" + str(ind) + ": " + hex(data[i]) + param)
        if y & 2:
            i += 1
            print("TB" + str(ind) + ": " + hex(data[i]))
        if y & 4:
            i += 1
            print("TC" + str(ind) + ": " + hex(data[i]))
        if y & 8:
            i += 1
            protocol = data[i] & 0xf
            print("TD" + str(ind) + ": " + hex(data[i]) + ", protocol T={:d}".format(protocol))
            y = data[i] >> 4
            ind += 1
        else:
            y = 0

    if k > 0:
        i+=1
        print("Historical bytes: " + hexlify(data[i:i+k]).decode())
    
    if protocol != 0 or data[i+k:] != b"":
        print("TCK: " + hex(data[i+k]))
    
    return convention

def smartcard_t1(nad, pcb, data, rcv_length):
    cmd = bytes([nad, pcb, len(data)]) + data
    cmd += smartcard_crc(cmd)
    
    print("Sending:   " + hexlify(cmd).decode())
    rsp = scard.write_read(cmd, rcv_length)
    print("Receiving: " + hexlify(rsp).decode())
    
def smartcard_send_pps(F, D, T=0):
    ppss = b"\xff"
    pps0 = bytes([0x10 | T])
    pps1 = bytes([Fi.index(F) << 4 | Di.index(D)])
    pps = ppss + pps0 + pps1
    pps = pps + smartcard_crc(pps)
    print("Sending PPS: " + hexlify(pps).decode())
    data = scard.write_read(pps, len(pps))
    print("Reponse PPS: " + hexlify(data).decode())

if __name__ == '__main__':
    scard = pyHydrabus.Smartcard('/dev/ttyACM0')
    atr = scard.get_atr()
    print("ATR: " + hexlify(atr).decode())
    convention = parse_atr(atr)
    scard.convention = convention
    scard.close()
    sys.exit()