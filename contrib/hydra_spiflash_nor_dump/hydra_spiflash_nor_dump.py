#!/usr/bin/python3

# Script to dump SPI flash chips with Hydrabus, tested with Hydrabus hardware v1.0 and firmware v0.8 beta
# Based on https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide
#
# Author: Pedro Ribeiro <pedrib@gmail.com>
# Author: Jonathan Borgeaud <dummys1337@gmail.com>
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#
import sys
import argparse
import serial

import signal

SECTORE_SIZE = 0x1000  # max buffer supported by hydrabus


def signal_handler(signal, frame):
    print("CTRL+C pressed, cleaning up and exiting")


class HydrabusSpiFlash:

    def __init__(self, serial_port):
        parser = argparse.ArgumentParser(
            description='Tool to query NOR memory flash with Hydrabus',
            epilog='This script requires python 3.7+, and serial',
            usage='''hydra_spiflash_nor_dump.py <command> [<args>]

    Commands are:
    get_chip_id                                     Return the chip identification
    dump <dump_file> <n_4k_sectors> <hex_address>   Dump the flash in <dump_file> starting at <hex_address>

            ''')

        parser.add_argument('command',
                            help='dump to dump the flash or get_chip_id')
        args = parser.parse_args(sys.argv[1:2])
        if not hasattr(self, args.command):
            print('Unrecognized command')
            parser.print_help()
            sys.exit(1)

        self.serial_port = serial_port
        self.hydrabus = None
        self.setup()

        # use dispatch pattern to invoke method with same name
        getattr(self, args.command)()

    def error(self, message):
        print(message)
        self.cleanup()
        sys.exit(1)

    def hex_to_bin(self, num, padding):
        return num.to_bytes(padding, byteorder='big')

    def calc_hex_addr(self, addr, add, addr_len):
        byte_arr = self.hex_to_bin(addr + add, addr_len)
        return byte_arr

    def setup(self):

        # Open serial port
        self.hydrabus = serial.Serial(self.serial_port, 115200)

        # Open binary mode
        for _ in range(20):
            self.hydrabus.write(b'\x00')
        if b"BBIO1" not in self.hydrabus.read(5):
            self.error(
                "Could not get into binary mode, try again or reset hydrabus.")

        self.hydrabus.reset_input_buffer()

        # Switching to SPI mode
        self.hydrabus.write(b'\x01')
        if b"SPI1" not in self.hydrabus.read(4):
            self.error("Cannot set SPI mode, try again or reset hydrabus.")

        # Configure SPI port (default polarity and clock phase, SPI1 device)
        self.hydrabus.write(b'\x81')
        if b'\x01' not in self.hydrabus.read(1):
            self.error(
                "Cannot set SPI device settings, try again or reset hydrabus.")

    def cleanup(self):

        # Return to main binary mode
        self.hydrabus.write(b'\x00')

        # reset to console mode
        self.hydrabus.write(b'\x0F\n')

    def get_chip_id(self):

        print("Sending RDID command...")
        # write-then-read: write one byte, read 3 (rdid)
        self.hydrabus.write(b'\x04\x00\x01\x00\x03')

        # send rdid byte (0x9f)
        self.hydrabus.write(b'\x9f')

        if b'\x01' not in self.hydrabus.read(1):
            self.error('Oups something went wrong...')

        print(f"Chip ID: 0x{self.hydrabus.read(3).hex()}")
        print("Finished get_chip_id function.")
        self.cleanup()
        sys.exit(0)

    def dump(self):
        parser = argparse.ArgumentParser(description='Dump the NOR flash')

        parser.add_argument('dump_file')
        parser.add_argument('n_4k_sectors', type=int)
        parser.add_argument('hex_address', help='Starting address')
        parser.add_argument('--speed',
                            action='store',
                            help='Slow(320Khz) or fast(10.5Mhz)')
        args = parser.parse_args(sys.argv[2:])
        if '0x' in args.hex_address:
            # we get hexadecimal address, need convert
            start_address = int(args.hex_address, 16)
        else:
            start_address = int(args.hex_address)

        if args.speed == 'slow':
            # Set spi speed to 320 Khz
            self.hydrabus.write(b'\x60')
        else:
            # Set spi speed to 10.5 mHz, a conservative fast speed
            self.hydrabus.write(b'\x61')
            print("Using fast mode (10.5 mHz) to dump the chip.")
        if b'\x01' not in self.hydrabus.read(1):
            self.error("Cannot set SPI speed, try again or reset hydrabus.")

        print('Starting to read chip...')
        print('Reading ' + str(args.n_4k_sectors) + ' sectors')

        sector = 0

        with open(args.dump_file, 'wb+') as dst:

            print(f"sectors: {args.n_4k_sectors}: {SECTORE_SIZE}")
            if args.n_4k_sectors * SECTORE_SIZE > 0xffffff:
                print("Size is bigger than 3bytes address, using 0x13 command")
                # size is bigger than 3bytes address so use the 0x13 command
                while sector < args.n_4k_sectors:
                    # write-then-read: write 5 bytes
                    # (1 read cmd + 4 read addr), read SECTORE_SIZE bytes
                    self.hydrabus.write(b'\x04\x00\x05' +
                                        self.hex_to_bin(SECTORE_SIZE, 2))

                    # read 4bytes address command (\x13) and address
                    self.hydrabus.write(b'\x13' + self.calc_hex_addr(
                        start_address, sector * SECTORE_SIZE, 4))

                    # Hydrabus will send \x01 in case of success...
                    if b'\x01' not in self.hydrabus.read(1):
                        self.error("Cannot read at address...")

                    # ...followed by SECTORE_SIZE read bytes
                    dst.write(self.hydrabus.read(SECTORE_SIZE))
                    print('Read sector ' + str(sector))
                    sector += 1
            else:
                # the size is less than 3bytes address so use the 0x03 command
                while sector < args.n_4k_sectors:
                    # write-then-read: write 4 bytes
                    # (1 read cmd + 3 read addr), read SECTORE_SIZE bytes
                    self.hydrabus.write(b'\x04\x00\x04' +
                                        self.hex_to_bin(SECTORE_SIZE, 2))

                    # read 4bytes address command (\x03) and address
                    self.hydrabus.write(b'\x03' + self.calc_hex_addr(
                        start_address, sector * SECTORE_SIZE, 3))

                    # Hydrabus will send \x01 in case of success...
                    if b'\x01' not in self.hydrabus.read(1):
                        self.error("Cannot read at address...")

                    # ...followed by SECTORE_SIZE read bytes
                    dst.write(self.hydrabus.read(SECTORE_SIZE))
                    print('Read sector ' + str(sector))
                    sector += 1

        print('Finished dumping to ' + args.dump_file)
        self.cleanup()


if __name__ == '__main__':

    # disable this for debugging
    # signal.signal(signal.SIGINT, signal_handler)

    hydra = HydrabusSpiFlash("/dev/ttyACM0")
