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


class SPI(Protocol):
    """
    SPI protocol handler

    :example:

    TODO

    """

    __SPI_DEFAULT_CONFIG = 0b010

    SPI_SPEED_320K = 0b000
    SPI_SPEED_650K = 0b001
    SPI_SPEED_1M = 0b010
    SPI_SPEED_2M = 0b011
    SPI_SPEED_5M = 0b100
    SPI_SPEED_10M = 0b101
    SPI_SPEED_21M = 0b110
    SPI_SPEED_42M = 0b111

    def __init__(self, port=""):
        self._config = self.__SPI_DEFAULT_CONFIG
        self._cs_val = 1
        super().__init__(name=b"SPI1", fname="SPI", mode_byte=b"\x01", port=port)
        self._configure_port()

    @property
    def cs(self):
        """
        Chip-Select (CS) status getter
        """
        return self._cs_val

    @cs.setter
    def cs(self, mode=0):
        """
        Chip-Select (CS) status setter

        :param mode: CS pin status (0=low, 1=high)
        """
        CMD = 0b00000010
        CMD = CMD | mode
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            self._cs_val = mode
        else:
            self._logger.error("Error setting CS.")

    def write_read(self, data=b"", read_len=0, drive_cs=0):
        """
        Write-then-read operation
        https://github.com/hydrabus/hydrafw/wiki/HydraFW-Binary-SPI-mode-guide#write-then-read-operation-0b00000100---0b00000101

        :param data: Data to be sent
        :type data: bytes
        :param read_len: Number of bytes to read
        :type read_len: int
        :param drive_cs: Whether to enable chip select before writing/reading (0=no, 1=yes)
        :type drive_cs: int
        :return: Read data
        :rtype: bytes
        """
        CMD = 0b00000100
        CMD = CMD | drive_cs
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
            self._logger.error("Unknown error.")
            return None

        return self._hydrabus.read(read_len)

    def write(self, data=b"", drive_cs=0):
        """
        Write on SPI bus

        :param data: data to be sent
        :type data: bytes
        :param drive_cs: Whether to enable chip select before writing/reading (0=no, 1=yes)
        :type drive_cs: int
        """
        self.write_read(data, read_len=0, drive_cs=drive_cs)

    def read(self, read_len=0, drive_cs=0):
        """
        Read on SPI bus

        :param read_len: Number of bytes to be read
        :type read_len: int
        :param drive_cs: Whether to enable chip select before writing/reading (0=no, 1=yes)
        :type drive_cs: int
        :return: Read data
        :rtype: bytes
        """
        return self.write_read(b"", read_len=read_len, drive_cs=drive_cs)

    def set_speed(self, speed):
        """
        Set SPI bus speed

        :param speed: Select any of the defined speeds (SPI_SPEED_*)
        """
        assert speed <= 0b111, "Incorrect speed"
        CMD = 0b01100000
        CMD = CMD | speed
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

    @property
    def polarity(self):
        """
        SPI polarity
        """
        if self._config & 0b100:
            return 1
        else:
            return 0

    @polarity.setter
    def polarity(self, value):
        if value == 0:
            self._config = self._config & ~(1 << 2)
        else:
            self._config = self._config | (1 << 2)
        self._config & 0b111
        self._configure_port()

    @property
    def phase(self):
        """
        SPI clock phase
        """
        if self._config & 0b10:
            return 1
        else:
            return 0

    @phase.setter
    def phase(self, value):
        if value == 0:
            self._config = self._config & ~(1 << 1)
        else:
            self._config = self._config | (1 << 1)
        self._config & 0b111
        self._configure_port()

    @property
    def device(self):
        """
        SPI device to use (0=spi1, 1=spi2)
        """
        if self._config & 0b1:
            return 1
        else:
            return 0

    @device.setter
    def device(self, value):
        self._config = self.__SPI_DEFAULT_CONFIG
        if value == 0:
            self._config = self._config & ~(1 << 0)
        else:
            self._config = self._config | (1 << 0)
        self._configure_port()
