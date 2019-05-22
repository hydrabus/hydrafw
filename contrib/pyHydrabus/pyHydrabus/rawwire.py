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
from .common import split


class RawWire(Protocol):
    """
    Raw wire protocol handler

    :example:

    >>> import pyHydrabus
    >>> i=pyHydrabus.I2C('/dev/hydrabus')
    >>> # Set SDA to high
    >>> sm.sda = 1
    >>> # Send two clock ticks
    >>> sm.clocks(2)
    >>> # Read two bytes
    >>> data = sm.read(2)

    """

    __RAWWIRE_DEFAULT_CONFIG = 0b100

    def __init__(self, port=""):
        self._config = self.__RAWWIRE_DEFAULT_CONFIG
        self._clk = 0
        self._sda = 0
        super().__init__(name=b"RAW1", fname="Raw-Wire", mode_byte=b"\x05", port=port)
        self._configure_port()

    def read_bit(self):
        """
        Sends a clock tick, and return the read bit value
        """
        CMD = 0b00000111
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        return self._hydrabus.read(1)

    def read_byte(self):
        """
        Read a byte from the raw wire

        :return: The read byte
        :rtype: bytes
        """
        CMD = 0b00000110
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        return self._hydrabus.read(1)

    def clock(self):
        """
        Send a clock tick
        """
        CMD = 0b00001001

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            return True
        else:
            self._logger.error("Error setting pin.")
            return False

    def bulk_ticks(self, num):
        """
        Sends a bulk of clock ticks (1 to 16)
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-binary-raw-wire-mode-guide#bulk-clock-ticks-0b0010xxxx

        :param num: Number of clock ticks to send
        :type num: int
        """
        assert num > 0, "Send at least one clock tick"
        assert num <= 16, "Too many ticks to send"

        CMD = 0b00100000
        CMD = CMD | (num - 1)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))

        if self._hydrabus.read(1) == b"\x01":
            return True
        else:
            self._logger.error("Error sending clocks.")
            return False

    def clocks(self, num):
        """
        Sends a number of clock ticks

        :param num: Number of clock ticks to send
        :type num: int
        """
        assert num > 0, "Must be a positive integer"

        while num > 16:
            self.bulk_ticks(16)
            num = num - 16
        self.bulk_ticks(num)

    def bulk_write(self, data=b""):
        """
        Bulk write on Raw-Wire
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-binary-raw-wire-mode-guide#bulk-raw-wire-transfer-0b0001xxxx

        Parameters:
        :param data: Data to be sent
        :type data: bytes
        """
        CMD = 0b00010000
        assert len(data) > 0, "Send at least one byte"
        assert len(data) <= 16, "Too many bytes to write"
        CMD = CMD | (len(data) - 1)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(data)

        if self._hydrabus.read(1) != b"\x01":
            self._logger.warn("Unknown error.")

        return self._hydrabus.read(len(data))

    def set_speed(self, speed):
        """
        Sets the clock max speed.

        :param speed: speed in Hz. Possible values : TODO
        """
        speeds = {5000: 0b00, 50000: 0b01, 100_000: 0b10, 1_000_000: 0b11}
        assert speed in speeds.keys(), f"Incorrect value. use {speeds.keys()}"
        CMD = 0b01100000
        CMD = CMD | speeds[speed]
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            return True
        else:
            self._logger.error("Error setting speed.")
            return False

    def _configure_port(self):
        CMD = 0b10000000
        CMD = CMD | self._config

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            return True
        else:
            self._logger.error("Error setting config.")
            return False

    def write(self, data=b""):
        """
        Write on Raw-Wire bus

        :param data: data to be sent
        :type data: bytes

        :return: Read bytes
        :rtype: bytes
        """
        result = b''
        for chunk in split(data, 16):
            result += self.bulk_write(chunk)
        return result

    def read(self, length=0):
        """
        Read on Raw-Wire bus

        :param length: Number of bytes to read
        :type length: int
        :return: Read data
        :rtype: bytes
        """
        result = b''
        for _ in range(length):
            result += self.read_byte()

        return result

    @property
    def clk(self):
        """
        CLK pin status
        """
        return self._clk

    @clk.setter
    def clk(self, value):
        value = value & 1
        CMD = 0b00001010
        CMD = CMD | value

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            self._clk = value
            return True
        else:
            self._logger.error("Error setting pin.")
            return False

    @property
    def sda(self):
        """
        SDA pin status
        """
        CMD = 0b00001000
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        return int.from_bytes(self._hydrabus.read(1), byteorder="big")

    @sda.setter
    def sda(self, value):
        value = value & 1
        CMD = 0b00001100
        CMD = CMD | value

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            self._clk = value
            return True
        else:
            self._logger.error("Error setting pin.")
            return False

    @property
    def wires(self):
        """
        Raw-Wire mode (2=2-Wire, 3=3-Wire)
        """
        if self._config & 0b100 == 0:
            return 2
        else:
            return 3

    @wires.setter
    def wires(self, value):
        if value == 2:
            self._config = self._config & ~(1 << 2)
            self._configure_port()
            return True
        elif value == 3:
            self._config = self._config | (1 << 2)
            self._configure_port()
            return True
        else:
            self._logger.error("Incorrect value. Must be 2 or 3")

    @property
    def gpio_mode(self):
        """
        Raw-Wire GPIO mode (0=Push-Pull, 1=Open Drain)
        """
        return (self._config & 0b1000) >> 3

    @gpio_mode.setter
    def gpio_mode(self, value):
        if value == 0:
            self._config = self._config & ~(1 << 3)
            self._configure_port()
            return True
        elif value == 1:
            self._config = self._config | (1 << 3)
            self._configure_port()
            return True
        else:
            self._logger.error("Incorrect value. Must be 0 or 1")

