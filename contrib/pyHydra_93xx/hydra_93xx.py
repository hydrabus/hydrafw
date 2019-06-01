#!/usr/bin/env python3
#
# Script to communicate to EEPROM 3-wire memories with Hydrabus.
#
# Author: Sylvain Pelissier <sylvain.pelissier@gmail.com>
# License: GPLv3 (https://choosealicense.com/licenses/gpl-3.0/)
#
from pyHydrabus import RawWire

class EEPROM93xx(RawWire):
    """
        Serial EEPROM  handler for the 93xx series (93C06, 93C46, 93C86, ...)
    """
    def __init__(self, port="", address_size=8, word_size=16):
        """
        Handler constructor
        :param address_size: Address size in bits
        :type data: int
        :param word_size: Word size in bits
        :type word_size: int
        """
        super().__init__(port)
        
        if address_size < 2:
            print("Address size must be at least 2 bit long")
            exit(2)
            
        self.address_size = address_size
        self.word_size = word_size // 8
        self.gpio_mode = 1
        self.set_speed(5000)
        self.cs = self.AUX[0]
        self.cs.direction=0
        self.cs.value=0

    def write_enable(self):
        """
        Write enable: opcode 00
        """
        self.cs.value=1
        # Start bit and opcode
        self.sda = 1
        self.clock()
        self.sda = 0
        self.clocks(2)
        
        self.sda = 1
        self.clocks(self.address_size)
        self.value=0
        self.cs.value=0
        
    def write_disable(self):
        """
        Write disable: opcode 00
        """
        self.cs.value=1
        # Start bit and opcode
        self.sda = 1
        self.clock()
        self.sda = 0
        self.clocks(2)
        
        self.clocks(self.address_size)
        self.cs.value=0

    def read_data(self, address, num):
        """
        Read: opcode 10
            
        :param address: Address to read
        :type data: int
        :param num: number of words to read
        :type word_size: int
        """
        data_format = "{:0" + str(self.address_size) + "b}"
        addr_bin = data_format.format(address)       
        self.cs.value=1

        # Start bit and opcode
        self.sda = 1
        self.clocks(2)
        self.sda = 0
        self.clock()
        
        # Write address
        for i in addr_bin:
            self.sda = int(i, 2)
            self.clock()
        
        # Read values
        data = self.read(num)
        self.cs.value=0
        
        return data

    def write_data(self, address, data):
        """
        Read: opcode 01
        
        :param address: Address to write
        :type data: int
        :param data: data to write
        :type word_size: bytes    
        """
        
        if len(data)*8 % self.word_size != 0:
            print("Data length must be multiple of %d bits" % self.word_size * 8)
            exit(2)
        
        data_format = "{:0" + str(self.address_size) + "b}"
        
        for i in range(0, len(data), self.word_size):
            addr_bin = data_format.format(address + i // self.word_size)
            self.cs.value=1
            # Start bit and Opcode
            self.sda = 1
            self.clock()
            self.sda = 0
            self.clock()
            self.sda = 1
            self.clock()
            
            # Write address
            for j in addr_bin:
                self.sda = int(j, 2)
                self.clock()
            
            self.write(data[i:i + self.word_size])
            self.cs.value=0
    
    def erase(self, address):
        """
        Erase: opcode 11
        
        :param address: Address to erase
        :type data: int    
        """
        
        data_format = "{:0" + str(self.address_size) + "b}"
        addr_bin = data_format.format(address)
        
        self.cs.value=1
        # Start bit and opcode
        self.sda = 1
        self.clock()
        self.sda = 1
        self.clocks(2)
        
        # Write address
        for i in addr_bin:
            self.sda = int(i, 2)
            self.clock()
            
        self.cs.value=0
        
if __name__ == '__main__':
    eeprom = EEPROM93xx('/dev/ttyACM0', 6, 16)
    
    print(eeprom.wires)
    print(eeprom.cs.value)
    
    eeprom.write_enable()
    eeprom.write_data(0,b"Hello world !!")
    eeprom.write_disable()

    print(eeprom.read_data(0,128))
    

