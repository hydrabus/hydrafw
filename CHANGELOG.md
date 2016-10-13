# CHANGELOG of 'hydrafw'
----------------------

#### 13.10.2016 - [HydraFW v0.8 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.8-beta)

##### Generic:
* Updated ChibiOS from actual 3.0 to latest [ChibiOS v16.1.6+/ChibiOS RT v4.0.0(Sep 27, 2016 )](https://github.com/bvernoux/ChibiOS/tree/86ea1b7f4c502e2b2a46a600b922aeb2d14d3fd7)
  * Added benchmark tests see hydrafw command `debug test-rx` and python script https://github.com/bvernoux/hydrafw/blob/master/scripts/tx_bench.py
* Updated [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) to chibios [fatfs-0.10b-patched.7z](https://github.com/bvernoux/ChibiOS/blob/86ea1b7f4c502e2b2a46a600b922aeb2d14d3fd7/ext/fatfs-0.10b-patched.7z)
* [Script support](https://github.com/bvernoux/hydrafw/wiki/HydraFW-console-commands#sd-scripting) (Thanks to [Baldanos](https://github.com/Baldanos))
 see commit [47f181a9](https://github.com/bvernoux/hydrafw/commit/47f181a9b8d8ed39f27322309dec532256e9fe77)
 * Added SD script (to execute commands from sdcard file)
 * Added SD startup script option (**initscript** file at root of sdcard)
* Fixed SDCard 4x slowness (was set implicitly to SDC_MODE_1BIT instead of SDC_MODE_4BIT in new chibios)
 * Now Read speed on Class 10 MicroSD reach 11MBytes/s (using 24MHz - 4Bits SDIO mode, can be increased later to 48MHz - 4 bits SDIO on microSD supporting it)
* tokenline
  *  [Fixed issue on history with up key](https://github.com/bvernoux/tokenline/commit/c011c5a1e77ba6a84ac460e0cf36546ed0d8d3bc) (Thanks to [0x8008135](https://github.com/0x8008135))
  *  [Manage invalid value for T_ARG_FLOAT & strict suffix only "k", "m" or "g" or no suffix for T_ARG_UINT & T_ARG_FLOAT](https://github.com/bvernoux/tokenline/commit/08cecc1a32f0413973c0043db0795e9561b0a782)
  *  show_help() fix coverity scan bug "Dereferencing null pointer tl->parsed.last_token_entry"
* [Updated drv/stm32cube to latest STM32Cube FW F4 V1.13.0](https://github.com/bvernoux/hydrafw/commit/e1705c045727ecfc223cac9c3dfdad26cd1efc48)
* HydraFW [Coverity Scan](https://scan.coverity.com) defects fixed (Thanks to [iceman1001](https://github.com/iceman1001))
* Fixed compatibility with python3 (always compatible with python 2.x) for [dfu-convert.py](https://github.com/bvernoux/hydrafw/commit/e7169070aa46fe5136ec71210be724b3f26bfa14) & [hydrafw-version.py](https://github.com/bvernoux/hydrafw/commit/f7b2296dad07330fd6dbaf1f6583ddbabcce38a1) (Thanks to [Baldanos](https://github.com/Baldanos))

##### HydraBus specific:
* Console mode
  * [Raw 3-wire](https://github.com/bvernoux/hydrafw/commit/a9681c5e39155f7ae1b6e1b2668950ef70b1223b) (Thanks to [Baldanos](https://github.com/Baldanos))
  * [UART Bridge enhancement](https://github.com/bvernoux/hydrafw/commit/81f925c54b42fe450e49802184bde9c365248dd1) (Thanks to [Baldanos](https://github.com/Baldanos))
      * The UART bridge now works perfectly at up to 115200 bauds
  * [Measure frequency & Duty Cycle](https://github.com/bvernoux/hydrafw/commit/510115b5a12865be579cc11a34f09dd8fb210089) (from 1282Hz to 84MHz, 128Hz to 8.4Mhz too, autorange ...)  (Thanks to [Baldanos](https://github.com/Baldanos))
  * [Hexdump in console](https://github.com/bvernoux/hydrafw/pull/58) command hd to display in hex / ascii data read for spi, i2c, uart, 2 and 3-wire (Thanks to [Baldanos](https://github.com/Baldanos))

* [Binary Mode](https://github.com/bvernoux/hydrafw/wiki/HydraFW-Binary-mode-guide)
  * [Raw 3-wire](https://github.com/bvernoux/hydrafw/commit/a9681c5e39155f7ae1b6e1b2668950ef70b1223b) (Thanks to [Baldanos](https://github.com/Baldanos))
      * Integrated in BBIO bbio_mode_rawwire which support now 2-wire and 3-wire
  * [SPI binary mode](https://github.com/bvernoux/hydrafw/commit/d67d98397d0502b7247c6f92569add35b0ad3390) Add configuration/selection of SPI1 or SPI2

##### HydraNFC specific:
* Added command sd (sdcard commands)
* Added Example [bbio_hydranfc_init.py](https://github.com/bvernoux/hydrafw/blob/master/scripts/bbio_hydranfc_init.py) for HydraNFC init using Console mode + switch to bbIO mode for SPI2 Init & communication with TRF7970A (HydraNFC shield)
* Read / Display / Save Mifare Ultra Light tag data (64bytes raw data of the Tag)
  * `read-mf-ul` command now requires a mandatory filename as destination to save the Mifare Ultra Light 64bytes data to microsd file
   * See commit [d569fcd8] 27-May-2016 (https://github.com/bvernoux/hydrafw/commit/d569fcd853415d7d56e54ef773315abb45015285) (modified scan command)
* Emulate Mifare Ultra Light tag (Beta version does not work with phone) (7Bytes UID and 64bytes data support only READ command)
  * `emul-mf-ul` command add optional filename (same 64bytes raw file previously written to microsd by `read-mf-ul`)
  * See commit [d569fcd8] 27-May-2016 (https://github.com/bvernoux/hydrafw/commit/d569fcd853415d7d56e54ef773315abb45015285) (modified scan command)
  * This feature is a Beta version and will be rewritten using low level mode SDM TX/DM1 RX in order to be hard real-time & ISO compliant, which will also fix the emulation when using a Phone...
* [NFC sniffer improvements](https://github.com/bvernoux/hydrafw/wiki/HydraFW-HydraNFC-guide#launch-nfc-sniffer-from-console) and [NFC sniffer command cleanup](https://github.com/bvernoux/hydrafw/commit/3ee41cc9070d74d229d7950afbe5479ed6cc0b8f)
  * [Modified NFC RX Gain Reduction from 10dB to 5dB & use ISO14443A mode](https://github.com/bvernoux/hydrafw/commit/8c2f0a3cb6f0b1672965abe7940d6d04df5cd2d3)
  * The sniffer now use native ISO14443A mode instead of previous hybrid mode ISO14443B/A, those modifications (with NFC RX Gain reduction set to 5dB) give better sniffing sensitivity and bigger range to sniff PICC(NFC Tag) and PCD(NFC Reader)
  configured using @6.3Mbauds(in reality it is 8.4Mbauds) 8N1 with Putty on Win7)
   * Removed `sniff-dbg` (replaced by `parity` & `frame-time` sub commands)
   * Added following `sniff` sub commands:
     * [`trace-uart1`](https://github.com/bvernoux/hydrafw/commit/3ee41cc9070d74d229d7950afbe5479ed6cc0b8f) to trace in real-time sniffed data to UART1 PA9 @8.4Mbauds 8N1 (validated with FTDI [C232HM-DDHSL-0](http://www.ftdichip.com/Support/Documents/DataSheets/Cables/DS_C232HM_MPSSE_CABLE.PDF) 
     * `bin` (Force binary sniff trace(UART1 only))       
     * `parity` (Add parity bit information in binary sniff trace(UART1 only))
     * `frame-time` (Add start/end frame timestamp(in CPU cycles))
   * Moved `sniff-raw` command to `sniff` sub command `raw`

#### 16.04.2016 - [HydraFW v0.7 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.7-beta)

##### Generic:
* Fixed ultra slow build (because of built in rules...) thanks to tuxscreen post http://hydrabus.com/forums/topic/make-is-ultra-slow/#post-696 for the fix
* Updated 'tokenline' submodule to the latest version
  * Change T_ARG_INT to T_ARG_UINT, supporting only unsigned integers.

##### HydraBus specific:
* Added freeform string as byte sequence (in addition to freeform integers) (thanks to [Baldanos](https://github.com/Baldanos))

* Added [Binary Modes](https://github.com/bvernoux/hydrafw/wiki/HydraFW-Binary-mode-guide) full documentation in wiki
* Added [Binary Modes](https://github.com/bvernoux/hydrafw/wiki/HydraFW-Binary-mode-guide) (USB CDC compatible with BusPirate BBIO/Bitbang) with:
  * [PIN mode](https://github.com/bvernoux/hydrafw/wiki/HydraFW-binary-PIN-mode-guide) to manage up to 8 GPIO pins (thanks to [Baldanos](https://github.com/Baldanos))
  * [SPI mode](https://github.com/bvernoux/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide) (thanks to [Baldanos](https://github.com/Baldanos))
  * [I2C mode](https://github.com/bvernoux/hydrafw/wiki/HydraFW-Binary-I2C-mode-guide)
  * [2-Wire mode](https://github.com/bvernoux/hydrafw/wiki/HydraFW-binary-2-wire-mode-guide) (thanks to [Baldanos](https://github.com/Baldanos))
  * [UART mode](https://github.com/bvernoux/hydrafw/wiki/HydraFW-binary-UART-mode-guide) (thanks to [Baldanos](https://github.com/Baldanos))
  * [CAN mode](https://github.com/bvernoux/hydrafw/wiki/HydraFW-Binary-CAN-mode-guide) (thanks to [Baldanos](https://github.com/Baldanos))
  * [OpenOCD JTAG](https://github.com/bvernoux/hydrafw/wiki/HydraFW-Binary-mode-guide#main-mode-commands]) (thanks to [Baldanos](https://github.com/Baldanos))

* CAN (thanks to [Baldanos](https://github.com/Baldanos))
  * Corrected filters handling. Now works for both CAN interfaces
  * Fixed `bsp_can_rxne()`
  * Fixed error in can2 pin description 
  * Added speed change for CAN

* Optimized **sump** critical part and Lock Kernel during get_samples()

* JTAG scanner/debugger mode like JTAGulator (thanks to [Baldanos](https://github.com/Baldanos))
  * Add support for TRST
  * Add support for MSB/LSB when reading

##### HydraNFC specific:
* Added in standalone mode sniffing LED indication #36 LED 2 ON for Standalone mode
* Fixed IRQ bug (stopped when enter/exit from sniffer or emul mode)
* Fixed Trf797xInitialSettings with workaround of Errata SLOZ011A–February 2014–Revised April 2015

* HydraNFC emulation commands are in alpha stage and they will be fixed in next version of HydraFW
   * Added in **emul-mifare** display of UID parameter when started
   * Added in **emul-mifare** 4 bytes UID parameter
   * Added **emul-mf-ul** command for Mifare Ultralight Emulation preliminary/work in progress code

* Added **dm0** command => Direct Mode 0 Sniffer Test work fine mainly for test with a Logic Analyzer on PC2
* Added **dm1** command => Direct Mode 1 Test (Work in Progress to test TX SDM & RX DM1)

#### 29.11.2015 - [HydraFW v0.6 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.6-beta)

##### Generic:
* Update of [tokenline](https://github.com/biot/tokenline), now both ```T_ARG_INT``` and ```T_ARG_FLOAT``` take **k**, **m** and **g** suffixes and apply a decimal factor on both argument types.
It is always compatible with old syntax **khz**, **mhz**, **ghz** (only first character is checked).

##### HydraBus specific:
* Added **2-wire** mode (support frequency up to 1MHz) (thanks to [Baldanos](https://github.com/Baldanos))
* Added **random** command new token **~** (```T_TILDE```) to write random byte (thanks to [Baldanos](https://github.com/Baldanos))
 * Random number generator (using STM32 hardware RNG) see command which returns a 32bit random number in hex (thanks to [Baldanos](https://github.com/Baldanos))
* Added **jtag** scanner/debugger mode (thanks to [Baldanos](https://github.com/Baldanos))
   * Classic JTAG (TDI/TDO/TMS/TCK)
   * Can be used with command line
   * BusPirate-compatible OpenOCD binary mode (**openocd** command)
   * Can scan a JTAG bus with IDCODE and BYPASS methods (**idcode** and **bypass** commands)
   * Can try to find JTAG bus on all GPIOB pins (like [JTAGulator](http://www.grandideastudio.com/portfolio/jtagulator/)) (**brute** command)
* Added **sump** mode, Logic Analyzer  up to 1MHz 16chan with SUMP support (thanks to [Baldanos](https://github.com/Baldanos))
 * Compatible with [ols-0.9.7.2](http://ols.lxtreme.nl) see also ols [profile](https://github.com/bvernoux/hydrafw/blob/master/ols.profile-hydrabus.cfg) for hydrabus
 * Compatible with [sigrok](http://sigrok.org): [sigrok-cli & PulseView](http://sigrok.org/wiki/Downloads) 
* Added **can** mode (thanks to [Baldanos](https://github.com/Baldanos) & [smillier](https://github.com/smillier))
  * Needs a [dedicated shield like HydraOBD](https://github.com/smillier/HydraOBD) to communicate with a real CAN bus
   * Support CAN bus 1 or 2 (speed up to 2M)
   * Support **read**, read **continuous**, write, **id** and **filter** commands
* Added in **uart** mode the **bridge** command to be used as UART Raw sniffer (thanks to [Baldanos](https://github.com/Baldanos))
* UART: fix bug in baudrate->BRR (thanks to [doegox](https://github.com/doegox))

##### HydraNFC specific:
* HydraNFC Tag Emulation UID Mifare 1K & ISO14443A
 * Mifare Emulation (Anticol+UID+HALT) see new command **emul-mifare**
 * ISO14443A Emulation (TRF7970A hardware Anticol/UID) see new command **emul-3a**
* Rename command **mifare** to **typea** (ISO14443A) as it was not specific to MIFARE
* **scan** command now support 4 and 7bytes UID (thanks to [NicoHub](https://github.com/NicoHub))
* Sniffer new command **sniff-dbg** with following new features:
 * Add after each 8bits/byte the parity (ASCII char "0" or "1" + space)
 * End Of Frame Timestamp + RSSI

#### 11.02.2015 - [HydraFW v0.5 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.5-beta)
* New USB VID_0x1d50/PID_0x60a7 using OpenMoko HydraFW, update with new drivers
  * Windows see driver_usb_cdc/hydrabus_usb_cdc.inf
  * Linux see 09-hydrabus.rules
* Added USB DFU boot by pressing UBTN at PowerOn/RESET for easier future update of the firmware (does not requires any wire).
* Added Log command (thanks to biot):
  * Log all commands/answer in the Terminal to HydraBus SD card file: `logging` `on` `off` `sd`
* Added command `debug` `test-rx` (debug rx) in order to test USB CDC reception performance.
* Added PWM command:
  * Configurable PWM frequency between 1Hz to 42MHz and duty cycle from 0 to 100%: `pwm`, `frequency`, `duty-cycle`
  * Give feedback of real PWM frequency & duty cycle after configuration.
* Added DAC command:
  * Configurable DAC1(PA4 used by ULED) or DAC2(PA5) with following modes:
    * `raw` (0 to 4095), `volt` (0 to 3.3), also returns feedback of raw/volt DAC value after configuration.
    * `triangle` (5Hz / 3.3V Amplitude), `noise` (0 to 3.3V Amplitude)
    * `dac exit` disable DAC1/2 & Timer6/7 and reinit GPIO for PA4 & PA5
* UART:
  * UART returns real baud rate+% error after configuration.
  * Check and correct min/max value for UART baudrate.
* I2C added `scan` (thanks to biot).
* SPI:
  * Fixed `cs` `on` / `off` bug with Infinite Loop.
  * Fixed SPI mode `slave`.
  * Fixed configuration for `phase` & `polarity` (was always set to 0).
* HydraNFC: 
  * Fixed HydraNFC detection/init/cleanup and autonomous sniffer mode started with K4.
  * Fixed some potential problems/crash with scan/continuous.
* Add full help for commands.
* Lot of cleanup and fixes in code.

---

#### 19.12.2014 - [HydraFW v0.4 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.4-beta)
* Major improvement/refactoring towards terminal/syntax using tokenline (instead of microrl) thanks to Bert Vermeulen (biot) for that amazing piece of software and all the work on this project.
* New commands added:
  * ADC, GPIO, more sd commands
* SPI1&2, I2C and UART1&2 are tested with logic analyzer/oscilloscope.

---

#### 31.10.2014 - [HydraFW v0.3 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.3-beta)
* New modes added: SPI, I2C, UART
* All modes tested with SCANA Plus logic analyzer, need more test with real hardware.
* Lot of fixes & cleanup.

---

#### 13.10.2014 - [HydraFW v0.2 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.2-beta)
* New commands added:
```
cd <dir>       - change directory in sd
pwd            - show current directory path in sd
hd <filename>  - display sd file (HEX)
sd_rperfo      - sd read performance test
```
*Updated ls command.
* Added an option for HydraNFC to be not included by commenting Makefile line export HYDRAFW_NFC ?= 1 (by default it is added)
* Lot of fixes.
* Thanks to biot for contribution !!

---

#### 09.10.2014 - [HydraFW v0.1 Beta](https://github.com/bvernoux/hydrafw/releases/tag/v0.1-beta)
* First version (0.1Beta)
