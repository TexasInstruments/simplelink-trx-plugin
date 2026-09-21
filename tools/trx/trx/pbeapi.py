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

from collections import defaultdict
import enum
from functools import lru_cache
import typing
from xmlrpc.client import Boolean
from construct import (
    Bytewise,
    Container,
    Default,
    GreedyRange,
    Mapping,
    Optional,
    Padding,
    Probe,
    Rebuild,
    Struct,
    BitStruct,
    BitsInteger,
    Enum,
    Flag,
    SelectError,
    Union,
    len_,
    this,
)
from dataclasses import InitVar, dataclass, field, asdict
from construct.core import Int16ul, Int8ul, Int8sl, Int32ul
from typing import Any, ByteString, ClassVar, List, Type
from trx.trxrequests import StreamStore, HardcodedPhy, from_dict, nice
from crc import Calculator, Configuration


class FCS(enum.IntFlag):
    CRC32 = 0
    CRC16 = 1


class Modulation(enum.IntEnum):
    FSK = 0
    OFDM = 1
    LORA = 2  # CC1308 only
    OQPSK = 2  # CC140x only


class OQpskRateMode(enum.IntEnum):
    DR_6_25kbps = 0
    DR_12_5kbps = 1
    DR_25kbps = 2
    DR_50kbps = 3


EModulation = Mapping(
    Enum(BitsInteger(2), Modulation), {Modulation(k): v for k, v in Enum(BitsInteger(2), Modulation).decmapping.items()}
)
EFCS = Mapping(Enum(BitsInteger(1), FCS), {FCS(k): v for k, v in Enum(BitsInteger(1), FCS).decmapping.items()})
EOQpskRateMode = Mapping(
    Enum(BitsInteger(2), OQpskRateMode),
    {OQpskRateMode(k): v for k, v in Enum(BitsInteger(2), OQpskRateMode).decmapping.items()},
)
SInternalPacketParams = Union(
    0,
    "sunfsk"
    / BitStruct(
        "modulation" / EModulation,
        Padding(3),
        "mode_switch" / Flag,  # type:ignore
        "fcs" / EFCS,
        "whitening" / Flag,  # type:ignore
        "phy_id" / Default(Bytewise(Int8ul), 0),
    ),
    "common"
    / BitStruct(
        "modulation" / EModulation,
        Padding(6),
        Padding(8),
    ),
    "sunofdm"
    / BitStruct(
        "modulation" / EModulation,
        "scrambler" / BitsInteger(2),
        "rate" / BitsInteger(4),
        "phy_id" / Default(Bytewise(Int8ul), 0),
    ),
    "sunoqpsk"
    / BitStruct(
        "modulation" / EModulation,
        Padding(4),
        "rate_mode" / EOQpskRateMode,
        Padding(8),
    ),
    "ofdmlr"
    / BitStruct(
        "modulation" / EModulation,
        Padding(4),
        "rate" / BitsInteger(2),
        Padding(8),
    ),
    "lora"
    / BitStruct(
        "modulation" / EModulation,
        Padding(3),
        "rate" / BitsInteger(3),
        Padding(8),
    ),
    "ieee"
    / BitStruct(
        "modulation" / EModulation,
        Padding(4),
        "fcs" / EFCS,
        "whitening" / Flag,  # type:ignore
        Padding(8),
    ),
)


@dataclass
class CommonParams:
    modulation: Modulation = Modulation.FSK
    _field: InitVar[str] = "common"


@dataclass
class SunFSKParams:
    modulation: Modulation = Modulation.FSK
    mode_switch: bool = False
    fcs: FCS = FCS.CRC16
    whitening: bool = True
    phy_id: int = 0
    _field: InitVar[str] = "sunfsk"


@dataclass
class SunOFDMParams:
    modulation: Modulation = Modulation.OFDM
    scrambler: int = 0
    rate: int = 2
    phy_id: int = 0
    _field: InitVar[str] = "sunofdm"


@dataclass
class SunOQPSKParams:
    modulation: Modulation = Modulation.OQPSK
    rate_mode: OQpskRateMode = OQpskRateMode.DR_12_5kbps
    _field: InitVar[str] = "sunoqpsk"


@dataclass
class OFDMLRParams:
    modulation: Modulation = Modulation.OFDM
    rate: int = 2
    _field: InitVar[str] = "ofdmlr"


@dataclass
class LoraParams:
    modulation: Modulation = Modulation.LORA
    rate: int = 1
    _field: InitVar[str] = "lora"


@dataclass
class IEEEParams:
    modulation: Modulation = Modulation.FSK
    fcs: FCS = FCS.CRC32
    whitening: bool = True
    _field: InitVar[str] = "ieee"


SPhyPacket = Struct(
    "length" / Rebuild(Int16ul, len_(this.data)),
    "params" / SInternalPacketParams,
    "data" / Int8ul[this.length],  # type:ignore
    "rssi" / Optional(Int8sl),
    "timestamp" / Optional(Int32ul),
)


@dataclass
class PhyPacket:
    params: typing.Union[
        CommonParams, SunFSKParams, SunOFDMParams, SunOQPSKParams, OFDMLRParams, LoraParams, IEEEParams
    ] = field(default_factory=SunFSKParams)
    data: List[int] = field(default_factory=list)
    rssi: int = 0
    timestamp: int = 0
    length: int = -1
    __struct = SPhyPacket

    def __post_init__(self):
        if self.length == -1:
            self.length = len(self.data)

    @classmethod
    def parse(
        cls,
        bytebuf: typing.Union[ByteString, List[int]],
        param_type: typing.Union[
            Type[CommonParams],
            Type[SunFSKParams],
            Type[SunOFDMParams],
            Type[SunOQPSKParams],
            Type[OFDMLRParams],
            Type[LoraParams],
            Type[IEEEParams],
        ] = CommonParams,
    ):
        if isinstance(bytebuf, list):
            bytebuf = bytes(bytebuf)
        ip: Container = cls.__struct.parse(bytebuf)  # type:ignore

        return cls(
            length=ip.length,
            params=from_dict(param_type, ip.params[param_type._field]),  # type:ignore
            data=ip.data,
            rssi=ip.rssi,
            timestamp=ip.timestamp,
        )

    def build(self):
        paramsdict = dict(common=asdict(self.params))
        if isinstance(self.params, SunFSKParams):
            paramsdict = dict(sunfsk=asdict(self.params))
        if isinstance(self.params, SunOFDMParams):
            paramsdict = dict(sunofdm=asdict(self.params))
        if isinstance(self.params, SunOQPSKParams):
            paramsdict = dict(sunoqpsk=asdict(self.params))
        if isinstance(self.params, OFDMLRParams):
            paramsdict = dict(ofdmlr=asdict(self.params))
        if isinstance(self.params, LoraParams):
            paramsdict = dict(lora=asdict(self.params))
        if isinstance(self.params, IEEEParams):
            paramsdict = dict(ieee=asdict(self.params))

        return self.__struct.build(dict(params=paramsdict, data=self.data))

    def as_store_requests(self, stream_id, retention=0):
        # max length of data in StreamStore Request: 128 - 4 (transaction header) - 4 (streamStore request header) - 2 (CRC)
        max_request_length = 128 - 4 - 4 - 2

        # Number of bytes packed into StreamStore Requests
        data_packed = 0
        fragments = []

        packet = self.build()

        while data_packed < len(packet):
            fragment_size = min(len(packet) - data_packed, max_request_length)
            fragments.append(packet[data_packed : data_packed + fragment_size])
            data_packed += fragment_size

        return [
            StreamStore(
                stream_id=stream_id,
                start=1 if idx == 0 else 0,
                md=1 if idx + 1 < len(fragments) else 0,
                retention=retention,
                data=fragment,
            )
            for idx, fragment in enumerate(fragments)
        ]

    def __repr__(self):
        return f"{type(self).__name__}(length={self.length:02d}, params={self.params}, data={nice(self.data)}, rssi={self.rssi}, timestamp={self.timestamp})"


SPropPacket = Struct(
    "length" / Rebuild(Int8ul, len_(this.data)),
    "data" / Int8ul[this.length],  # type:ignore
    "rssi" / Optional(Int8sl),
    "timestamp" / Optional(Int32ul),
)


@dataclass
class PropPacket:
    data: List[int] = field(default_factory=list)
    rssi: int = 0
    timestamp: int = 0
    length: int = -1
    __struct = SPropPacket

    def __post_init__(self):
        if self.length == -1:
            self.length = len(self.data)

    @classmethod
    def parse(cls, bytebuf):
        if isinstance(bytebuf, list):
            bytebuf = bytes(bytebuf)
        ip: Container = cls.__struct.parse(bytebuf)  # type:ignore

        return cls(
            length=ip.length,
            data=ip.data,
            rssi=ip.rssi,
            timestamp=ip.timestamp,
        )

    def build(self):
        return self.__struct.build(
            dict(
                data=self.data,
            )
        )

    def as_store_requests(self, stream_id, retention=0):
        # max length of data in StreamStore Request: 128 - 4 (transaction header) - 4 (streamStore request header) - 2 (CRC)
        max_request_length = 128 - 4 - 4 - 2

        # Number of bytes packed into StreamStore Requests
        data_packed = 0
        fragments = []

        packet = self.build()

        while data_packed < len(packet):
            fragment_size = min(len(packet) - data_packed, max_request_length)
            fragments.append(packet[data_packed : data_packed + fragment_size])
            data_packed += fragment_size

        return [
            StreamStore(
                stream_id=stream_id,
                start=1 if idx == 0 else 0,
                md=1 if idx + 1 < len(fragments) else 0,
                retention=retention,
                data=fragment,
            )
            for idx, fragment in enumerate(fragments)
        ]

    def __repr__(self):
        return f"{type(self).__name__}(length={self.length:02d}, data={nice(self.data)}, rssi={self.rssi}, timestamp={self.timestamp})"


SLoraInternalPacket = Struct(
    "length" / Rebuild(Int16ul, len_(this.data)),
    "params" / SInternalPacketParams,
    "data" / Int8ul[this.length],  # type:ignore
    "rssi" / Optional(Int8sl),
    "timestamp" / Optional(Int32ul),
)


@dataclass
class LoraInternalPacket:
    length: int
    modulation: Modulation
    rate: int
    data: List[int] = field(default_factory=list)
    rssi: int = 0
    timestamp: int = 0
    __struct = SLoraInternalPacket

    @classmethod
    def parse(cls, bytebuf):
        if isinstance(bytebuf, list):
            bytebuf = bytes(bytebuf)
        ip: Container = cls.__struct.parse(bytebuf)  # type:ignore

        return cls(
            length=ip.length,
            modulation=ip.params.lora.modulation,
            rate=ip.params.lora.rate,
            data=ip.data,
            rssi=ip.rssi,
            timestamp=ip.timestamp,
        )

    def build(self):
        return self.__struct.build(
            dict(
                params=dict(
                    lora=dict(
                        length=self.length,
                        modulation=self.modulation,
                        rate=self.rate,
                    ),
                ),
                data=self.data,
            )
        )

    def __repr__(self):
        return f"{type(self).__name__}(length={self.length:02d}, modulation={str(self.modulation)}, rate={self.rate}, data={nice(self.data)}, rssi={self.rssi}, timestamp={self.timestamp})"


# This packet looks a bit different compared to LoraInternalPacket, PhyPacket and PropPacket
# because it is not possible to match its payload fields as a C structure as it has a
# varying length.
@dataclass
class WMBusPacket:
    rssi: int = 0
    timestamp: int = 0

    # Class-level mapping of HardcodedPhy to frame formats
    _PHY_TO_FRAME_FORMAT = {
        # FFA format PHYs
        HardcodedPhy.WMBus_Tmode_3of6Encoding: "FFA",
        HardcodedPhy.WMBus_Smode_ManchesterEncoding: "FFA",
        HardcodedPhy.Wmbus_Cmode_M2O_FFA: "FFA",
        HardcodedPhy.Wmbus_Cmode_O2M_FFA: "FFA",
        # FFB format PHYs
        HardcodedPhy.Wmbus_Cmode_M2O_FFB: "FFB",
        HardcodedPhy.Wmbus_Cmode_O2M_FFB: "FFB",
    }

    def __init__(self, phy: HardcodedPhy, data: List[int]):
        """
        Initialize WMBusPacket with either a HardcodedPhy enum or frame format string.

        Args:
            phy_or_format: Either a HardcodedPhy enum or frame format string ("FFA" or "FFB")
            data: List of integers representing the payload data
        """
        self.data = data.copy()  # Make a copy to avoid modifying the original

        # Determine frame format
        if isinstance(phy, HardcodedPhy):
            if phy not in self._PHY_TO_FRAME_FORMAT:
                raise ValueError(f"Unsupported WMBus PHY: {phy}")
            self.frame_format = self._PHY_TO_FRAME_FORMAT[phy]
            self.phy = phy

        else:
            raise ValueError("phy_or_format must be either HardcodedPhy enum or string")

        self.packet = self.create_packet()

    def create_packet(self):
        if self.frame_format == "FFA":
            return self.create_ffa_packet()
        elif self.frame_format == "FFB":
            return self.create_ffb_packet()
        else:
            raise ValueError("Invalid frame format")

    def create_ffa_packet(self):
        userPayload = self.data.copy()  # Work with a copy as we are using pop() later
        buffer = [0] * 12

        if len(userPayload) > 246:
            raise ValueError("Maximum num bytes to send is 246")
        buffer[0] = 9 + len(userPayload)

        configCrc = Configuration(
            width=16,
            polynomial=0x3D65,
            init_value=0x0000,
            final_xor_value=0xFFFF,
            reverse_input=False,
            reverse_output=False,
        )

        calculator = Calculator(configCrc, optimized=True)

        crc = calculator.checksum(bytes(buffer[0:10]))

        buffer[10] = (crc & 0xFF00) >> 8
        buffer[11] = crc & 0xFF

        if len(userPayload) == 0:
            raise ValueError("At least one byte is required")
        else:
            # Complete blocks
            while userPayload:
                limit = min(16, len(userPayload))

                for i in range(limit):
                    buffer.append(userPayload.pop(0))

                crc = calculator.checksum(bytes(buffer[-limit:]))

                buffer.append((crc & 0xFF00) >> 8)
                buffer.append(crc & 0xFF)

        return buffer

    def create_ffb_packet(self):
        userPayload = self.data[:256]  # 256 bytes max as per Frame Format Definition
        buffer = [0] * 10
        numCrcs = 2 if len(userPayload) > 116 else 1
        buffer[0] = 9 + len(userPayload) + numCrcs * 2
        iters = 0

        configCrc = Configuration(
            width=16,
            polynomial=0x3D65,
            init_value=0x0000,
            final_xor_value=0xFFFF,
            reverse_input=False,
            reverse_output=False,
        )

        calculator = Calculator(configCrc, optimized=True)
        additionalBlockRequired = False
        crc = 0

        # Complete blocks
        while userPayload:
            buffer.append(userPayload.pop(0))
            iters += 1
            if iters == 116 and userPayload != []:
                crc = calculator.checksum(bytes(buffer))
                buffer.append((crc & 0xFF00) >> 8)
                buffer.append(crc & 0xFF)
                additionalBlockRequired = True
            elif iters == 256:
                break

        if additionalBlockRequired == False:
            crc = calculator.checksum(bytes(buffer[:]))
        else:
            crc = calculator.checksum(bytes(buffer[128:]))

        buffer.append((crc & 0xFF00) >> 8)
        buffer.append(crc & 0xFF)

        return buffer

    def build(self, rx_format=False):
        # Modem prepends the first (automatic) byte indicating type of frame format at the start of payload at RX
        if rx_format:
            if (
                self.phy == HardcodedPhy.WMBus_Tmode_3of6Encoding
                or self.phy == HardcodedPhy.WMBus_Smode_ManchesterEncoding
            ):
                self.packet.insert(0, 0xAA)
            elif self.phy == HardcodedPhy.Wmbus_Cmode_M2O_FFA or self.phy == HardcodedPhy.Wmbus_Cmode_O2M_FFA:
                self.packet.insert(0, 0xCD)
            elif self.phy == HardcodedPhy.Wmbus_Cmode_M2O_FFB or self.phy == HardcodedPhy.Wmbus_Cmode_O2M_FFB:
                self.packet.insert(0, 0x3D)

        return self.packet


if __name__ == "__main__":
    tx_packet = PhyPacket(SunFSKParams(), data=[x % 0xFF for x in range(2048)])
    reqs = tx_packet.as_store_requests(stream_id=31)
    print(reqs)
    # print(nice(tx_packet.build()))
    # print(PhyPacket.parse(tx_packet.build(), SunFSKParams))

    # data_length=1
    # p = PhyPacket(SunFSKParams(whitening=True, fcs=FCS.CRC16), data=[x % 0xFF for x in range(data_length)])
    # print(nice(p.build()))
    # p = PhyPacket(SunFSKParams(whitening=False, fcs=FCS.CRC32, phy_id=14), data=[x % 0xFF for x in range(data_length)])
    # print(nice(p.build()))
    # print(PhyPacket.parse(p.build(), SunFSKParams))

    # # print(PhyPacket.parse(p.build(), SunFSKParams))

    # print(PhyPacket.parse(bytes([1, 0, 2, 14, 0, 217, 182, 181, 191, 1]), SunFSKParams))

    # print(SInternalPacketParams.parse(bytes([1,0,2, 0])))
