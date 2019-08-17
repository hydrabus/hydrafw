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


def split(seq, length):
    """
    Split a list in chunks of specific length
    """
    return [seq[i : i + length] for i in range(0, len(seq), length)]


def set_bit(byte, bit, position):
    v = ord(byte)
    if bit == 1:
        v = v | (1 << position)
    elif bit == 0:
        v = v & (~(1 << position)) & 0xFF
    else:
        raise ValueError("Bit must be 0 or 1")
    return v.to_bytes(1, byteorder="big")
