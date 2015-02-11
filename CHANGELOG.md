# CHANGELOG of 'hydrafw'
----------------------

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
