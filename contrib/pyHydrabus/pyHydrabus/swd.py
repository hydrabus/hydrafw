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
from .rawwire import RawWire


class SWD:
    """
    SWD protocol handler

    :example:

    >>> import pyHydrabus
    >>> swd = pyHydrabus.SWD('/dev/ttyACM0')
    >>> swd.bus_init()
    >>> swd.read_dp(0)
    >>> swd.write_dp(4, 0x50000000)
    >>> swd.scan_bus()

    """

    def __init__(self, port=""):
        self._interface = RawWire(port)

        self._interface._config = 0xA
        self._interface._configure_port()

    def __del__(self):
        self.close()

    def _apply_dp_parity(self, value):
        tmp = value & 0b00011110
        if (bin(tmp).count("1") % 2) == 1:
            value = value | 1 << 5
        return value

    def close(self):
        self._interface.close()

    def _sync(self):
        self._interface.write(b"\x00")

    def bus_init(self):
        """
        Initiate SWD bus.
        Sends the JTAG-TO-SWD token and sync clocks
        """
        self._interface.write(
            b"\xff\xff\xff\xff\xff\xff\x7b\x9e\xff\xff\xff\xff\xff\xff\x0f"
        )
        self._sync()

    def read_dp(self, addr, to_ap=0):
        """
        Read a register from DP

        :param addr: DP register address
        :type addr: int

        :return: Value stored in register
        :rtype: int

        :example:
        >>> # read RDBUFF
        >>> swd.read_dp(0xc)
        """
        CMD = 0x85
        CMD = CMD | to_ap << 1
        CMD = CMD | (addr & 0b1100) << 1
        CMD = self._apply_dp_parity(CMD)

        self._interface.write(CMD.to_bytes(1, byteorder="little"))
        status = 0
        for i in range(3):
            status += ord(self._interface.read_bit()) << i
        if status == 1:
            retval = int.from_bytes(self._interface.read(4), byteorder="little")
            self._sync()
            return retval
        elif status == 2:
            # When receiving WAIT, retry transaction
            self._sync()
            self.write_dp(0, 0x0000001F)
            return self.read_dp(addr, to_ap)
        else:
            self._sync()
            raise ValueError(f"Returned status is {hex(status)}")

    def write_dp(self, addr, value, to_ap=0):
        """
        Write to DP register

        :param addr: DP register address
        :type addr: int
        :param value: Value to be written to register
        :type value: int

        :example:
        >>> write_dp(4, 0x50000000)
        """
        CMD = 0x81
        CMD = CMD | to_ap << 1
        CMD = CMD | (addr & 0b1100) << 1
        CMD = self._apply_dp_parity(CMD)

        self._interface.write(CMD.to_bytes(1, byteorder="little"))
        status = 0
        for i in range(3):
            status += ord(self._interface.read_bit()) << i
        self._interface.clocks(2)
        if status == 2:
            # When receiving WAIT, retry transaction
            self._sync()
            self.write_dp(0, 0x0000001F)
            return self.write_dp(addr, value, to_ap)
        if status != 1:
            self._sync()
            raise ValueError(f"Returned status is {hex(status)}")
        self._interface.write(value.to_bytes(4, byteorder="little"))

        # Send the parity but along with the sync clocks
        if (bin(value).count("1") % 2) == 1:
            self._interface.write(b"\x01")
        else:
            self._interface.write(b"\x00")

    def read_ap(self, address, bank):
        """
        Read AP register

        :param address: AP address on the bus
        :type address: int
        :param bank: AP register address
        :type bank: int

        :return: Value read from AP
        :rtype: int

        :example:
        >>> # Read AP IDR
        >>> read_ap(0, 0xfc)

        """

        select_reg = 0
        # Place AP address in DP SELECT register
        select_reg = select_reg | address << 24
        # Place bank in register as well
        select_reg = select_reg | (bank & 0b11110000)
        # Write the SELECT DP register
        self.write_dp(8, select_reg)
        self.read_dp((bank & 0b1100), to_ap=1)
        # Read RDBUFF
        return self.read_dp(0xC)

    def write_ap(self, address, bank, value):
        """
        Write to AP register

        :param address: AP address on the bus
        :type address: int
        :param bank: AP register address
        :type bank: int
        :param value: Value to be written to register
        :type value: int

        :example:
        >>> write_ap(0, 0x4, 0x20000000)
        """

        select_reg = 0
        # Place AP address in DP SELECT register
        select_reg = select_reg | address << 24
        # Place bank in register as well
        select_reg = select_reg | (bank & 0b11110000)
        # Write the SELECT DP register
        self.write_dp(8, select_reg)
        # Send the actual value to the AP
        self.write_dp((bank & 0b1100), value, to_ap=1)

    def scan_bus(self):
        """
        Scan the SWD bus for APs
        The SWD bus must have been enabled before using this command.
        """

        for ap in range(256):
            idr = self.read_ap(ap, 0xFC)
            if idr != 0x0 and idr != 0xFFFFFFFF:
                print(f"0x{ap:02x}: 0x{idr:08x}")
