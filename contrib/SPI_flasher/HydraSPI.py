#!/usr/bin/python3
#
# Script for dumping/programming SPI flash chips with Hydrabus.
# Based on HydraBus BBIO documentation: https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide
#
# Author: MrKalach [https://github.com/MrKalach]
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#

import hexdump
import serial
import sys
import argparse


class Params():
    serial_port     = 'COM4'
    serial_speed    = 115200

    block_size      = 0x1000      # max_buffer (HydraBus limitation)
    spi             = 1
    polarity        = 0
    clock_phase     = 0

    SPI_SPEED_TABLE = {
        "SPI1" : {
            "320k"  : 0b01100000,
            "650k"  : 0b01100001,
            "1.31m" : 0b01100010,
            "2.62m" : 0b01100011,
            "5.25m" : 0b01100100,
            "10.5m" : 0b01100101,
            "21m"   : 0b01100110,
            "42m"   : 0b01100111,
        },
        "SPI2" : {
            "160k"  : 0b01100000,
            "320k"  : 0b01100001,
            "650k"  : 0b01100010,
            "1.31m" : 0b01100011,
            "2.62m" : 0b01100100,
            "5.25m" : 0b01100101,
            "10.5m" : 0b01100110,
            "21m"   : 0b01100111,
        }
    }

    SPI_speed = 0b01100110

def check_result(result, expected, error_str):
    assert expected in result, f'{error_str}: expected={expected} != result={result}'


class SPI_hydra:
    def __init__(self, params):
        self.serial = None
        self.params = params

    def __enter__(self):
        self.serial = serial.Serial(self.params.serial_port, self.params.serial_speed)

        # Switching HydraBus to binary mode
        for _ in range(20):
            self.serial.write(b'\x00')
        check_result(b"BBIO1", self.serial.read(5), 'Could not switch into binary mode!')

        self.serial.reset_input_buffer()

        # Sitching to SPI mode
        self.write_bytes(0b00000001)
        check_result(b"SPI1", self.serial.read(4), 'Could not switch to SPI mode')

        # Configuring SPI
        cfg = 0b10000000 | self.params.polarity << 3 | self.params.clock_phase << 2 | self.params.spi
        self.write_bytes(cfg) #polarity => 0, phase => 0, SPI1
        check_result(b'\x01', self.serial.read(1),'Could not setup SPI port!')

        # Setting up SPI speed
        self.write_bytes(self.params.SPI_speed)
        check_result(b'\x01', self.serial.read(1), 'Could not setup SPI speed!')

        self.cs_off()

        return self


    def __exit__(self, type, value, traceback):
        # Switch HydraBus back to terminal mode
        self.serial.write(b'\x00')
        self.serial.write(b'\x0F')

    def write_bytes(self, data):
        if not isinstance(data, list) and not isinstance(data, bytes):
            data = [data]
        self.serial.write(data)

    def read(self, num):
        return self.serial.read(num)

    def cs_on(self):
        self.write_bytes(0b00000011)
        check_result(b'\x01', self.serial.read(1), 'Cannot switch CS to on!')

    def cs_off(self):
        self.write_bytes(0b00000010)
        check_result(b'\x01', self.serial.read(1), 'Cannot switch CS to off!')


##############################################################################################################################

def num_to_bytes(num, padding):
    return num.to_bytes(padding, byteorder='big')

def dump_chip_id(params):
    with SPI_hydra(params) as hydra:
        print("Getting chip manufacturer id ...")

        # 0b00000100 - write then read (with CS pinning)
        hydra.write_bytes([0b00000100, 0x00, 0x01, 0x00, 0x03])
        # send RDID command
        hydra.write_bytes(0x9f)

        check_result(b'\x01', hydra.read(1), 'Error occuried while getting chip id!')
        result = hydra.read(3)

        print(f'Chip manufacturer id: {hexdump.dump(result)}')

def dump_flash(params, filename, blocks, start_addr_hex):
    start_addr = int(start_addr_hex, 16)

    with SPI_hydra(params) as hydra:
        print('Start dumping ...')
        print(f'Reading blocks: ', end='')

        block = 0
        buf = bytearray()

        while block < blocks:
            # write-then-read: \x00\x04 - input size
            hydra.write_bytes(b'\x04\x00\x04' + num_to_bytes(params.block_size, 2))

            # read command (\x03) and 3 bytes of address
            hydra.write_bytes(b'\x03' + num_to_bytes(start_addr, 3))

            check_result(b'\x01', hydra.read(1), 'Error occurred while dumping!')

            buf += hydra.read(params.block_size)

            if (block % 10) == 0:
                print(block, end='')
            else:
                print('.', end='')

            block += 1
            start_addr += params.block_size

        with open(filename, 'wb+') as f:
            f.write(buf)

        print('\nDone')

# This code has been proofed && worked with Macronix MX25L12845E chip
# But somekind of generic commands is used, so it should work for most of other chips
def program_flash(params, filename, start_addr_hex, erase_before_program):
    MAX_ATTEMPTS = 30

    def enable_write():
        # Enabling write
        attempts = 0
        status = 0
        while status & 0b11 != 0b10:
            # WREN
            hydra.write_bytes(b'\x04\x00\x01\x00\x00')
            hydra.write_bytes(0x06)
            check_result(b'\x01', hydra.read(1), 'Error occurred while sending WREN!')

            # RDSR
            hydra.write_bytes(b'\x04\x00\x01\x00\x01')
            hydra.write_bytes(0x05)
            check_result(b'\x01', hydra.read(1), 'Error occurred while sending RDSR!')

            status = hydra.read(1)[0]

            if status & (1 << 7):
                assert 'Chip has a write protection!'

            attempts += 1
            if attempts > MAX_ATTEMPTS:
                assert 'Cannot set WREN!'

    def check_fail():
        # RDSCUR to check P_FAIL and E_FAIL
        hydra.write_bytes(b'\x04\x00\x01\x00\x01')
        hydra.write_bytes(0x28)
        check_result(b'\x01', hydra.read(1), 'Error occurred while sending RDSCUR!')

        status = hydra.read(1)[0]
        if status & 0x01100000 != 0: # check P_FAIL && E_FAIL
            # CLSR - clear SR Fail Register
            hydra.write_bytes(b'\x04\x00\x01\x00\x01')
            hydra.write_bytes(0x30)
            check_result(b'\x01', hydra.read(1), 'Error occurred while sending RDSCUR!')

            assert f'P_FAIL or E_FAIL: register={bin(status)}'

    def erase_sector_4k(addr):
        enable_write()

        # Erase sector/block
        # SE - sector erase, 0x20 + 3 bytes of address
        hydra.write_bytes(b'\x04\x00\x04\x00\x00')
        hydra.write_bytes(b'\x20' + num_to_bytes(start_addr, 3)) # CE - chip erase, can be used here
        check_result(b'\x01', hydra.read(1), 'Error occurred while sending SE!')

        # Check status
        status = 1
        attempts = 0
        while status & 0b1 != 0: #write in progress bit check
            # RDSR
            hydra.write_bytes(b'\x04\x00\x01\x00\x01')
            hydra.write_bytes(0x05)
            check_result(b'\x01', hydra.read(1), 'Error occurred while sending RDSR!')

            status = hydra.read(1)[0]

            attempts += 1
            if attempts > MAX_ATTEMPTS:
                assert 'Cannot read status RDSR!'

        check_fail()

    def write_page(page, data):
        if len(data) != 0x100:
            data = data + b'\x00' * (0x100 - len(data))
        assert len(data) == 0x100, f'Somthing goes wrong, data size is not 0x100 => {len(data)}'

        enable_write()

        # Program data
        # PP - page program, 0x02 + 3 bytes of address + data
        # low byte of address is always 0x00
        hydra.write_bytes(b'\x04\x01\x04\x00\x00')
        hydra.write_bytes(b'\x02' + num_to_bytes(page, 2) + b'\x00')
        hydra.write_bytes(data)
        check_result(b'\x01', hydra.read(1), 'Error occurred while sending PP!')

        # Check status
        status = 1
        attempts = 0
        while status & 0b1 != 0: #write in progress bit checking
            # RDSR
            hydra.write_bytes(b'\x04\x00\x01\x00\x01')
            hydra.write_bytes(0x05)
            check_result(b'\x01', hydra.read(1), 'Error occurred while sending RDSR!')

            status = hydra.read(1)[0]

            attempts += 1
            if attempts > MAX_ATTEMPTS:
                assert 'Cannot read status RDSR!'

        check_fail()

    start_addr = int(start_addr_hex, 16)
    assert start_addr & 0x1FF == 0, f'In program mode start address has to be aligned (0x1FF bytes boundary). start address = {start_addr}'

    print('Starting programming...')
    print('Blocks:', end='')
    page = 0
    with SPI_hydra(params) as hydra:
        with open(filename, 'rb') as f:
            while True:
                data = f.read(0x100)
                if len(data) == 0:
                    break

                if page % 0xF == 0:
                    if erase_before_program:
                        erase_sector_4k(start_addr)
                    start_addr += 0x1000
                    block_number = int(start_addr / 0x1000)
                    if block_number % 10 == 0:
                        print(block_number, end='')
                    else:
                        print('.', end='')

                write_page(page, data)
                page += 1

    print('\nDone.')


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '-hsp', '--hydra-serial-port',
        type=str, default=Params.serial_port,
        help='Hydra COM port (eg. COM4 or /dev/ttyS0)'
    )
    parser.add_argument(
        '-hss', '--hydra-serial-speed',
        type=int, default=Params.serial_speed,
        help='Hydra COM port speed'
    )
    parser.add_argument(
        '-bs', '--block-size',
        type=int, default=Params.block_size,
        help='Max size of data block for SPI, value should be in range (0, 4096]'
    )

    parser.add_argument(
        '-spi', '--spi-port',
        type=str, default='SPI2' if Params.spi == 0 else 'SPI1',
        help='SPI port',
        choices=['SPI1', 'SPI2']
    )

    parser.add_argument(
        '-ss', '--spi-speed',
        type=str, default='21m',
        help=f'''
            SPI speed (
                SPI1: {",".join(Params.SPI_SPEED_TABLE["SPI1"].keys())}
                SPI2: {",".join(Params.SPI_SPEED_TABLE["SPI2"].keys())}
            )
        ''',
    )

    parser.add_argument(
        '-sp', '--spi-polarity',
        type=int, default=Params.polarity,
        help=f'SPI polarity',
        choices=[0, 1]
    )

    parser.add_argument(
        '-scph', '--spi-clock-phase',
        type=int, default=Params.clock_phase,
        help=f'SPI clock phase',
        choices=[0, 1]
    )


    subparsers = parser.add_subparsers(dest='action', required=True)

    subparsers.add_parser('chip-id', help='Get manufacturer chip id')

    dump_parser = subparsers.add_parser('dump', help='Dump flash into file')
    dump_parser.add_argument('filename', type=str, help='File to store dump')
    dump_parser.add_argument('blocks', type=int, help='Blocks to read')
    dump_parser.add_argument('start_address', type=str, help='Start address (HEX)')

    burn_parser = subparsers.add_parser('program', help='Program img into flash')
    burn_parser.add_argument('filename', type=str, help='File to program')
    burn_parser.add_argument('start_address', type=str, help='Start address (HEX)')
    burn_parser.add_argument('-ebw', '--erase_before_program', action='store_true', help='Erase data before program')

    try:
        options = parser.parse_args()
        params = Params()
        params.serial_port = options.hydra_serial_port
        params.serial_speed = options.hydra_serial_speed
        if options.block_size < 1 or options.block_size > 0x1000:
            print(f'Invalid block size {options.block_size}! Value should be in range (1, 4096]')
            exit()
        params.block_size = options.block_size
        params.spi = {"SPI1" : 1, "SPI2" : 0}[options.spi_port]
        if options.spi_speed not in Params.SPI_SPEED_TABLE[options.spi_port]:
            print(f'Wrong speed({options.spi_speed}) for this({options.spi_port}) port!')
            exit()

        params.SPI_speed = Params.SPI_SPEED_TABLE[options.spi_port][options.spi_speed]
        params.polarity = options.spi_polarity
        params.clock_phase = options.spi_clock_phase

        if options.action == 'chip-id':
            dump_chip_id(params)
        elif options.action == 'dump':
            dump_flash(params, options.filename, options.blocks, options.start_address)
        elif options.action == 'program':
            program_flash(params, options.filename, options.start_address, options.erase_before_program)

    except SystemExit:
        print()
        parser.print_help()
        sys.exit(0)

if __name__ == '__main__':
    main()
