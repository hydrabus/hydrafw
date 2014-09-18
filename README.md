HydraFW official firmware for HydraBus/HydraNFC
========

HydraFW is a native C (and asm) open source firmware for HydraBus board with support of HydraNFC Shield.

![HydraBus+HydraNFC board](HydraBus_HydraNFC_board.jpg)

For windows users, how to flash hydrafw firmware:

    1) PowerOff HydraBus board (disconnect all USB)
    2) Connect HydraBus pin BOOT0 to 3V3 (using a dual female splittable jumper wire) to enter USB DFU
    3) Connect MicroUsb cable from PC to HydraBus USB1 board, now windows shall detect a new device
       (if you have problem to detect the device use an USB1.1 or 2.0 Hub
          as there is problem with USB3.0 port on some computer and windows version).
    4) Download the USB DFU driver from directory https://github.com/bvernoux/hydrabus/firmware/STM32F4_USB_DFU_Driver.zip
     4-1) Extract the archive and install the drivers.
    5) Launch from current directory update_fw_usb_dfu_hydrabus.bat (will flash latest firmware during about 10s)
    6) Disconnect MicroUsb cable from HydraBus and Disconnect "BOOT0 to 3V3"
    7) Reconnect MicroUsb cable on USB1 or USB2(both port are supported), Now hydrafw is started.

I recommend Putty for terminal to be used with HydraBus connected with USB1 or 2
When connected type h for help and use TAB key for completion,
you can also use arrow up or down for history.
mount, umount, ls, cat & erase work only when a micro SD card is inserted in HydraBus.

HydraFW supported commands in v0.1Beta for HydraBus:
? or h         - Help
clear          - clear screen
info           - info on FW & HW
ch_mem         - memory info
ch_threads     - threads
ch_test        - chibios tests
mount          - mount sd
umount         - unmount sd
ls [opt dir]   - list files in sd
cat <filename> - display sd file (ASCII)
erase          - erase sd
Those commands are very basic and more will come later to read/write on I2C, SPI, UART...

HydraFW supported commands in v0.1Beta for HydraBus+HydraNFC(HydraNFC detected):
? or h         - Help
clear          - clear screen
info           - info on FW & HW
ch_mem         - memory info
ch_threads     - threads
ch_test        - chibios tests
mount          - mount sd
umount         - unmount sd
ls [opt dir]   - list files in sd
cat <filename> - display sd file (ASCII)
erase          - erase sd
nfc_mifare     - NFC read Mifare/ISO14443A UID
nfc_vicinity   - NFC read Vicinity UID
nfc_dump       - NFC dump registers
nfc_sniff      - NFC start sniffer ISO14443A
nfc_sniff can be started by K3 and stopped by K4 buttons

The firmware is set up for compilation with the GCC toolchain GNU_ARM_4_7_2013q3 available here:
https://launchpad.net/gcc-arm-embedded/+milestone/4.7-2013-q3-update

Required dependencies:
Custom chibios (based on ChibiOS 3.0) included as Submodule.
Custom microrl included as Submodule.

To build hydrafw firmware:
    $ cd in root directory (which contains directories common, fatfs, hydrabus, hydranfc ...)
    $ make clean
    $ make
