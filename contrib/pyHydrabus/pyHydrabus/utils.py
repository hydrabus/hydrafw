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


class Utils:
    """
    Utilities available in hydrafw

    :param port: The port name
    :type port: str
    """

    def __init__(self, port=""):
        self._hydrabus = Hydrabus(port)
        self._logger = logging.getLogger(__name__)

        self._hydrabus.flush_input()

    @property
    def adc(self):
        """
        Read ADC value

        :return: ADC value (10 bits)
        :rtype: int
        """
        self._hydrabus.write(b"\x14")
        v = self._hydrabus.read(2)
        return int.from_bytes(v, byteorder="big")

    def continuous_adc(self):
        """
        Continuously print ADC value
        """
        try:
            self._hydrabus.write(b"\x15")
            while 1:
                v = self._hydrabus.read(2)
        except KeyboardInterrupt:
            self._hydrabus.write(b"\x00")
            self._hydrabus.reset()
            return True

    def frequency(self):
        """
        Read frequency value

        :return: (frequency, duty cycle)
        :rtype: tuple
        """
        self._hydrabus.write(b"\x16")
        freq = self._hydrabus.read(4)
        duty = self._hydrabus.read(4)
        return (
            int.from_bytes(freq, byteorder="little"),
            int.from_bytes(duty, byteorder="little"),
        )

    def close(self):
        """
        Close the communication channel and resets Hydrabus
        """
        self._hydrabus.exit_bbio()
        self._hydrabus.close()
