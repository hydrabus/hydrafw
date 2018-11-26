# Example using Console mode Initialize GPIO + bbIO mode to configure SPI and to communicate with TRF7970A(using HydraNFC Shield)
# For more details see:
# https://github.com/bvernoux/hydrafw/wiki/HydraFW-HydraNFC-TRF7970A-Tutorial#initialize-gpio--spi-to-communicate-with-trf7970a
#
import hexdump
import serial
import time
import sys
import struct

# see python -m serial.tools.list_ports
#ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.01)
ser = serial.Serial('COM3', 115200, timeout=0.01)

def cmd_check_status(status):
	if status != '\x01':
		print status.encode('hex'),
		print "Error",
		print ""
	else:
		print "OK",
		print ""

def cs_on(): 
	ser.write('\02')
	status=ser.read(1)
	if status != '\x01':
		print "CS-ON:",
		print status.encode('hex'),
		print "Error",
		print ""

def cs_off():
	ser.write('\03')
	status=ser.read(1)
	if status != '\x01':
		print "CS-OFF:",
		print status.encode('hex'),
		print "Error",
		print ""

def configure_trf797a_gpio():
	# Configure NFC/TRF7970A in SPI mode with Chip Select
	print("Configure NFC/TRF7970A in SPI mode with Chip Select")
	ser.write("exit\n")
	print(ser.readline()),;print(ser.readline()),
	print(ser.readline()),;print(ser.readline()),
	ser.write("\n")
	print(ser.readline()),;print(ser.readline()),

	ser.write("gpio pa3 mode out off\n")
	print(ser.readline()),;print(ser.readline()),
	ser.write("gpio pa2 mode out on\n")
	print(ser.readline()),;print(ser.readline()),
	ser.write("gpio pc0 mode out on\n")
	print(ser.readline()),;print(ser.readline()),
	ser.write("gpio pc1 mode out on\n")
	print(ser.readline()),;print(ser.readline()),
	ser.write("gpio pb11 mode out off\n")
	print(ser.readline()),;print(ser.readline()),

	time.sleep(0.02);

	ser.write("gpio pb11 mode out on\n");
	print(ser.readline()),;print(ser.readline()),

	time.sleep(0.01);

	ser.write("gpio pa2-3 pc0-1 pb11 r\n");
	print(ser.readline()),;print(ser.readline()),
	print(ser.readline()),;print(ser.readline()),
	print(ser.readline()),;print(ser.readline()),
	print(ser.readline()),;print(ser.readline()),
	print ""

def enter_bbio():
	for i in xrange(20):
		ser.write("\x00")
	if "BBIO1" in ser.read(5):
		print "Into BBIO mode: OK",
		print(ser.readline()),
		print ""
	else:
		print "Could not get into bbIO mode"
		exit()

def exit_bbio():
	ser.write('\x00')
	ser.write('\x0F\n')
	ser.readline();ser.readline()

def bbio_spi_conf():
	print "Switching to SPI mode:",
	ser.write('\x01')
	print ser.read(4),
	print(ser.readline()),
	print ""

	print "Configure SPI2 polarity 0 phase 1:",
	ser.write('\x80')
	status=ser.read(1) # Read Status
	cmd_check_status(status)

	print "Configure SPI2 speed to 2620000 bits/sec:",
	ser.write('\x63')
	status=ser.read(1) # Read Status
	cmd_check_status(status)

def trf7970a_software_init():
	cs_on()
	print "Write TRF7970A Software Initialization 0x83 0x83 (no read):",
	ser.write('\x05\x00\x02\x00\x00') # Write 2 data, read 0 data
	ser.write('\x83\x83') # Data
	status=ser.read(1) # Read Status
	cmd_check_status(status)
	cs_off()

def trf7970a_write_idle():
	cs_on()
	print "Write TRF7970A Idle 0x80 0x80 (no read):",
	ser.write('\x05\x00\x02\x00\x00') # Write 2 data, read 0 data
	ser.write('\x80\x80') # Data
	status=ser.read(1) # Read Status
	cmd_check_status(status)
	cs_off()

def trf7970a_read_modulator():
	cs_on()
	print "Read TRF7970A Modulator/SYS_CLK Control Register (0x09):",
	ser.write('\x05\x00\x01\x00\x01') # Write 1 data, read 1 data
	ser.write('\x49') # Data
	status=ser.read(1) # Read Status
	modulator = ser.read(1)
	print modulator.encode('hex'), # Read Data
	if modulator == '\x91':
		print "OK",
	else:
		print "Error"
	print ""
	cs_off()
	print ""
	if modulator == '\x91':
		return 1
	else:
		return 0

def bbio_trf7970a_init():
	i = 0
	end = 10
	while True:
		trf7970a_software_init()
		trf7970a_write_idle()
		time.sleep(0.1)
		ret=trf7970a_read_modulator()
		if ret == 1:
			break
		i = i + 1
		if  i > end:
			break

# main code
configure_trf797a_gpio()
enter_bbio()
bbio_spi_conf()
bbio_trf7970a_init()
exit_bbio()

