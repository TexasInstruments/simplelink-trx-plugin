#
# Copyright (c) 2022-2023, Texas Instruments Incorporated
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
#   Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import enum
import inspect
from urllib import request
import construct
from construct import (
    Adapter,
    Aligned,
    CancelParsing,
    Container,
    Default,
    Mapping,
    If,
    IfThenElse,
    Padding,
    Probe,
    RepeatUntil,
    Seek,
    Struct,
    Int8ul,
    Int32ul,
    Subconstruct,
    Switch,
    this,
    Rebuild,
    len_,
    GreedyRange,
    Select,
    Const,
    BitStruct,
    Nibble,
    Enum,
    Flag,
    BitsInteger,
    Int8sl,
    Bitwise,
    Bytewise,
    Debugger,
    Tell,
    Check,
    FocusedSeq,
    StopIf,
    SelectError,
    Padded,
)
from typing import List, Any, Tuple, Union
from dataclasses import dataclass, asdict, field
from construct.core import CheckError, Checksum, ChecksumError, Int16ub, Int16ul, Optional, RawCopy, Sequence
import binascii


class SSBLCommandID(enum.IntEnum):
    PING = 0
    GET_STATUS = 1
    GET_SEC_VER = 2
    BREAK_OUT = 3
    RESET = 4
    GET_PART_ID = 5
    GET_KEY_ID = 6
    SET_CUST_KEY = 7
    DOWNLOAD = 8
    BANK_ERASE = 9
    SEND_DATA = 10


ESSBLCommandID_ = Enum(Int8ul, SSBLCommandID)
ESSBLCommandID = Mapping(ESSBLCommandID_, {SSBLCommandID(k): v for k, v in ESSBLCommandID_.decmapping.items()})


class SSBLAckStatus(enum.IntEnum):
    BLDR_STATUS_INVALID = 0x00
    BLDR_STATUS_ACK = 0xCC
    BLDR_STATUS_NAK = 0x33


ESSBLAckStatus_ = Enum(Int8ul, SSBLAckStatus)
ESSBLAckStatus = Mapping(ESSBLAckStatus_, {SSBLAckStatus(k): v for k, v in ESSBLAckStatus_.decmapping.items()})


class Checksummed32(Subconstruct):
    r"""
    Checks or appends 32-bit CRC
    """

    def __init__(self, subcon):
        super().__init__(subcon)

    def _parse(self, stream, context, path):
        position1 = stream.tell()
        obj = self.subcon._parsereport(stream, context, path)
        position2 = stream.tell()
        stream.seek(position1, 0)
        data = stream.read(position2 - position1 + 4)
        return Container(obj, checksum=binascii.crc32(data, 0xFFFFFFFF) == 0)

    def _build(self, obj, stream, context, path):
        position1 = stream.tell()
        buildret = self.subcon._build(obj, stream, context, path)
        position2 = stream.tell()
        stream.seek(position1, 0)
        data = stream.read(position2 - position1)
        crc = binascii.crc32(data, 0x0000000000)
        stream.write(bytes([(crc >> 24) & 0xFF, (crc >> 16) & 0xFF, (crc >> 8) & 0xFF, (crc >> 0) & 0xFF]))
        return Container(buildret, checksum=crc)


SSSBLPacket = Checksummed32(Struct("len" / Int16ul, "command" / ESSBLCommandID, "payload" / Int8ul[this.len - 7]))


@dataclass
class SSBLPacket:
    command: ESSBLCommandID
    len: int = 0
    payload: List[int] = field(default_factory=list)

    __struct = SSSBLPacket

    def build(self):
        built = self.__struct.build(
            dict(
                command=self.command,
                len=self.len,
                payload=self.payload,
            )
        )
        return built

    @classmethod
    def parse(cls, bytebuf: bytes):
        try:
            tr = cls.__struct.parse(bytebuf)
        except SelectError:
            return None

        return cls(
            command=tr.command,
            len=tr.len,
            payload=tr.payload,
        )


SSSBLAck = Struct("reserved0" / Const(0, BitsInteger(1)), "status" / ESSBLAckStatus)


@dataclass
class SSBLAck:
    status: int = 0

    __struct = SSSBLAck

    def build(self):
        built = self.__struct.build(
            dict(
                status=self.status,
                len=2,
            )
        )
        return built

    @classmethod
    def parse(cls, bytebuf: bytes):
        try:
            tr = cls.__struct.parse(bytebuf)
        except SelectError:
            return None

        return cls(
            status=tr.status,
        )


if __name__ == "__main__":

    def nice(b):
        return ":".join(["%02X" % x for x in b])

    # pkt = SSBLPacket(command=SSBLCommandID.PING, len=7, payload=[])
    # print(pkt)
    # print(nice(pkt.build()))
    # print(SSBLPacket.parse(pkt.build()))
    # print(nice(SSBLPacket.parse(pkt.build()).build()))

    # print("==================")
    # print(SSBLPacket.parse(bytearray([0x07, 0x00, 0x00, 0xFA, 0x0E, 0xCF, 0x97])))
    # print(nice(SSBLPacket.parse(bytearray([0x07, 0x00, 0x00, 0xFA, 0x0E, 0xCF, 0x97])).build()))
    # print(nice([0x07, 0x00, 0x00, 0xFA, 0x0E, 0xCF, 0x97]))

    pkt = SSBLAck(status=SSBLAckStatus.BLDR_STATUS_ACK)
    print(pkt)
    print(nice(pkt.build()))
    print(SSBLAck.parse(pkt.build()))
    print(nice(SSBLAck.parse(pkt.build()).build()))

    pkt = SSBLAck(status=SSBLAckStatus.BLDR_STATUS_NAK)
    print(pkt)
    print(nice(pkt.build()))
    print(SSBLAck.parse(pkt.build()))
    print(nice(SSBLAck.parse(pkt.build()).build()))

    print("==================")
    print(SSBLAck.parse(bytearray([0x00, 0xCC])))
    print(SSBLAck.parse(bytearray([0x00, 0x33])))
