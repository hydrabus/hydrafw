import logging

from .common import set_bit


class AUXPin:
    """
    Auxilary pin base class

    This class is meant to be used in any mode and is instanciated in the
    Protocol class

    :example:

    >>> import pyHydrabus
    >>> i=pyHydrabus.RawWire('/dev/hydrabus')
    >>> # Set AUX pin 0 (PC4) to output
    >>> i.AUX[0].direction = pyHydrabus.OUTPUT
    >>> # Set AUX pin to logical 1
    >>> i.AUX[0].value = 1
    >>> # Read Auxiliary pin 2 (PC5) value
    >>> i.AUX[1].value

    """

    OUTPUT = 0
    INPUT = 1

    def __init__(self, number, hydrabus):
        """
        AUXPin constructor

        :param number: Auxilary pin number (0-3)
        :param hydrabus: Initialized pyHydrabus.Hydrabus instance
        """
        self.number = number
        self._hydrabus = hydrabus
        self._logger = logging.getLogger(__name__)

    def _get_config(self):
        """
        Gets Auxiliary pin config from Hydrabus

        :return: Auxilary pin config (4 bits pullup for AUX[0-3], 4 bits value for AUX[0-3])
        :rtype: byte
        """
        CMD = 0b11100000
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        return self._hydrabus.read(1)

    def _get_values(self):
        """
        Gets Auxiliary pin values from Hydrabus

        :return: Auxilary pin values AUX[0-3]
        :rtype: byte
        """
        CMD = 0b11000000
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        return self._hydrabus.read(1)

    @property
    def value(self):
        """
        Auxiliary pin getter
        """
        return (ord(self._get_values()) >> self.number) & 0b1

    @value.setter
    def value(self, value):
        """
        Auxiliary pin setter

        :param value: auxiliary pin value (0 or 1)
        """
        CMD = 0b11010000
        CMD = CMD | ord(set_bit(self._get_values(), value, self.number))
        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        if self._hydrabus.read(1) == b"\x01":
            return
        else:
            self._logger.error("Error setting auxiliary pins value.")

    def toggle(self):
        """
        Toggle pin state
        """
        self.value = not self.value

    @property
    def direction(self):
        """
        Auxiliary pin direction getter

        :return: The pin direction (0=output, 1=input)
        :rtype: int
        """
        return (ord(self._get_config()) >> self.number) & 0b1

    @direction.setter
    def direction(self, value):
        """
        Auxiliary pin direction setter

        :param value: The pin direction (0=output, 1=input)
        """
        CMD = 0b11110000
        PARAM = self._get_config()
        PARAM = set_bit(PARAM, value, self.number)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(PARAM)
        if self._hydrabus.read(1) == b"\x01":
            return
        else:
            self._logger.error("Error setting auxiliary pins direction.")

    @property
    def pullup(self):
        """
        Auxiliary pin pullup getter

        :return: Auxiliary pin pullup status (1=enabled, 0=disabled")
        :rtype: int
        """
        return (ord(self._get_config()) >> (4 + self.number)) & 0b1

    @pullup.setter
    def pullup(self, value):
        """
        Auxiliary pin pullup setter

        :param value: Auxiliary pin pullup (1=enabled, 0=disabled")
        """
        CMD = 0b11110000
        PARAM = self._get_config()
        PARAM = set_bit(PARAM, value, 4 + self.number)

        self._hydrabus.write(CMD.to_bytes(1, byteorder="big"))
        self._hydrabus.write(PARAM)
        if self._hydrabus.read(1) == b"\x01":
            return
        else:
            self._logger.error("Error setting auxiliary pins value.")
