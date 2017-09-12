# Hydrabus utils

This folder contains various useful utilities and config files to enhance the
Hydrabus usage.

## ols-profile

The [OLS](https://www.lxtreme.nl/ols/) client profile file that can be used for
Hydrabus.

Just copy this file in the installation folder for the Hydrabus to be detected.

## openocd

This config file for openocd allows to use the Hydrabus as a debugging platform.

The `hydrabus.cfg` file can be placed in the `scripts/interface/` folder.

## udev-rules

This file adds udev rules for the Hydrabus serial port to be usable by any user.

This udev rule file must be installed int the `/etc/udev/rules.d/` folder. Once
installed, the following root command will enable the rule without rebooting :

```
# udevadm control --reload-rules && udevadm trigger
```

## windows_usb_driver

This driver installation file can be used to use the Hydrabus on Windows. At
the first use, just select this file when requested to select the driver.
