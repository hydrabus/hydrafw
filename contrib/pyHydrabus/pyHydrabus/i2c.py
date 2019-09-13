# Copyright 2019 Nicolas OBERLI
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
from .protocol import Protocol


class I2C(Protocol):
    """
    I2C protocol handler

    :example:

    >>> #Read data from an EEPROM
    >>> import pyHydrabus
    >>> i=pyHydrabus.I2C('/dev/hydrabus')
    >>> i.set_speed(pyHydrabus.I2C.I2C_SPEED_100K)
    >>> i.pullup=1
    >>> i.start();i.bulk_write(b'\xa0\x00');print(i.write_read(b'\xa1',64))

    """

    __I2C_DEFAULT_CONFIG = 0b000

    I2C_SPEED_50K = 0b00
    I2C_SPEED_100K = 0b01
    I2C_SPEED_400K = 0b10
    I2C_SPEED_1M = 0b11

    def __init__(self, port=""):
        self._config = self.__I2C_DEFAULT_CONFIG
        super().__init__(name=b"I2C1", fname="I2C", mode_byte=b"\x02", port=port)
        self._configure_port()

    def start(self):
        """
        Send a I2C start condition
        """
        self._hydrabus.write(b"\x02")
        if self._hydrabus.read(1) == b"\x00":
            self._logger.error("Cannot execute command.")
            return False
        return True

    def stop(self):
        """
        Send a I2C stop condition
        """
        self._hydrabus.write(b"\x03")
        if self._hydrabus.read(1) == b"\x00":
            self._logger.error("Cannot execute command.")
            return False
        return True

    def read_byte(self):
        """
        Read a byte from the I2C bus
        """
        self._hydrabus.write(b"\x04")
        return self._hydrabus.read(1)

    def send_ack(self):
        """
        Send an ACK
        Used with the read_byte() function
        """
        self._hydrabus.write(b"\x06")
        if self._hydrabus.read(1) == b"\x00":
            self._logger.error("Cannot execute command.")
            return False
        return True

    def send_nack(self):
        """
        Send a NACK
        Used with the read_byte() function
        """
        self._hydrabus.write(b"\x07")
        if self._hydrabus.read(1) == b"\x00":
            self._logger.error("Cannot execute command.")
            return False
        return True

    def write_read(self, data=b"", read_len=0):
        """
        Write-then-read operation
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-I2C-mode-guide#i2c-write-then-read-0b00001000

        This method sends a start condition before writing and a stop condition after reading.

        :param data: Data to be sent
        :type data: bytes
        :param read_len: Number of bytes to read
        :type read_len: int
        :return: Read data
        :rtype: bytes
        """
        CMD = 0b00001000
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))

        self._hydrabus.write(len(data).to_bytes(2, byteorder="big"))

        self._hydrabus.write(read_len.to_bytes(2, byteorder="big"))

        self._hydrabus.timeout = 0
        if self._hydrabus.read(1) == b"\x00":
            self._logger.error("Cannot execute command. Too many bytes requested ?")
            return None
        self._hydrabus.timeout = None

        self._hydrabus.write(data)

        if self._hydrabus.read(1) != b"\x01":
            self._logger.warn("Data not ACKed. Aborting.")
            return None

        return self._hydrabus.read(read_len)

    def write(self, data=b""):
        """
        Write on I2C bus

        :param data: data to be sent
        :type data: bytes
        """
        self.write_read(data, read_len=0)

    def read(self, length=0):
        """
        Read on I2C bus

        :param length: Number of bytes to read
        :type length: int
        :return: Read data
        :rtype: bytes
        """
        result = []
        for _ in range(length - 1):
            result.append(self.read_byte())
            self.send_ack()
        result.append(self.read_byte())
        self.send_nack()

        return b"".join(result)

    def bulk_write(self, data=b""):
        """
        Bulk write on I2C bus
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-I2C-mode-guide#bulk-i2c-write-0b0001xxxx

        :param data: Data to be sent
        :type data: bytes

        :return: ACK status of the written bytes (b'\x00'=ACK, b'\x01'=NACK)
        :rtype: list
        """
        CMD = 0b00010000
        if not len(data) > 0:
            raise ValueError("Send at least one byte")
        if not len(data) <= 16:
            raise ValueError("Too many bytes to write")
        CMD = CMD | (len(data) - 1)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) != b"\x01":
            self._logger.warn("Unknown error.")
            return None

        self._hydrabus.write(data)

        return self._hydrabus.read(len(data))

    def scan(self):
        """
        Scan I2C bus and returns all available device addresses in a list

        :return: List of available device addresses
        :rtype: list
        """
        result = []
        for i in range(1, 0x78):
            addr = (i << 1).to_bytes(1, byteorder="big")
            self.start()
            if self.bulk_write(addr) == b"\x00":
                addr = int.from_bytes(addr, byteorder="big") >> 1
                result.append(addr)
            self.stop()
        return result

    def set_speed(self, speed):
        """
        Set I2C bus speed

        :param speed: Select any of the defined speeds (I2C_SPEED_*)
        """
        if not speed <= 0b11:
            raise ValueError("Incorrect speed")
        CMD = 0b01100000
        CMD = CMD | speed
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))

        if self._hydrabus.read(1) == b"\x01":
            return True
        else:
            self._logger.error("Error setting speed.")
            return False

    def _configure_port(self):
        CMD = 0b01000000
        CMD = CMD | self._config

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            return True
        else:
            self._logger.error("Error setting config.")
            return False

    @property
    def pullup(self):
        """
        Pullup status (0=off, 1=on)
        """
        if self._config & 0b100:
            return 1
        else:
            return 0

    @pullup.setter
    def pullup(self, value):
        if value == 0:
            self._config = self._config & ~(1 << 2)
        else:
            self._config = self._config | (1 << 2)
        self._configure_port()
