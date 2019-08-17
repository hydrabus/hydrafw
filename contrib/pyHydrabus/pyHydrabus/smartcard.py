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


class Smartcard(Protocol):
    """
    Smartcard protocol handler

    :example:

    >>> #Read ATR from a smartcard
    >>> import pyHydrabus
    >>> sm=pyHydrabus.Smartcard('/dev/hydrabus')
    >>> sm.prescaler=12
    >>> sm.baud=9600
    >>> sm.rst=1;sm.rst=0;sm.read(1)

    """

    def __init__(self, port=""):
        self._config = 0b0000
        self._rst = 1
        self._baud = 9600
        self._prescaler = 12
        self._guardtime = 16
        super().__init__(name=b"CRD1", fname="Smartcard", mode_byte=b"\x0b", port=port)
        self._configure_port()

    def _configure_port(self):
        CMD = 0b10000000
        CMD = CMD | self._config

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            self.rst = self._rst
            return True
        else:
            self._logger.error("Error setting config.")
            return False

    def write_read(self, data=b"", read_len=0):
        """
        Write-then-read operation
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-binary-SMARTCARD-mode-guide#write-then-read-operation-0b00000100

        Parameters:
            data: Data to be sent
            read_len: number of bytes to read
        """
        CMD = 0b00000100
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
            self._logger.warn("Unknown error. Aborting")
            return None

        return self._hydrabus.read(read_len)

    def write(self, data=b""):
        """
        Write on smartard

        Parameters:
            data: data to be sent
        """
        self.write_read(data, read_len=0)

    def read(self, read_len=0):
        """
        Read on smartard

        Parameters:
            read_len: number of bytes to be read
        """
        return self.write_read(b"", read_len=read_len)

    @property
    def prescaler(self):
        """
        Prescaler value
        """
        return self._prescaler

    @prescaler.setter
    def prescaler(self, value):
        CMD = 0b00000110

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(value.to_bytes(1, byteorder="big"))

        if self._hydrabus.read(1) == b"\x01":
            self._prescaler = value
            return True
        else:
            self._logger.error("Error setting prescaler.")
            return False

    @property
    def guardtime(self):
        """
        Guard time value
        """
        return self._prescaler

    @guardtime.setter
    def guardtime(self, value):
        CMD = 0b00000111

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(value.to_bytes(1, byteorder="big"))

        if self._hydrabus.read(1) == b"\x01":
            self._guardtime = value
            return True
        else:
            self._logger.error("Error setting guard time.")
            return False

    @property
    def rst(self):
        """
        RST pin status
        """
        return self._rst

    @rst.setter
    def rst(self, value):
        value = value & 1
        CMD = 0b00000010
        CMD = CMD | value

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            self._rst = value
            return True
        else:
            self._logger.error("Error setting pin.")
            return False

    @property
    def baud(self):
        """
        Baud rate
        """
        return self._baud

    @baud.setter
    def baud(self, value):
        CMD = 0b01100000

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(value.to_bytes(4, byteorder="big"))

        if self._hydrabus.read(1) == b"\x01":
            self._baud = value
            return True
        else:
            self._logger.error("Error setting pin.")
            return False

    @property
    def pullup(self):
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
