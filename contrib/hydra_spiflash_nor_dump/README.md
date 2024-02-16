Script to dump SPI flash chips with Hydrabus, tested with Hydrabus hardware v1.0 and firmware v0.8 beta or more

Based on https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide

Usage:
    hydra_spiflash_nor_dump.py [options] <command> [<args>]
    options:
    --com_port (/dev/ttyACM0 by default)
    --spi (1 for SPI1 by default) or 2 for SPI2
    command:
    hydra_spiflash_nor_dump.py dump <dump_file> <n_4k_sectors> <hex_address> [--speed slow|fast|max]
        Dumps n_4k_sectors into dump_file, starting at hex_address.
        By default, it dumps in slow (320kHz) mode, choose fast for 10.5 MHz or max for SPI speed 42MHz SPI1/21MHz SPI2.
    hydra_spiflash_nor_dump.py get_chip_id
        Prints chip idenfification (RDID).

Example Debian get_chip_id SPI1(default) with COM Port(default /dev/ttyACM0)
    python hydra_spiflash_nor_dump.py get_chip_id

Example Debian dump SPI1(default) with COM Port(default /dev/ttyACM0) 100 sectors of 4K (400K) speed default(slow)
    python hydra_spiflash_nor_dump.py dump dump.bin 100 0

Example Windows get_chip_id SPI2 with COM9
    python hydra_spiflash_nor_dump.py --com_port COM9 --spi 2 get_chip_id

Example Windows dump SPI2 with COM9 100 sectors of 4K (400K) speed fast
    python hydra_spiflash_nor_dump.py --com_port COM9 --spi 2 dump dump.bin 100 0 --speed fast

This script requires Python 3.7+, pip3 install pyserial

Author: Pedro Ribeiro <pedrib@gmail.com>
Author: Jonathan Borgeaud <dummys1337@gmail.com>
Author: Benjamin Vernoux <bvernoux@hydrabus.com>

License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)

(if License conflicts with hydrafw license, hydrafw license prevails)
