# Car monitor

## Author

Nicolas OBERLI (balda@balda.ch)

## Information

This script shows the basic BBIO CAN usage.

It is meant to be used with the HydraLINCAN shield, and can be used by
connecting it to a OBD2 port in a recent car.

When running, it will display the current vehicle speed and ehe engine RPM data

## Usage

The script uses Python2 and the PySerial library.

Run the script with the Hydrabus serial port as a parameter :

```
$ python2 car_monitor.py /dev/ttyACM0
```

