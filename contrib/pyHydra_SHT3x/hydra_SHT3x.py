#!/usr/bin/env python3
#
# Script to communicate to I2C Sensirion Humidity and Temperature Sensor (SHT30, SHT31, SHT35)
#
# Author: Benjamin Vernoux <bvernoux@gmail.com>
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#
import sys
import time
from pyHydrabus import I2C

class SHT3x(I2C):
    """
         I2C Sensirion Humidity and Temperature Sensor (SHT30, SHT31, SHT35)
    """
    def __init__(self, port="", i2c_addr_7bits=0x44):
        """
        Handler constructor
        :param i2c_addr_7bits: I2C Addr 7bits
        """
        SHT3X_ACK_TIME_MAX_MILLISEC = 20
        I2C_CLK_STRETCH = int((SHT3X_ACK_TIME_MAX_MILLISEC/(1/400))) # 20ms max @400KHz
        super().__init__(port)
        self.set_speed(self.I2C_SPEED_400K)
        self.pullup=0 # Use external 10K pull-up
        self.clock_stretch(I2C_CLK_STRETCH)
        self.i2c_addr_write_8b = (i2c_addr_7bits << 1)
        self.i2c_addr_read_8b = self.i2c_addr_write_8b + 1
        
    def read_data(self):
        data1 = self.i2c_addr_write_8b.to_bytes(1, byteorder="big") + b'\x2c\x06'
        self.write(data1);
        data2= self.i2c_addr_read_8b.to_bytes(1, byteorder="big")
        data = self.write_read(data2, 6)
        #print(data1, len(data1))
        #print(data2, len(data2))
        #print(data, len(data))
        cTemp=0
        humidity=0
        if (data is not None):
            cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45
            humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0)
        return cTemp, humidity
        
if __name__ == '__main__':
    args = sys.argv[1:]
    #sht3x = SHT3x('/dev/ttyACM0')
    sht3x = SHT3x('COM7')
    iterations = 0
    if len(args) > 0:
        iterations = int(args[0])
    if iterations < 1:
        iterations = 1
    for x in range(iterations):
        cTemp, humidity = sht3x.read_data()
        print("T(°C)="+"{:.2f}".format(cTemp)+" Humidity(%)="+"{:.2f}".format(humidity))
        time.sleep(0.25)
