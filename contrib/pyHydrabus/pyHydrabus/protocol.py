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
from .hydrabus import Hydrabus
from .auxpin import AUXPin


class Protocol:
    """
    Base class for all supported protocols

    :param name: Name of the protocol (returned by Hydrabus) eg. SPI1
    :type name: str
    :param fname: Full name of the protocol
    :type fname: str
    :param mode_byte: Byte used to enter the mode (eg. \x01 for SPI)
    :type mode_byte: bytes
    :param port: The port name
    :type port: str
    """

    def __init__(self, name="", fname="", mode_byte=b"\x00", port=""):
        self.name = name
        self.fname = fname
        self._mode_byte = mode_byte
        self._hydrabus = Hydrabus(port)
        self._logger = logging.getLogger(__name__)

        self._enter()
        self._hydrabus.flush_input()

        self.AUX = []
        for i in range(4):
            self.AUX.append(AUXPin(i, self._hydrabus))


    def _enter(self):
        self._hydrabus.write(self._mode_byte)
        if self._hydrabus.read(4) == self.name:
            self._hydrabus.mode = self.name
            return True
        else:
            self._logger.error(f"Cannot enter mode.")
            return False

    def _exit(self):
        return self._hydrabus.reset()

    def identify(self):
        """
        Identify the current mode

        :return: The current mode identifier (4 bytes)
        :rtype: str
        """
        return self._hydrabus.identify()

    def close(self):
        """
        Close the communication channel and resets Hydrabus
        """
        self._hydrabus.exit_bbio()
        self._hydrabus.close()

    @property
    def timeout(self):
        return self._hydrabus.timeout

    @timeout.setter
    def timeout(self, value):
        self._hydrabus.timeout = value
