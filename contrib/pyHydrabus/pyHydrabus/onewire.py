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


class OneWire(Protocol):
    """
    One wire protocol handler

    :example:

    TODO

    """

    __ONEWIRE_DEFAULT_CONFIG = 0b100

    def __init__(self, port=""):
        self._config = self.__ONEWIRE_DEFAULT_CONFIG
        super().__init__(name=b"1W01", fname="1-Wire", mode_byte=b"\x04", port=port)
        self._configure_port()

    def reset(self):
        """
        Send a 1-Wire reset
        """
        self._hydrabus.write(b"\x02")
        return True

    def read_byte(self):
        """
        Read a byte from the 1-Wire bus
        """
        self._hydrabus.write(b"\x04")
        return self._hydrabus.read(1)

    def write(self, data=b""):
        """
        Write on 1-Wire bus

        :param data: data to be sent
        :type data: bytes
        """
        for chunk in split(data, 16):
            self.bulk_write(chunk)

    def read(self, read_len=0):
        """
        Read on 1-Wire bus

        :param read_len: Number of bytes to be read
        :return read_len: int
        :return: Read data
        :rtype: bytes
        """
        return b"".join([self.read_byte() for x in range(read_len)])

    def bulk_write(self, data=b""):
        """
        Bulk write on 1-Wire bus
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-binary-1-Wire-mode-guide#bulk-1-wire-transfer-0b0001xxxx

        :param data: Data to be sent
        :type data: bytes
        """
        CMD = 0b00010000
        if not len(data) > 0:
            raise ValueError("Send at least one byte")
        if not len(data) <= 16:
            raise ValueError("Too many bytes to write")
        CMD = CMD | (len(data) - 1)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(data)

        if self._hydrabus.read(1) != b"\x01":
            self._logger.warn("Unknown error.")

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
