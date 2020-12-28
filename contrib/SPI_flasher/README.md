Script to dump/program SPI flash. Tested with HydraBus FW version 0.10

Based on https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide

Usage:
```
usage: HydraSPI.py [-h] [-hsp HYDRA_SERIAL_PORT] [-hss HYDRA_SERIAL_SPEED] [-bs BLOCK_SIZE] [-spi {SPI1,SPI2}] [-ss SPI_SPEED]
                   [-sp {0,1}] [-scph {0,1}]
                   {chip-id,dump,program} ...

positional arguments:
  {chip-id,dump,program}
    chip-id             Get manufacturer chip id
    dump                Dump flash into file
    program             Program binary img into flash

optional arguments:
  -h, --help            show this help message and exit
  -hsp HYDRA_SERIAL_PORT, --hydra-serial-port HYDRA_SERIAL_PORT
                        Hydra COM port") (default: COM4)
  -hss HYDRA_SERIAL_SPEED, --hydra-serial-speed HYDRA_SERIAL_SPEED
                        Hydra COM port speed (default: 115200)
  -bs BLOCK_SIZE, --block-size BLOCK_SIZE
                        Max size of data block for SPI, value should be in range (0, 4096] (default: 4096)
  -spi {SPI1,SPI2}, --spi-port {SPI1,SPI2}
                        SPI port (default: SPI1)
  -ss SPI_SPEED, --spi-speed SPI_SPEED
                        SPI speed ( SPI1: 320k,650k,1.31m,2.62m,5.25m,10.5m,21m,42m SPI2: 160k,320k,650k,1.31m,2.62m,5.25m,10.5m,21m
                        ) (default: 21m)
  -sp {0,1}, --spi-polarity {0,1}
                        SPI polarity (default: 0)
  -scph {0,1}, --spi-clock-phase {0,1}
                        SPI clock phase (default: 0)
```

Examples:
- get manufacturer chip di:
    ```
    HydraSPI.py chip-id
    ```
- dump flash:
    ```
    HydraSPI.py dump test.bin 60 0x0
    ```
- program flash:
    ```
    # -ebw (or --erase_before_program) flag is used
    HydraSPI.py program u-boot.bin 0x0 -ebw
    ```

Requirements:
1) python3
2) py3 modules: `python3 -m pip install -U pyserial hexdump`

Author: MrKalach [https://github.com/MrKalach]

License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)

(if License conflicts with hydrafw license, hydrafw license prevails)
