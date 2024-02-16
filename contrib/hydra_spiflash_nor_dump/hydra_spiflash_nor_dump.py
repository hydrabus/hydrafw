#!/usr/bin/python3

# Script to dump SPI flash chips with Hydrabus, tested with Hydrabus hardware v1.0 and firmware v0.8 beta
# Based on https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide
#
# Author: Pedro Ribeiro <pedrib@gmail.com>
# Author: Jonathan Borgeaud <dummys1337@gmail.com>
# Author: Benjamin Vernoux <bvernoux@hydrabus.com>
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#
import sys
import time
import argparse
import serial

import signal

SECTORE_SIZE = 0x1000  # max buffer supported by hydrabus


def signal_handler(signal, frame):
    print("CTRL+C pressed, cleaning up and exiting")


class HydrabusSpiFlash:

    def __init__(self):

        parser = argparse.ArgumentParser(
            description='Tool to query NOR memory flash with Hydrabus',
            epilog='This script requires python 3.7+, and serial',
            usage='''hydra_spiflash_nor_dump.py [options] <command> [<args>]
        options:
        --com_port (/dev/ttyACM0 by default)
        --spi (1 for SPI1 by default) or 2 for SPI2
        Commands are:
        get_chip_id                                   Return the chip identification
        dump <dump_file> <n_4k_sectors> <hex_address> Dump the flash in <dump_file> starting at <hex_address>
           [--speed slow](by default 320KHz) or [--speed fast](10.5MHz) or [--speed max](42MHz SPI1/21MHz SPI2)
        ''')

        parser.add_argument("--com_port",
                            default="/dev/ttyACM0",
                            help="Specify COM port (default: /dev/ttyACM0)")
        parser.add_argument("--spi",
                            type=int,
                            choices=[1, 2],
                            default=1,
                            help="Specify SPI value (1 or 2) (default: 1)")

        subparsers = parser.add_subparsers(dest="command",
                                           required=True,
                                           help="Choose command")

        dump_parser = subparsers.add_parser("dump", help="Dump command help")
        dump_parser.add_argument("dump_file",
                                 type=str,
                                 help="Specify dump file")
        dump_parser.add_argument("n_4k_sectors",
                                 type=int,
                                 help="Specify number of 4K sectors")
        dump_parser.add_argument("hex_address",
                                 type=str,
                                 help="Specify hex address")
        dump_parser.add_argument("--speed",
                                 choices=["slow", "fast", "max"],
                                 default="slow",
                                 help="Specify speed (default: slow)")

        subparsers.add_parser("get_chip_id", help="Get chip ID command help")
        self.args = parser.parse_args()

        self.setup()

        if self.args.command == "dump":
            print("Dumping with the following options:")
            print("COM Port:", self.args.com_port, "SPI:", self.args.spi,
                  "SPI Speed:", self.args.speed, "Dump File:",
                  self.args.dump_file, "Number of 4K Sectors:",
                  self.args.n_4k_sectors, "Hex Address:",
                  self.args.hex_address)
            self.dump()
        elif self.args.command == "get_chip_id":
            print("Getting chip ID with the following options:")
            print("COM Port:", self.args.com_port, "SPI:", self.args.spi)
            self.get_chip_id()
        self.cleanup()

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
        self.hydrabus = serial.Serial(self.args.com_port, 115200)

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

        if self.args.spi == 1:
            # Configure SPI port (default polarity and clock phase, SPI1 device)
            self.hydrabus.write(b'\x81')
        else:
            # Configure SPI port (default polarity and clock phase, SPI2 device)
            self.hydrabus.write(b'\x80')
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

        chip_id = self.hydrabus.read(3)
        if int.from_bytes(chip_id) == 0 or int.from_bytes(chip_id) == 0xFFFFFF:
            print(f"Chip ID: 0x{chip_id.hex()}")
            print('Cannot read get chip ID (invalid chip ID) exit')
            self.cleanup()
            sys.exit(-1)
        else:
            print(f"Chip ID: 0x{chip_id.hex()}")
            print("Finished get_chip_id function.")

    def dump(self):
        self.get_chip_id()
        if '0x' in self.args.hex_address:
            # we get hexadecimal address, need convert
            start_address = int(self.args.hex_address, 16)
        else:
            start_address = int(self.args.hex_address)

        if self.args.speed == 'slow':
            # Set spi speed 'slow' to 320kHz
            if self.args.spi == 1:
                self.hydrabus.write(b'\x60')
            else:  # SPI2
                self.hydrabus.write(b'\x61')
        elif self.args.speed == 'fast':
            # Set spi speed 'fast' to 10.5MHz
            if self.args.spi == 1:
                self.hydrabus.write(b'\x65')
            else:  # SPI2
                self.hydrabus.write(b'\x66')
        elif self.args.speed == 'max':
            # Set spi speed 'max' 42MHz SPI1 / 21MHz SPI2
            self.hydrabus.write(b'\x67')
        if b'\x01' not in self.hydrabus.read(1):
            self.error("Cannot set SPI speed, try again or reset hydrabus.")

        print('Starting to read chip...')
        print('Reading ' + str(self.args.n_4k_sectors) + ' sectors')

        sector = 0
        total_bytes_read = 0
        start_time = time.time()

        with open(self.args.dump_file, 'wb+') as dst:
            print(f"sectors: {self.args.n_4k_sectors}: {SECTORE_SIZE}")
            if self.args.n_4k_sectors * SECTORE_SIZE > 0xffffff:
                print("Size is bigger than 3bytes address, using 0x13 command")
                # size is bigger than 3bytes address so use the 0x13 command
                while sector < self.args.n_4k_sectors:
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
                    data = self.hydrabus.read(SECTORE_SIZE)
                    dst.write(data)
                    total_bytes_read += len(data)
                    print('Read sector ' + str(sector), end='\r')
                    sector += 1
            else:
                # the size is less than 3bytes address so use the 0x03 command
                while sector < self.args.n_4k_sectors:
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
                    data = self.hydrabus.read(SECTORE_SIZE)
                    dst.write(data)
                    total_bytes_read += len(data)
                    print('Read sector ' + str(sector), end='\r')
                    sector += 1

        end_time = time.time()
        elapsed_time = end_time - start_time

        # Calculate throughput in KBytes/s
        throughput_kbytes_persec = (total_bytes_read / elapsed_time) / 1024

        print('Finished dumping to ' + self.args.dump_file)
        print(f'Time taken: {elapsed_time:.2f} seconds')
        print(f'Throughput: {throughput_kbytes_persec:.2f} KBytes/s')
        self.cleanup()


if __name__ == '__main__':

    # disable this for debugging
    # signal.signal(signal.SIGINT, signal_handler)

    hydra = HydrabusSpiFlash()
