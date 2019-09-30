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


class UART(Protocol):
    """
    UART protocol handler

    :example: TODO

    >>> #Read data from an EEPROM
    >>> import pyHydrabus
    >>> u=pyHydrabus.UART('/dev/hydrabus')
    >>> u.baud=115200
    >>> u.echo=1

    """

    def __init__(self, port=""):
        self._config = 0b0000
        self._echo = 0
        self._baud = 9600
        super().__init__(name=b"ART1", fname="UART", mode_byte=b"\x03", port=port)

    def bulk_write(self, data=b""):
        """
        Bulk write on UART
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-I2C-mode-guide#bulk-i2c-write-0b0001xxxx FIXME: change link

        :param data: Data to be sent
        :type data: bytes

        :return: Returns the ACK status of the written bytes (b'\x00'=ACK, b'\x01'=NACK)
        :rtype: list
        """
        CMD = 0b00010000
        if not len(data) > 0:
            raise ValueError("Send at least one byte")
        if not len(data) <= 16:
            raise ValueError("Too many bytes to write")
        CMD = CMD | (len(data) - 1)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))

        self._hydrabus.write(data)

        for _ in range(len(data)):
            if self._hydrabus.read(1) != b"\x01":
                self._logger.warn("Transfer error.")

    def write(self, data=b""):
        """
        Write on UART bus

        :param data: data to be sent
        :type data: bytes
        """
        for chunk in split(data, 16):
            self.bulk_write(chunk)

    @property
    def echo(self):
        """
        Local echo (0=No, 1=Yes)
        """
        return self._echo

    @echo.setter
    def echo(self, value):
        value = value & 1
        self._echo = value
        CMD = 0b00000010
        CMD = CMD | (not value)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            self._rst = value
            return True
        else:
            self._logger.error("Error setting echo.")
            return False

    def read(self, length=1):
        """
        Read echoed data

        :param length: Number of bytes to read
        :type length: int

        :return: read bytes
        :rtype: bytes
        """

        return self._hydrabus.read(length)

    @property
    def baud(self):
        """
        Baud rate
        """
        return self._baud

    @baud.setter
    def baud(self, value):
        CMD = 0b00000111

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(value.to_bytes(4, byteorder="big"))

        if self._hydrabus.read(1) == b"\x01":
            self._baud = value
            return True
        else:
            self._logger.error("Error setting pin.")
            return False

    def bridge(self):
        """
        Bind the Hydrabus USB and UART
        Note that in order to leave bridge mode, you need to press the UBTN
        """
        CMD = 0b00001111
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
