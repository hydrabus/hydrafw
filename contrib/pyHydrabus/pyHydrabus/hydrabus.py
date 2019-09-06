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

import time
import logging

import serial


class Hydrabus:
    """
    Base class for all modes.

    Manages all serial communication
    """

    def __init__(self, port=""):
        """
        Class init

        :param port: Serial port to use
        """
        self.port = port
        self._logger = logging.getLogger(__name__)

        try:
            self._serialport = serial.Serial(self.port, timeout=None)
            self.enter_bbio()
        except serial.SerialException as e:
            self._logger.error(f"Cannot connect : {e.strerror}")
            raise type(e)(f"Cannot open {self.port}")

    def write(self, data=b""):
        """
        Base write primitive

        :param data: Bytes to write
        :type data: bytes
        """
        if not self.connected:
            raise serial.SerialException("Not connected.")
        try:
            self._logger.debug(f"==>[{str(len(data)).zfill(4)}] {data.hex()}")
            self._serialport.write(data)
            self._serialport.flush()
        except serial.SerialException as e:
            self._logger.error(f"Cannot send : {e.strerror}")
            raise type(e)(f"Cannot send : {e.strerror}")

    def read(self, length=1):
        """
        Base read primitive

        :param length: Number of bytes to read
        :type length: int
        :return: Read data
        :rtype: bytes
        """
        if not self.connected:
            raise serial.SerialException("Not connected.")
        try:
            data = self._serialport.read(length)
            self._logger.debug(f"<==[{str(length).zfill(4)}] {data.hex()}")
            self._lastread = data
            return data
        except serial.SerialException as e:
            self._logger.error(f"Cannot read : {e.strerror}")
            raise type(e)(f"Cannot read : {e.strerror}")

    def flush_input(self):
        """
        Flush input buffer (data from Hydrabus)
        """
        self._serialport.reset_input_buffer()

    def exit_bbio(self):
        """
        Reset Hydrabus to CLI mode
        :return: Bool
        """
        if not self.connected:
            raise serial.SerialException("Not connected.")
        if self.reset() == True:
            self.write(b"\x00")
            self.write(b"\x0F\n")
            return True
        else:
            return False

    def enter_bbio(self):
        """
        Enter BBIO mode. 
        This should be done prior all further operations
        :return: Bool
        """
        if not self.connected:
            raise serial.SerialException("Not connected.")
        self.timeout = 0.01
        for _ in range(20):
            self.write(b"\x00")
            if b"BBIO1" in self.read(5):
                self.flush_input()
                self.timeout = None
                return True
        self._logger.error("Cannot enter BBIO mode.")
        self.timeout = None
        return False

    def reset(self):
        """
        Force reset to BBIO main mode
        :return: Bool
        """
        if not self.connected:
            raise serial.SerialException("Not connected.")
        self.timeout = 0.1
        while self.read(5) != b"BBIO1":
            self.flush_input()
            self.write(b"\x00")
        self.timeout = None
        return True

    def identify(self):
        """
        Identify the current mode

        :return: Current mode
        :rtype: str
        """
        if not self.connected:
            raise serial.SerialException("Not connected.")
        self.write(b"\x01")
        return self.read(4).decode("ascii")

    def close(self):
        """
        Close the serial port
        """
        self._serialport.close()

    @property
    def timeout(self):
        """
        Serial port read timeout
        :return: timeout
        :rtype: int
        """
        return self._serialport.timeout

    @timeout.setter
    def timeout(self, value):
        """
        Set serial port read timeout
        :param value: timeout
        :type value: int
        """
        self._serialport.timeout = value

    @property
    def connected(self):
        """
        Check if serial port is opened
        :return: Bool
        """
        return self._serialport.is_open
