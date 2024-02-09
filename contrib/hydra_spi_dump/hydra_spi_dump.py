#!/usr/bin/python3
#
# Script to dump SPI flash chips with Hydrabus, tested with Hydrabus hardware v1.0 and firmware v0.8 beta
# Based on https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide
#
# Author: Pedro Ribeiro <pedrib@gmail.com>
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#
import hexdump
import serial
import struct
import sys
import signal

sector_size = 0x1000      # also the max buffer supported by hydrabus ? (CONFIRM THIS)
hydrabus = None

def error(msg):
    print(msg)
    
    global hydrabus
    
    if hydrabus != None:
        # cleanup the hydrabus so that we don't have to reset it
        hydrabus_cleanup
        
    quit()
    
def signal_handler(signal, frame):
        error("CTRL+C pressed, cleaning up and exiting")

def print_usage():
    print("Usage:")
    print("\thydra_spi_dump.py dump <dump_file> <n_4k_sectors> <hex_address> [slow|fast]")
    print("\t\tDumps n_4k_sectors into dump_file, starting at hex_address.")
    print("\t\tBy default, it dumps in slow (320kHz) mode, choose fast to increase to 10.5 mHz.")
    print("\n\thydra_spi_flash.py chip_id")
    print("\t\tPrints chip idenfification (RDID).")
    print("\nThis script requires Python 3.2+, pip3 install serial hexdump")
    quit()
    
def hex_to_bin(num, padding):
    return num.to_bytes(padding, byteorder='big')
    
def calc_hex_addr(addr, add):
    addr_int = int(addr, 16)
    addr_int += add
    byte_arr = hex_to_bin(addr_int, 4)
    return byte_arr
    
def hydrabus_setup():
    global hydrabus
    
    #Open serial port
    hydrabus = serial.Serial('/dev/ttyACM0', 115200)

    #Open binary mode
    for i in range(20):
        hydrabus.write(b'\x00')
    if b"BBIO1" not in hydrabus.read(5):
        error("Could not get into binary mode, try again or reset hydrabus.")
        
    hydrabus.reset_input_buffer()

    # Switching to SPI mode
    hydrabus.write(b'\x01')
    if b"SPI1" not in  hydrabus.read(4):
        error("Cannot set SPI mode, try again or reset hydrabus.")
        
    # Configure SPI port (default polarity and clock phase, SPI1 device)
    hydrabus.write(b'\x81')
    if b'\x01' not in hydrabus.read(1):
        error("Cannot set SPI device settings, try again or reset hydrabus.")
                
                
def hydrabus_cleanup():
    global hydrabus
    
    #Return to main binary mode
    hydrabus.write(b'\x00')

    #reset to console mode
    hydrabus.write(b'\x0F\n')

def dump_chip():
    if len(sys.argv) < 5:
        print_usage
    else:
        dump_file = sys.argv[2]
        sectors = int(sys.argv[3])
        start_addr = sys.argv[4]
        if sectors < 1:
            print_usage
            
    hydrabus_setup()
    global hydrabus
            
    if len(sys.argv) == 6 and sys.argv[5] == "fast":
        # Set spi speed to 10.5 mHz, a conservative fast speed
        hydrabus.write(b'\x61')
        if b'\x01' not in hydrabus.read(1):
            error("Cannot set SPI speed, try again or reset hydrabus.")
        else:
            print("Using fast mode (10.5 mHz) to dump the chip.")

    print('Starting to read chip...')
    print('Reading ' + str(sectors) + ' sectors')

    sector = 0
    buf = bytearray()
    
    while sector < sectors:
        # write-then-read: write 4 bytes (1 read cmd + 3 read addr), read sector_size bytes
        hydrabus.write(b'\x04\x00\x05' + hex_to_bin(sector_size, 2))

        # read 4bytes address command (\x13) and address
        hydrabus.write(b'\x13' + calc_hex_addr(start_addr, sector * sector_size))
        
        # Hydrabus will send \x01 in case of success...
        ok = hydrabus.read(1)
        
        #...followed by sector_size read bytes
        buf += hydrabus.read(sector_size)
        print('Read sector ' + str(sector))
        sector += 1

    with open(dump_file, 'wb+') as f:
        f.write(buf)
        
    print('Finished dumping to ' + dump_file)
    
def chip_id():
    global hydrabus
    
    buf = bytearray()
    
    print("Sending RDID command...")
    # write-then-read: write one byte, read 3 (rdid)
    hydrabus.write(b'\x04\x00\x01\x00\x03')
    
    # send rdid byte (0x9f)
    hydrabus.write(b'\x9f')
    
    ok = hydrabus.read(1)
    buf = hydrabus.read(3)
    
    print(hexdump.hexdump(buf))
    
    print("Finished chip_id function.")
    

if __name__ == '__main__':
    
    # disable this for debugging
    #signal.signal(signal.SIGINT, signal_handler)
    
    if len(sys.argv) < 2:
        print_usage()
        
    elif sys.argv[1] == "dump":
        # hydrabus_setup inside dump_chip because of arg validation
        dump_chip()
        hydrabus_cleanup()
        
    elif sys.argv[1] == "chip_id":
        hydrabus_setup()
        chip_id()
        hydrabus_cleanup()
        
    else:
        print_usage()
