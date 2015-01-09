# CHANGELOG of 'hydrafw'
----------------------

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
