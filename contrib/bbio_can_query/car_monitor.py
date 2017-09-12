# Simple BBIO CAN example querying a car current speed and engine RPM
# Can be tested by connecting the HydraLINCAN shield to the OBD2 port of any recent car
#
# See https://en.wikipedia.org/wiki/OBD-II_PIDs for more information about OBD2
import serial
import os
import time
import sys

# Issue a read packet command, and parse its output
def rdpkt():
    ser.write('\x02')
    ser.read(1)
    can_id = int(ser.read(4).encode('hex'), 16)
    length = ord(ser.read(1))
    data = ser.read(length)
    return (can_id, data)

out = open('/tmp/car.log', 'w')
try:
    ser = serial.Serial(sys.argv[1], 115200)
except:
    print "Cannot open the serial interface."
    print "Usage : %s <port>" % sys.argv[0]

#Enter BBIO mode
for i in xrange(20):
    ser.write("\x00")
if "BBIO1" not in ser.read(5):
    print "Could not get into bbIO mode"
    quit()

print "Switching to CAN mode"
ser.write('\x08')
if "CAN1" not in  ser.read(4):
    print "Cannot set CAN mode"
    quit()

# OBD2 queries must use CAN ID 0x7DF
print "Setting ID"
ser.write('\x03\x00\x00\x07\xdf')
if ser.read(1) != '\x01':
    print "Error setting ID"
    quit()

# OBD2 responses are sent with CAN IF 0x7E8
print "Setting filter low"
ser.write('\x06\x00\x00\x07\xe8')
if ser.read(1) != '\x01':
    print "Error setting filter"
    quit()

while 1:
    # RPM query
    ser.write('\x0f\x02\x01\x0c\x55\x55\x55\x55\x55')
    ser.read(1)
    can_id, data = rdpkt()
    rpmdata = int(data[3:5].encode('hex'), 16)/4

    #Vehicle speed query
    ser.write('\x0f\x02\x01\x0d\x55\x55\x55\x55\x55')
    ser.read(1)
    can_id, data = rdpkt()
    speeddata = int(data[3:4].encode('hex'), 16)

    print speeddata, rpmdata
