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
from typing import List, Any, Tuple, Type, Union
from dataclasses import dataclass, asdict, field
from construct.core import CheckError, Checksum, ChecksumError, Int16ub, Int16ul, Optional, RawCopy, Sequence
import binascii

from trx.trxrequests import PaddedSelectRequest, RequestMap, from_dict, nice

TRX_HDR_LEN = 4


def sum_actual_length_of_requests(ctx):
    retLen = 0
    for idx, req in enumerate(ctx._._.requests):
        retLen += len(PaddedSelectRequest.build(req))
        # Pad unless last entry
        if idx < len(ctx._._.requests) - 1:
            retLen = (retLen + 3) & ~3
    return retLen


class Checksummed(Subconstruct):
    r"""
    Checks or appends 16-bit CRC
    """

    def __init__(self, subcon):
        super().__init__(subcon)

    def _parse(self, stream, context, path):
        position1 = stream.tell()
        obj = self.subcon._parsereport(stream, context, path)
        position2 = stream.tell()
        stream.seek(position1, 0)
        data = stream.read(position2 - position1 + 2)
        return Container(obj, checksum=binascii.crc_hqx(data, 0xFFFF) == 0)

    def _build(self, obj, stream, context, path):
        position1 = stream.tell()
        buildret = self.subcon._build(obj, stream, context, path)
        position2 = stream.tell()
        stream.seek(position1, 0)
        data = stream.read(position2 - position1)
        crc = binascii.crc_hqx(data, 0xFFFF)
        stream.write(bytes([(crc >> 8) & 0xFF, (crc >> 0) & 0xFF]))
        return Container(buildret, checksum=crc)


SRequestsArray = If(
    lambda ctx: (ctx.header.get("trx", None) or ctx.header.host).len > 0, GreedyRange(PaddedSelectRequest)
)


@dataclass
class AckState:
    tx: int
    rx: int


SHostHeader = Struct(
    "res0" / Int8ul,  # type:ignore
    "len" / Rebuild(Int8ul, sum_actual_length_of_requests),
    "ack"
    / BitStruct(
        "rx" / Nibble,  # type:ignore
        "tx" / Nibble,  # type:ignore
    ),
    "res1" / Int8ul,  # type:ignore
)


@dataclass
class HostHeader:
    res0: int = 0
    res1: int = 0
    len: int = 0
    ack: AckState = field(default_factory=lambda: AckState(0, 0))

    def __post_init__(self):
        if isinstance(self.ack, tuple):
            self.ack = AckState(*(self.ack))

    def __repr__(self):
        if self.len == 0 and self.ack == AckState(0, 0):
            return f"{type(self).__name__}()"
        return f"{type(self).__name__}(len={self.len}, sent={self.ack.tx}, ack={self.ack.rx})"

    @classmethod
    def from_container(cls, c):
        c = c.host if hasattr(c, "host") else c
        return cls(len=c.len, ack=AckState(tx=c.ack.tx, rx=c.ack.rx))


STrxHeader = BitStruct(
    "chip_rdy_n" / Const(0, BitsInteger(1)),
    "data_rdy_n" / Rebuild(BitsInteger(1), lambda ctx: 1 if 0 == len(ctx._._.requests) else 0),
    "error" / BitsInteger(1),
    "reserved0" / Default(BitsInteger(1), 0),
    "free" / Default(Nibble, 0),
    "len" / Rebuild(Bytewise(Int8ul), sum_actual_length_of_requests),
    "ack"
    / Struct(
        "rx" / Default(Nibble, 0),
        "tx" / Default(Nibble, 0),
    ),
    "status0" / Default(Bytewise(Int8ul), 0),
)


@dataclass
class TrxHeader:
    chip_rdy_n: int = 1
    data_rdy_n: int = 1
    error: int = 0
    free: int = 0
    status0: int = 0
    len: int = 0
    ack: AckState = field(default_factory=lambda: AckState(0, 0))

    def __post_init__(self):
        if isinstance(self.ack, tuple):
            self.ack = AckState(*(self.ack))

    def __repr__(self):
        return f"{type(self).__name__}(len={self.len}, error={self.error}, sent_id={self.ack.tx}, ack_id={self.ack.rx}, free={self.free})"

    @classmethod
    def from_container(cls, c):
        c = c.trx if hasattr(c, "trx") else c
        return cls(
            chip_rdy_n=c.chip_rdy_n,
            data_rdy_n=c.data_rdy_n,
            error=c.error,
            free=c.free,
            status0=c.status0,
            len=c.len,
            ack=AckState(tx=c.ack.tx, rx=c.ack.rx),
        )


STransaction = Checksummed(
    Struct(
        "header"
        / construct.Union(
            0,
            "host" / SHostHeader,
            "trx" / STrxHeader,
        ),
        "requests" / SRequestsArray,
    )
)


@dataclass
class TrxTransaction:
    header: Union[TrxHeader, HostHeader]
    requests: List[Any] = field(default_factory=list)
    checksum: bool = True
    min_len: int = 0
    __struct = STransaction

    def build(self):
        is_trxheader = isinstance(self.header, TrxHeader)
        header = {"trx" if is_trxheader else "host": asdict(self.header)}

        built = self.__struct.build(dict(header=header, requests=[asdict(x) for x in self.requests]))
        return built + bytes([0] * max(0, self.min_len - len(built)))

    @classmethod
    def parse(cls, headerclass: Union[Type[TrxHeader], Type[HostHeader]], bytebuf: bytes) -> "TrxTransaction":
        try:
            tr: Container = cls.__struct.parse(bytebuf)  # type: ignore
        except Exception as e:
            print("!!!!!!! Failed parsing:", e)
            print("!!!!!!! Input was:", nice(bytebuf))
            raise IOError(str(e))

        try:
            return cls(
                header=headerclass.from_container(tr.header),
                requests=[from_dict(RequestMap[r.id], r) for r in (tr.requests or [])],
                checksum=tr.checksum,
            )
        except Exception as e:
            print("Failed casting transaction content to python class")
            print("Transaction container:")
            print(tr)
            raise IOError(str(e))
