Script to dump SPI flash chips with Hydrabus, tested with Hydrabus hardware v1.0 and firmware v0.8 beta

Based on https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide

Usage:

    hydra_spi_flash.py dump <dump_file> <n_4k_sectors> <hex_address> [slow|fast]
        Dumps n_4k_sectors into dump_file, starting at hex_address.
        By default, it dumps in slow (320kHz) mode, choose fast to increase to 10.5 mHz.
    hydra_spi_flash.py chip_id
        Prints chip idenfification (RDID).
    
    
This script requires Python 3.2+, pip3 install serial hexdump

Author: Pedro Ribeiro <pedrib@gmail.com>
License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)

(if License conflicts with hydrafw license, hydrafw license prevails)
