#
# Copyright (c) 2022-2026, Texas Instruments Incorporated
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

import os
import enum
import inspect
import typing
from collections import defaultdict
from dataclasses import asdict, dataclass, field, is_dataclass
from struct import pack, unpack
from typing import (
    Any,
    Dict,
    List,
    MutableMapping,
    MutableSequence,
    Optional,
    Sequence,
    Tuple,
    TypeVar,
    Union,
    get_args,
    get_origin,
)

import construct
from construct import (
    Aligned,
    BitsInteger,
    BitStruct,
    Bitwise,
    Bytewise,
    Check,
    Computed,
    Const,
    Container,
    Debugger,
    Enum,
    Flag,
    FocusedSeq,
    GreedyRange,
    Int8sl,
    Int8ul,
    Int16ul,
    Int16ub,
    Int32ul,
    Int32ub,
    ListContainer,
    Mapping,
    Nibble,
    Padding,
    Probe,
    Rebuild,
    Select,
    SelectError,
    StopIf,
    Struct,
    Switch,
    Tell,
    bits2bytes,
    len_,
    this,
)
from construct.core import BytesInteger, ByteSwapped


def nice(b):
    return ":".join(["%02X" % x for x in b])


T = TypeVar("T")


def from_dict(
    cls: T, definition: dict, /, type_map: Union[Dict[str, type], None] = None, _path: Union[List, None] = None
) -> T:
    """Create an instance of a dataclass from dictionary.

    Requested class can have nested members whose parameters will be processed
    as well, as long as those members are decorated as dataclasses. Dictionary
    has to match the class structure exactly, in that:
     * keys have to match parameter names
     * dataclass member values have to be dictionaries matching those members
       class structures.
     * unions have to be explicitly mapped to a type using type_map in form of
       {'path.to.param': type}

    Example ClassA and corresponding dictionary dictA and type_map mapA:

    ```
    @dataclass
    class SubA:
        val: Union[int, str]

    @dataclass
    class ClassA:
            a: SubA
            b: int

    mapA = {'a.val': int}

    dictA = dict(
        a = dict(val = 4),
        b = 'K'
    )
    ```

    :param cls: Requested dataclass to be constructed
    :param definition: Class instance definition as a dictionary
    :param type_map: Map of possibly nested dict fields whose type cannot be
                     resolved implicitly
    :param _path: Internal. Used to keep track of absolute path to parameter

    :returns: Constructed instance of requested class.
    """
    if type_map is None:
        type_map = dict()
    path = [] if _path is None else _path
    is_list = False

    # Remove keys from the dictionary that don't have class parameter equivalents
    if not hasattr(definition, "items"):
        raise TypeError(f"Was expecting a container to initialize {cls!r}, got {definition!r}")
    trimmed_def = {k: v for k, v in definition.items() if k in inspect.signature(cls.__init__).parameters}

    cast_dict = {}
    # Iterate over parameters as defined in dictionary
    for key, val in trimmed_def.items():
        # Update the parameter with a full path from root class
        field_path = ".".join(path + [key])
        # Get the type of processed parameter
        annotation = inspect.signature(cls.__init__).parameters[key].annotation
        # Check if the parameter has been explicitly cast in type_map. If not, use the type extracted from signature
        field_type = type_map.get(field_path, annotation)

        # Unions can't be resolved without explicit mapping.
        if get_origin(field_type) is Union:
            # Maybe the class has a __resolve_child_type function, that would be helpful
            resolve = getattr(cls, f'_{getattr(cls, "__name__")}__resolve_child_type', None)
            field_type = None if resolve is None else resolve(definition, key)
            if field_type is None:
                raise TypeError(
                    f'Got unmapped union type. Add class map dict("{field_path}"=SomeDataClass) to invocation'
                )

        elif get_origin(field_type) is list:
            is_list = True
            (field_type,) = get_args(field_type)

        # If needed, call recursively for complex parameters.
        # This requires nested classes to be marked as dataclasses.
        if is_dataclass(field_type):
            if is_list:
                cast_dict[key] = [from_dict(field_type, x, type_map, path + [key]) for x in val]
            else:
                cast_dict[key] = from_dict(field_type, val, type_map, path + [key])
        else:
            cast_dict[key] = val

    # Construct requested class using processed dictionary
    return cls(**cast_dict)  # type:ignore


class BaseRequest:
    id: int


class Request(enum.IntEnum):
    INVALID = 0
    LAST_STATUS = 1
    TRANSPORT_ERROR = 2
    COMMAND_STORE = 16
    COMMAND_SUBMIT = 17
    COMMAND_STOP = 18
    COMMAND_STATUS = 19
    STREAM_STORE = 32
    STREAM_FLUSH = 33
    CONFIG_STORE = 48
    CONFIG_PERSIST = 49
    CONFIG_LIST = 50
    CONFIG_LIST_RESPONSE = 51
    CONFIG_FLUSH = 52
    DEVICE_CONFIG_STORE = 56
    DEVICE_CONFIG_PERSIST = 57
    DEVICE_CONFIG_GET = 58
    DEVICE_CONFIG_GET_RESPONSE = 59
    UTIL_GETTIME = 64
    UTIL_GETRSSI = 65
    UTIL_INFO_VERSION = 66
    UTIL_INFO_MAC_UUID = 68
    UTIL_INFO_DEVICE = 69
    UTIL_ENTER_SSBL = 70
    UTIL_POWER_MODE = 71
    UTIL_RF_MODE = 72
    UTIL_SHUTDOWN = 73
    NV_ALLOCATE = 80
    NV_WRITE = 81
    NV_FINALIZE = 82
    NV_INVALIDATE = 83
    NV_GET = 84
    NV_INFO = 85
    NV_ERASE = 86
    NV_CORRUPT = 87
    DIO_SETUP = 91
    PING = 127


DEVICE_PHY_GROUP = "NOT_CONFIGURED"

# Enum for currently hardcoded PHYs
# "enum.auto()" is added to get unique values
# (slot+option mask is not necessarily unique alone)
class HardcodedPhy(enum.Enum):
    # All supported PHYs
    SUNFSK1a = 0
    SUNFSK1b = 1
    SUNFSK2a = 2
    SUNFSK2b = 3
    SUNFSK4b = 4
    SUNOFDMo1 = 5
    SUNOFDMo2 = 6
    SUNOFDMo3MCS0 = 7
    SUNOFDMo3MCS16 = 8
    SUNOFDMo4MCS01 = 9
    SUNOFDMo4MCS26 = 10
    LoraSF11 = 11
    LoraSF7 = 12
    OFDMLR = 13
    SidewalkFSK50kbps = 14
    SidewalkFSK150kbps = 15
    SidewalkFSK250kbps = 16
    LoraSF11_125kbps = 17
    LoraSF7_125kbps = 18
    PROPFSK50kbps = 19
    PROPOOK4_8kbps = 20
    SUNOQPSK = 21
    XLR_20Ksps_Dss2 = 22
    XLR_20Ksps_Dss4 = 23
    XLR_20Ksps_Dss8 = 24
    WMBus_Tmode_3of6Encoding = 25
    WMBus_Smode_ManchesterEncoding = 26
    Wmbus_Cmode_M2O_FFB = 27
    Wmbus_Cmode_O2M_FFB = 28
    Wmbus_Cmode_M2O_FFA = 29
    Wmbus_Cmode_O2M_FFA = 30
    PROPFSK9p6kbps = 31
    PROPFSK38p4kbps = 32
    PROPFSK50kbps_LowPower = 33
    PROPFSK50kbps_GeneralPurpose = 34
    DUALRX_SUNOQPSK_PROPFSK38p4kbps = 35
    DUALRX_SUNOQPSK_PROPFSK50kbps = 36
    DUALRX_SUNOFDM_O3_PROPFSK50kbps = 37

    def set_device(device: str):
        global DEVICE_PHY_GROUP
        DEVICE_PHY_GROUP = device

    @property
    def selector(self):
        global DEVICE_PHY_GROUP
        # Tyr PHYs
        if DEVICE_PHY_GROUP == "CC130X":
            if self == HardcodedPhy.SUNFSK2b:
                return PhySelector(1, 0x0000)
            elif self == HardcodedPhy.SUNFSK4b:
                return PhySelector(1, 0x0001)
            elif self == HardcodedPhy.SUNOFDMo2:
                return PhySelector(1, 0x0000)
            elif self == HardcodedPhy.SUNOFDMo3MCS0:
                return PhySelector(1, 0x0002)
            elif self == HardcodedPhy.SUNOFDMo3MCS16:
                return PhySelector(1, 0x0002)
            elif self == HardcodedPhy.SUNOFDMo4MCS01:
                return PhySelector(1, 0x0004)
            elif self == HardcodedPhy.SUNOFDMo4MCS26:
                return PhySelector(1, 0x0004)
            elif self == HardcodedPhy.LoraSF11:
                return PhySelector(2, 0x0010)
            elif self == HardcodedPhy.LoraSF7:
                return PhySelector(2, 0x0004)
            elif self == HardcodedPhy.OFDMLR:
                return PhySelector(3, 0x0000)
            elif self == HardcodedPhy.SidewalkFSK50kbps:
                return PhySelector(2, 0x0000)
            elif self == HardcodedPhy.SidewalkFSK150kbps:
                return PhySelector(2, 0x0001)
            elif self == HardcodedPhy.SidewalkFSK250kbps:
                return PhySelector(2, 0x0002)
            elif self == HardcodedPhy.LoraSF11_125kbps:
                return PhySelector(2, 0x0008)
            elif self == HardcodedPhy.LoraSF7_125kbps:
                return PhySelector(2, 0x0000)
            else:
                raise Exception("%s is an unsupported PHY on %s" % (self, DEVICE_PHY_GROUP))

        # Hugin PHYs
        elif DEVICE_PHY_GROUP == "CC140X":
            if self == HardcodedPhy.SUNFSK1a:
                return PhySelector(1, 0x0000)
            elif self == HardcodedPhy.SUNFSK1b:
                return PhySelector(1, 0x0002)
            if self == HardcodedPhy.SUNFSK2a:
                return PhySelector(1, 0x0004)
            elif self == HardcodedPhy.SUNFSK2b:
                return PhySelector(1, 0x0008)
            elif self == HardcodedPhy.SUNFSK4b:
                return PhySelector(1, 0x0040)
            elif self == HardcodedPhy.SUNOFDMo1:
                return PhySelector(1, 0x0100)
            elif self == HardcodedPhy.SUNOFDMo2:
                return PhySelector(1, 0x0200)
            elif self == HardcodedPhy.SUNOFDMo3MCS0:
                return PhySelector(1, 0x0400)
            elif self == HardcodedPhy.SUNOFDMo3MCS16:
                return PhySelector(1, 0x0800)
            elif self == HardcodedPhy.SUNOFDMo4MCS01:
                return PhySelector(1, 0x1000)
            elif self == HardcodedPhy.SUNOFDMo4MCS26:
                return PhySelector(1, 0x2000)
            elif self == HardcodedPhy.OFDMLR:
                return PhySelector(2, 0x0000)
            elif self == HardcodedPhy.PROPFSK50kbps:
                return PhySelector(3, 0x0000)
            elif self == HardcodedPhy.PROPFSK9p6kbps:
                return PhySelector(3, 0x0000)
            elif self == HardcodedPhy.PROPFSK38p4kbps:
                return PhySelector(3, 0x0000)
            elif self == HardcodedPhy.PROPFSK50kbps_LowPower:
                return PhySelector(3, 0x0000)
            elif self == HardcodedPhy.PROPFSK50kbps_GeneralPurpose:
                return PhySelector(3, 0x0000)
            elif self == HardcodedPhy.PROPOOK4_8kbps:
                return PhySelector(4, 0x0001)
            elif self == HardcodedPhy.SUNOQPSK:
                return PhySelector(5, 0x0000)
            elif self == HardcodedPhy.XLR_20Ksps_Dss2:
                return PhySelector(6, 0x0000)
            elif self == HardcodedPhy.XLR_20Ksps_Dss4:
                return PhySelector(6, 0x0001)
            elif self == HardcodedPhy.XLR_20Ksps_Dss8:
                return PhySelector(6, 0x0002)
            elif self == HardcodedPhy.WMBus_Tmode_3of6Encoding:
                return PhySelector(7, 0x0000)
            elif self == HardcodedPhy.WMBus_Smode_ManchesterEncoding:
                return PhySelector(7, 0x0001)
            elif self == HardcodedPhy.Wmbus_Cmode_M2O_FFA:
                return PhySelector(7, 0x0002)
            elif self == HardcodedPhy.Wmbus_Cmode_M2O_FFB:
                return PhySelector(7, 0x0004)
            elif self == HardcodedPhy.Wmbus_Cmode_O2M_FFA:
                return PhySelector(7, 0x0008)
            elif self == HardcodedPhy.Wmbus_Cmode_O2M_FFB:
                return PhySelector(7, 0x0010)
            elif self == HardcodedPhy.DUALRX_SUNOQPSK_PROPFSK38p4kbps:
                return PhySelector(8, 0x0000)
            elif self == HardcodedPhy.DUALRX_SUNOQPSK_PROPFSK50kbps:
                return PhySelector(9, 0x0000)
            elif self == HardcodedPhy.DUALRX_SUNOFDM_O3_PROPFSK50kbps:
                return PhySelector(10, 0x0000)
            else:
                raise Exception("%s is an unsupported PHY on %s" % (self, DEVICE_PHY_GROUP))

        # Unknown device
        else:
            raise Exception("No HardcodedPhys for %s, consider calling HardcodedPhy.set_device()" % DEVICE_PHY_GROUP)


# Work around an insane bug in construct that prevents enum.IntEnum to compare successfully with construct.Enum
# Essentially we force a cast between the two types via this otherwise useless lookup
# See https://github.com/construct/construct/issues/844#issuecomment-595061012
ERequest_ = Enum(Int8ul, Request)
ERequest = Mapping(ERequest_, {Request(k): v for k, v in ERequest_.decmapping.items()})
ERequest7bit_ = Enum(BitsInteger(7), Request)
ERequest7bit = Mapping(
    ERequest7bit_,
    defaultdict(lambda x: str(x), {Request(k): v for k, v in ERequest7bit_.decmapping.items()}),  # type:ignore
)


class RequestStatus(enum.IntEnum):
    SUCCESS = 0
    FAILURE = 2
    INVALID_PARAM = 3
    INVALID_STATE = 4
    INVALID_ALIGNMENT = 5
    NOT_IMPLEMENTED = 15


ERequestStatus_ = Enum(Nibble, RequestStatus)
ERequestStatus = Mapping(ERequestStatus_, {RequestStatus(k): v for k, v in ERequestStatus_.decmapping.items()})


class TransportErrorType(enum.IntEnum):
    CRC = 0
    RX_ALLOC_ERR = 1
    TX_ALLOC_ERR = 2
    UNEXPECTED_TID = 3


ETransportErrorType_ = Enum(BitsInteger(8), TransportErrorType)
ETransportErrorType = Mapping(
    ETransportErrorType_, {TransportErrorType(k): v for k, v in ETransportErrorType_.decmapping.items()}
)


class CommandStatusCode(enum.IntEnum):
    Idle = 0x00  # Command is not yet run. This state is mandatory when submitting.
    Queued = 0x01  # Command is queued, but pending end of previous command
    Scheduled = 0x02  # Command is scheduled, pending start time.
    Active = 0x03  # Command is currently running.
    Suspended = 0x04  # Command is suspended and will resume once the othe running commands have finished
    Deferred = 0x05  # Command is deferred.
    Finished = 0x10  # Command has finished normally
    ChannelIdle = 0x11  # Channel assessment has finished with channel idle
    ChannelBusy = 0x12  # Channel assessment has finished with channel busy
    NoSync = 0x14  # Command timed out waiting for a returned packet from peer device
    RxErr = 0x15  # Command ended due to errors with the received packet (e.g, CRC errors)
    RejectedStart = 0x16  # Command was rejected start due to scheduling parameters
    UnexpectedMdrRx = 0x17  # Command ended because an MDR packet was received when we have MDR disabled
    DescheduledApi = 0x31  # Command was descheduled (by API) before starting running in the radio
    DescheduledScheduling = 0x32  # Command was descheduled (by Scheduling) before starting running in the radio
    GracefulStopTimeout = 0x34  # Command ended because graceful stop time was reached
    GracefulStopApi = 0x35  # Command ended because graceful stop command was sent
    GracefulStopScheduling = 0x36  # Command ended gracefully because another command was scheduled
    HardStopApi = 0x39  # Command ended because hard stop command was sent
    HardStopScheduling = 0x3A  # Command ended hard because another command was scheduled
    Error = 0x80  # Command ended with unknown error
    Error_Setup = 0x81  # Command ended because of an error in the setup
    Error_Param = 0x82  # Command ended because of an error with a parameter
    Error_MissingTxBuffer = 0x83  # Command ended because no TX buffer was available when required
    Error_TxBufferCorruption = 0x84  # Command ended because of errors in TX buffer structure
    Error_RxBufferCorruption = 0x85  # Command ended because of errors in RX buffer structure
    Error_StartTooLate = 0x86  # Command ended because start time was in the past
    Error_TxFifo = 0x87  # Command ended because of underflow of TX FIFO
    Error_RxFifo = 0x88  # Command ended because of unsupported overflow of RX FIFO (no buffer to store packets)
    Error_Synth = 0x89  # Command ended because of synth programming error
    Error_UnknownOp = 0x8A  # Command ended because radio did not recognize command; probably wrong image
    Error_AlreadySubmitted = 0x8B  # Command not submitted because it was already scheduled or running


ECommandStatusCode16_ = Enum(Int16ul, CommandStatusCode)
ECommandStatusCode16 = Mapping(
    ECommandStatusCode16_, {CommandStatusCode(k): v for k, v in ECommandStatusCode16_.decmapping.items()}
)
ECommandStatusCode8_ = Enum(Int8ul, CommandStatusCode)
ECommandStatusCode8 = Mapping(
    ECommandStatusCode8_, {CommandStatusCode(k): v for k, v in ECommandStatusCode8_.decmapping.items()}
)


## RAW
##
SRequest = Struct(
    "len" / Rebuild(Int8ul, len_(this.data) + 2),
    "id" / ERequest,
    "data" / Int8ul[this.len - 2],  # type:ignore
)


@dataclass
class RawRequest(BaseRequest):
    """Fallback request type for unknown requests.
    For default cases and debugging.
    """

    id: int
    data: List[int]
    len: int = 0

    def __repr__(self):
        datalist = ":".join([f"{d:02X}" for d in self.data])
        return f"{type(self).__name__}(id={self.id}, len={self.len}, data=[{datalist}])"


## PING
##
SPingRequest = Struct(
    "len" / Const(3, Int8ul),
    "id" / Const(Request.PING, ERequest),
    "ping" / Int8ul,  # type:ignore
)


@dataclass
class PingRequest(BaseRequest):
    ping: int
    len: int = 3
    id: int = Request.PING

    def __repr__(self):
        return f"{type(self).__name__}(ping={self.ping:02X})"


## LAST_STATUS VERSION PARAM
SLastStatusParamVersion = Struct(
    "major" / Int8ul,  # type:ignore
    "minor" / Int8ul,  # type:ignore
    "patch" / Int8ul,  # type:ignore
    "build" / Int8ul,  # type:ignore
    "hash" / Int32ul,  # type:ignore
)


## LAST_STATUS MAC UUID PARAM
SLastStatusParamMacUuid = Struct(
    "macAddress" / Int8ul[8],  # type:ignore
    "uuid" / Int8ul[8],  # type:ignore
)


## LAST_STATUS DEVICE INFO PARAM
SLastStatusParamDevice = Struct(
    "deviceId" / Int32ul,  # type:ignore
    "partId" / Int32ul,  # type:ignore
)

## LAST_STATUS NV INFO PARAM
SLastStatusParamNvInfo = Struct(
    "capacity" / Int16ul,  # type:ignore
    "available" / Int16ul,  # type:ignore
    "utilized" / Int16ul,  # type:ignore
)

## LAST_STATUS
##
SLastStatus = BitStruct(
    "len" / BitsInteger(8),
    "id" / Const(Request.LAST_STATUS, Bytewise(ERequest)),
    "res0" / BitsInteger(1),
    "req" / ERequest7bit,
    "status" / ERequestStatus,
    "tid" / Nibble,  # type:ignore
    "params"
    / Bytewise(
        Switch(
            this.req,
            {
                Request.UTIL_GETTIME: Struct("time" / Int32ul),  # type:ignore
                Request.UTIL_GETRSSI: Struct("rssi" / Int8sl),  # type:ignore
                Request.COMMAND_STOP: Struct("status" / ECommandStatusCode8),
                Request.COMMAND_SUBMIT: Struct("status" / ECommandStatusCode8),
                Request.UTIL_INFO_VERSION: SLastStatusParamVersion,
                Request.UTIL_INFO_MAC_UUID: SLastStatusParamMacUuid,
                Request.UTIL_INFO_DEVICE: SLastStatusParamDevice,
                Request.NV_ALLOCATE: SLastStatusParamNvInfo,
                Request.NV_INFO: SLastStatusParamNvInfo,
            },
            default=Struct("data" / Int8ul[this._.len - 4]),  # type:ignore
        )
    ),
)


@dataclass
class GetTimeParams:
    time: int


@dataclass
class GetRssiParams:
    rssi: int


@dataclass
class CommandStopParams:
    status: CommandStatusCode


@dataclass
class CommandSubmitParams:
    status: CommandStatusCode


@dataclass
class VersionParams:
    major: int
    minor: int
    patch: int
    build: int
    hash: int


@dataclass
class NvInfoParams:
    capacity: int
    available: int
    utilized: int


@dataclass
class MacUuidParams:
    macAddress: int
    uuid: int


@dataclass
class DeviceParams:
    deviceId: int
    partId: int


@dataclass
class GenericParams:
    data: List[int] = field(default_factory=list)

    def __post_init__(self):
        # Remove construct container nonsense
        if isinstance(self.data, ListContainer):
            self.data = [x for x in self.data]

    def __str__(self):
        return nice(self.data)


@dataclass
class LastStatus(BaseRequest):
    tid: int
    req: Request
    status: RequestStatus
    res0: int = 0
    params: Union[
        GenericParams,
        GetTimeParams,
        GetRssiParams,
        CommandStopParams,
        CommandSubmitParams,
        VersionParams,
        MacUuidParams,
        DeviceParams,
    ] = field(default_factory=GenericParams)
    id: int = Request.LAST_STATUS
    len: int = 4

    def __post_init__(self):
        if self.len == 4 and hasattr(self.params, "__len__"):  # type:ignore
            self.len += len(self.params)  # type:ignore
        if isinstance(self.params, Container) and hasattr(self.params, "data"):
            self.params = [x for x in self.params.data]  # type:ignore

    def __repr__(self):
        return f"{type(self).__name__}(tid={self.tid:01X}, req={self.req.name}, status={self.status.name}, params={self.params})"

    def __eq__(self, __o: object) -> bool:
        if not isinstance(__o, self.__class__):
            return False
        return self.tid == __o.tid and self.status == __o.status and self.req == __o.req

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "params":
            lookup = defaultdict(
                lambda: GenericParams,
                {
                    Request.UTIL_GETTIME: GetTimeParams,
                    Request.UTIL_GETRSSI: GetRssiParams,
                    Request.COMMAND_STOP: CommandStopParams,
                    Request.COMMAND_SUBMIT: CommandSubmitParams,
                    Request.UTIL_INFO_VERSION: VersionParams,
                    Request.UTIL_INFO_MAC_UUID: MacUuidParams,
                    Request.UTIL_INFO_DEVICE: DeviceParams,
                    Request.NV_ALLOCATE: NvInfoParams,
                    Request.NV_INFO: NvInfoParams,
                },
            )
            return lookup[self_dictionary["req"]]
        if child == "req":
            return Request


## TRANSPORT_ERROR
##
STransportError = BitStruct(
    "len" / BitsInteger(8),
    "id" / Const(Request.TRANSPORT_ERROR, Bytewise(ERequest)),
    "tid_error" / BitsInteger(4),
    "tid_ok" / BitsInteger(4),
    "error" / ETransportErrorType,
)


@dataclass
class TransportError(BaseRequest):
    error: TransportErrorType
    tid_ok: int = 0xF
    tid_error: int = 0xF
    len: int = 4
    id: int = Request.TRANSPORT_ERROR

    def __post_init__(self):
        if not isinstance(self.error, TransportErrorType):
            self.error = TransportErrorType(int(self.error))

    def __repr__(self):
        return f"{type(self).__name__}(error={self.error.name}, tid_ok={self.tid_ok}, tid_error={self.tid_error})"


class CommandId(enum.IntEnum):
    NoOp = 0
    Transmit = 1
    Receive = 2
    CarrierSense = 3
    TransmitTest = 4
    ReceiveTest = 5
    DualReceive = 6
    ExtTransmit = 7
    ExtReceive = 8
    Reserved = 15


ECommandId_ = Enum(Int8ul, CommandId)
ECommandId = Mapping(ECommandId_, {CommandId(k): v for k, v in ECommandId_.decmapping.items()})
E4BitCommandId_ = Enum(BitsInteger(4), CommandId)
E4BitCommandId = Mapping(E4BitCommandId_, {CommandId(k): v for k, v in E4BitCommandId_.decmapping.items()})


class ConflictPolicy(enum.IntEnum):
    AlwaysInterrupt = 0
    Polite = 1
    NeverInterrupt = 2


E2BitConflictPolicy_ = Enum(BitsInteger(2), ConflictPolicy)
E2BitConflictPolicy = Mapping(
    E2BitConflictPolicy_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {ConflictPolicy(k): v for k, v in E2BitConflictPolicy_.decmapping.items()},  # type:ignore
    ),
)


class TriggerType(enum.IntEnum):
    Immediate = 0
    IoEdge = 2
    Time_Absolute = 3
    Stream = 5
    Time_Relative_Previous_Start = 10
    Time_Relative_Previous_End = 11


E5BitTriggerType_ = Enum(BitsInteger(5), TriggerType)
E5BitTriggerType = Mapping(
    E5BitTriggerType_,
    defaultdict(lambda x: str(x), {TriggerType(k): v for k, v in E5BitTriggerType_.decmapping.items()}),  # type:ignore
)


class ModemMask(enum.IntFlag):
    AUTO = 0  # CC140x only
    NONE = 0
    FSK = 1 << 0
    OFDM = 1 << 1
    LORA = 1 << 2  # CC1308 only
    OQPSK = 1 << 2  # CC140x only
    FSK_OFDM = FSK | OFDM
    OFDM_LORA = OFDM | LORA
    FSK_LORA = FSK | LORA
    FSK_OQPSK = OQPSK | FSK
    FSK_OFDM_LORA = FSK | OFDM | LORA

    def set_device(device: str):
        global DEVICE_PHY_GROUP
        DEVICE_PHY_GROUP = device

    def __str__(self):
        # Make sure CC140x displays OQPSK, not LORA
        if DEVICE_PHY_GROUP == "CC140X" and self == ModemMask.OQPSK:
            return "ModemMask.OQPSK"
        if DEVICE_PHY_GROUP == "CC140X" and self == ModemMask.OQPSK | ModemMask.FSK:
            return "ModemMask.FSK_OQPSK"
        else:
            return f"ModemMask.{self.name}"


E3BitModemMask_ = Enum(BitsInteger(3), ModemMask)
E3BitModemMask = Mapping(
    E3BitModemMask_,
    defaultdict(lambda x: str(x), {ModemMask(k): v for k, v in E3BitModemMask_.decmapping.items()}),  # type:ignore
)


class RXSearchStrategy(enum.IntEnum):
    Sync = 0
    Energy_Sync = 1
    Preamble_Sync = 2
    Energy_Preamble_Sync = 3


E8BitRXSearchStrategy_ = Enum(BitsInteger(8), RXSearchStrategy)
E8BitRXSearchStrategy = Mapping(
    E8BitRXSearchStrategy_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {
            RXSearchStrategy(k): v for k, v in E8BitRXSearchStrategy_.decmapping.items()  # type:ignore
        },
    ),
)


class TxPa(enum.IntEnum):
    Low = 0
    High = 1


E1BitTxPa_ = Enum(BitsInteger(1), TxPa)
E1BitTxPa = Mapping(
    E1BitTxPa_,
    defaultdict(lambda x: str(x), {TxPa(k): v for k, v in E1BitTxPa_.decmapping.items()}),  # type:ignore
)


class TxTestMode(enum.IntEnum):
    Unmodulated = 0
    Modulated_Pattern = 1
    Whitened_Pattern = 2
    Packet = 3


E2BitTxTestMode_ = Enum(BitsInteger(2), TxTestMode)
E2BitTxTestMode = Mapping(
    E2BitTxTestMode_,
    defaultdict(lambda x: str(x), {TxTestMode(k): v for k, v in E2BitTxTestMode_.decmapping.items()}),  # type:ignore
)


class TxRepeatMode(enum.IntEnum):
    RepeatNone = 0
    RepeatInfinite = 0xFFFF


class CSRSSIOverride(enum.IntEnum):
    UsePHY = 127


class CSMode(enum.IntEnum):
    Energy = 0
    Preamble = 1
    And = 2
    Or = 3


E2BitCSMode_ = Enum(BitsInteger(2), CSMode)
E2BitCSMode = Mapping(
    E2BitCSMode_,
    defaultdict(lambda x: str(x), {CSMode(k): v for k, v in E2BitCSMode_.decmapping.items()}),  # type:ignore
)


class CSExitCondition(enum.IntEnum):
    WaitForIdle = 0
    WaitForBusy = 1
    Fast = 2


E2BitCSExitCondition_ = Enum(BitsInteger(2), CSExitCondition)
E2BitCSExitCondition = Mapping(
    E2BitCSExitCondition_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {CSExitCondition(k): v for k, v in E2BitCSExitCondition_.decmapping.items()},  # type:ignore
    ),
)


class StatusMatchAction(enum.IntEnum):
    Ignore = 0
    Stop = 1
    Branch = 2


E2BitStatusMatchAction_ = Enum(BitsInteger(2), StatusMatchAction)
E2BitStatusMatchAction = Mapping(
    E2BitStatusMatchAction_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {StatusMatchAction(k): v for k, v in E2BitStatusMatchAction_.decmapping.items()},  # type:ignore
    ),
)

SPhySelector = Struct(
    "subphy" / Bytewise(Int16ul),
    "reserved" / BitsInteger(4),
    "config" / BitsInteger(4),
)


class TxPowerSpecial(enum.IntEnum):
    Use_Min = -128
    Use_Max = 125
    Use_Raw = 126
    Use_None = 127


class ExtTXRXRSSIOverride(enum.IntEnum):
    UsePHY = 127


class ExtTXFixedLengthOverride(enum.IntEnum):
    UsePHY = 0xFFFF


# A note on bit ordering in these structs: bits are defined from top to bottom. This means that bits in
# a byte are ordered
# bit4
# bit3
# bit2
# bit1
# Instead of the more typical in C structs, which would be:
# bit1
# bit2
# bit3
# bit4

SCommandStoreTxPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "stream_id" / Bytewise(Int16ul),
    "frequency" / BitsInteger(24, swapped=True, signed=True),  # signed to support delta-frequency (hidden feature)
    "modem" / E3BitModemMask,
    "pa" / E1BitTxPa,
    "reserved" / BitsInteger(4),
    "power_code" / Bytewise(Int8sl),
    "power"
    / Computed(this.power_code / 2 if this.power_code not in [x.value for x in TxPowerSpecial] else this.power_code),
)

SCommandStoreRxPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "stream_id" / Bytewise(Int16ul),
    "frequency" / BitsInteger(24, swapped=True, signed=True),  # signed to support delta-frequency (hidden feature)
    "modem" / E3BitModemMask,
    "stream_early" / Flag,  # type:ignore
    "enable_mdr" / Flag,  # type:ignore
    "discardStatusOnTimeout" / BitsInteger(1),
    "reserved" / BitsInteger(2),
    "timeout" / Bytewise(Int32ul),
    "search_strategy" / BitsInteger(8),
    "repeat" / BitsInteger(8),
)

SCommandStoreTxTestPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "stream_id" / Bytewise(Int16ul),
    "frequency" / BitsInteger(24, swapped=True, signed=True),  # signed to support delta-frequency (hidden feature)
    "modem" / E3BitModemMask,
    "pa" / E1BitTxPa,
    "reserved0" / BitsInteger(2),
    "test_mode" / E2BitTxTestMode,
    "power_code" / Bytewise(Int8sl),
    "pattern" / BitsInteger(8),
    "repeat" / Bytewise(Int16ul),
    "power_raw"
    / Switch(
        this.pa,
        {
            TxPa.Low: Struct(
                "rawValue" / BitsInteger(16, swapped=True),
                "tempCoeff" / BitsInteger(8),
                "reserved1" / BitsInteger(8),
            ),
            TxPa.High: Struct(
                "rawValue" / BitsInteger(32, swapped=True),
                "tempCoeff" / BitsInteger(8),
                "reserved1" / BitsInteger(8),
                "reserved2" / BitsInteger(8),
            ),
        },
    ),
    "power"
    / Computed(this.power_code / 2 if this.power_code not in [x.value for x in TxPowerSpecial] else this.power_code),
)

SCommandStoreRxTestPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "stream_id" / Bytewise(Int16ul),
    "frequency" / BitsInteger(24, swapped=True, signed=True),  # signed to support delta-frequency (hidden feature)
    "modem" / E3BitModemMask,
    "stream_early" / Flag,  # type:ignore
    "enable_mdr" / Flag,  # type:ignore
    "discardStatusOnTimeout" / BitsInteger(1),
    "discard_packets" / BitsInteger(1),
    "no_sync" / BitsInteger(1),
    "timeout" / Bytewise(Int32ul),
    "search_strategy" / BitsInteger(8),
    "repeat" / BitsInteger(8),
)

SCommandStoreCSPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "rssi_count" / BitsInteger(4),
    "rssi_window" / BitsInteger(4),
    "corr_count" / BitsInteger(4),
    "corr_window" / BitsInteger(4),
    "frequency" / BitsInteger(24, swapped=True, signed=True),  # signed to support delta-frequency (hidden feature)
    "modem" / E3BitModemMask,
    "discardStatusOnTimeout" / BitsInteger(1),
    "exit_condition" / E2BitCSExitCondition,
    "mode" / E2BitCSMode,
    "timeout" / Bytewise(Int32ul),
    "rssi_override" / Bytewise(Int8sl),
    "reserved1" / BitsInteger(24),
)

SCommandStoreDualRxPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "stream_id" / Bytewise(Int16ul),
    "frequency0" / Bytewise(Int32ul),
    "frequency1" / Bytewise(Int32ul),
    "modem" / E3BitModemMask,
    "stream_early" / Flag,  # type:ignore
    "enable_mdr" / Flag,  # type:ignore
    "discardStatusOnTimeout" / BitsInteger(1),
    "discard_packets" / BitsInteger(1),
    "no_sync" / BitsInteger(1),
    "timeout" / Bytewise(Int32ul),
    "search_strategy" / BitsInteger(8),
    "repeat" / BitsInteger(8),
)

SCommandStoreExtTxPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "stream_id" / Bytewise(Int8ul),
    "frequency" / BitsInteger(32, swapped=True, signed=True),  # signed to support delta-frequency (hidden feature)
    "modem" / E3BitModemMask,
    "pa" / E1BitTxPa,
    "reserved" / BitsInteger(4),
    "power_code" / Bytewise(Int8sl),
    "power"
    / Computed(this.power_code / 2 if this.power_code not in [x.value for x in TxPowerSpecial] else this.power_code),
    "fixed_length" / Bytewise(Int16ul),
)

SCommandStoreExtRxPartial = BitStruct(
    "phy0" / SPhySelector,
    "phy1" / SPhySelector,
    "phy2" / SPhySelector,
    "stream_id" / Bytewise(Int8ul),
    "frequency" / Bytewise(Int32ul),
    "modem" / E3BitModemMask,
    "stream_early" / Flag,  # type:ignore
    "enable_mdr" / Flag,  # type:ignore
    "discardStatusOnTimeout" / BitsInteger(1),
    "discard_packets" / BitsInteger(1),
    "no_sync" / BitsInteger(1),
    "timeout" / Bytewise(Int32ul),
    "search_strategy" / BitsInteger(8),
    "repeat" / BitsInteger(8),
    "fixed_length" / Bytewise(Int16ul),
    "rssi_threshold" / Bytewise(Int8sl),
)

## COMMAND_STORE
##
SCommandStoreCommon = "root" / BitStruct(
    "len" / BitsInteger(8),
    "id" / Const(Request.COMMAND_STORE, Bytewise(ERequest)),
    "slot" / BitsInteger(8),
    "cmd_id" / Bytewise(ECommandId),
    "reserved0" / Const(0, BitsInteger(8)),
    "chain"
    / Struct(
        "enable_on_true" / BitsInteger(1),
        "slot_on_true" / BitsInteger(7),
        "enable_on_false" / BitsInteger(1),
        "slot_on_false" / BitsInteger(7),
        "action_on_status" / E2BitStatusMatchAction,
        "slot_on_status" / BitsInteger(6),
        "status_compare" / Bytewise(Int16ul),
    ),
    "trigger"
    / Struct(
        "allow_delay" / Flag,  # type:ignore
        "conflict_policy" / E2BitConflictPolicy,
        "trigger" / E5BitTriggerType,
        "chain_allow_delay" / Flag,  # type:ignore
        "chain_conflict_policy" / E2BitConflictPolicy,
        "chain_trigger" / E5BitTriggerType,
        "trigger_param" / Bytewise(Int32ul),
        "chain_trig_param" / Bytewise(Int32ul),
    ),
    "params"
    / Bytewise(
        Switch(
            this.cmd_id,
            {
                CommandId.Transmit: SCommandStoreTxPartial,
                CommandId.Receive: SCommandStoreRxPartial,
                CommandId.TransmitTest: SCommandStoreTxTestPartial,
                CommandId.ReceiveTest: SCommandStoreRxTestPartial,
                CommandId.CarrierSense: SCommandStoreCSPartial,
                CommandId.DualReceive: SCommandStoreDualRxPartial,
                CommandId.ExtTransmit: SCommandStoreExtTxPartial,
                CommandId.ExtReceive: SCommandStoreExtRxPartial,
            },
        )
    ),
)


@dataclass
class TriggerParams:
    trigger: TriggerType = TriggerType.Immediate
    allow_delay: bool = False
    conflict_policy: ConflictPolicy = ConflictPolicy.AlwaysInterrupt
    chain_trigger: TriggerType = TriggerType.Immediate
    chain_allow_delay: bool = False
    chain_conflict_policy: ConflictPolicy = ConflictPolicy.AlwaysInterrupt
    trigger_param: int = 0x00000000
    chain_trig_param: int = 0x0000000

    def __post_init__(self):
        if not isinstance(self.trigger, TriggerType):
            try:
                self.trigger = TriggerType(int(self.trigger))
            except:
                pass
        if not isinstance(self.chain_trigger, TriggerType):
            try:
                self.chain_trigger = TriggerType(int(self.chain_trigger))
            except:
                pass
        if not isinstance(self.conflict_policy, ConflictPolicy):
            try:
                self.conflict_policy = ConflictPolicy(int(self.conflict_policy))
            except:
                pass
        if not isinstance(self.chain_conflict_policy, ConflictPolicy):
            try:
                self.chain_conflict_policy = ConflictPolicy(int(self.chain_conflict_policy))
            except:
                pass

    def __repr__(self):
        parts = []
        parts.append(f"{self.trigger.name}({self.trigger_param:02d}, {self.conflict_policy.name})")
        parts.append("allowDelay" if self.allow_delay else "noDelay")
        chain_parts = []
        chain_parts.append(f"{self.chain_trigger.name}({self.chain_trig_param:02d}, {self.chain_conflict_policy.name})")
        chain_parts.append("allowDelay" if self.chain_allow_delay else "noDelay")
        return f'Trigger({", ".join(parts)}), ChainTrigger({", ".join(chain_parts)})'


@dataclass
class ChainParams:
    slot_on_true: int = 0
    enable_on_true: bool = False
    slot_on_false: int = 0
    enable_on_false: bool = False
    slot_on_status: int = 0
    action_on_status: StatusMatchAction = StatusMatchAction.Ignore
    status_compare: int = 0x0000

    def __repr__(self):
        parts = []
        if self.enable_on_true:
            parts.append(f"success: {self.slot_on_true}")
        if self.enable_on_false:
            parts.append(f"fail: {self.slot_on_false}")
        if self.action_on_status != StatusMatchAction.Ignore:
            parts.append(f"compare({self.status_compare}): {self.slot_on_status}")
        return f'Chain({", ".join(parts)})'


@dataclass
class PhySelector:
    config: int = 0
    subphy: int = 0
    reserved: int = 0


@dataclass
class TxParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    stream_id: int = 0
    frequency: int = 915000
    reserved: int = 0
    pa: int = (
        None
    )  # type:ignore # Initialize it in __post_init__ in case it's not user-initialized, based on the selected modem
    modem: int = ModemMask.FSK
    power: Union[float, TxPowerSpecial, Dict[str, float]] = 10.0
    power_code: int = field(init=False)

    def __post_init__(self):
        if self.pa == None:
            if self.modem == ModemMask.AUTO:
                # Find PA based on PHY
                if (
                    self.phy0.config == HardcodedPhy.SUNOFDMo1.selector.config
                    and self.phy0.subphy >= HardcodedPhy.SUNOFDMo1.selector.subphy
                ) or self.phy0.config == HardcodedPhy.OFDMLR.selector.config:
                    # SUN OFDM or OFDMLR PHY
                    self.pa = TxPa.High
                else:
                    self.pa = TxPa.Low
            elif self.modem == ModemMask.OFDM:
                self.pa = TxPa.High
            else:
                self.pa = TxPa.Low

        if isinstance(self.power, dict):
            power = self.power["High"] if self.pa == TxPa.High else self.power["Low"]
            self.power = power

        self.power_code = (
            round(self.power * 2) if self.power not in [x.value for x in TxPowerSpecial] else round(self.power)
        )

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "pa":
            return int
        if child == "power":
            if self_dictionary["power_code"] in [x.value for x in TxPowerSpecial]:
                return TxPowerSpecial
            return float

    def __repr__(self):
        return f"Tx({self.frequency}kHz, stream={self.stream_id}, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, pwr={self.power})"


@dataclass
class RxParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    stream_id: int = 0
    frequency: int = 915000
    reserved: int = 0
    enable_mdr: bool = False
    stream_early: bool = False
    discardStatusOnTimeout: bool = False
    modem: int = ModemMask.FSK
    timeout: int = 0
    search_strategy: int = 0
    repeat: int = 0

    def __repr__(self):
        return f"Rx({self.frequency}kHz, stream={self.stream_id}, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, timeout={self.timeout})"


@dataclass
class DualRxParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    stream_id: int = 0
    frequency0: int = 915000000
    frequency1: int = 915000000
    reserved: int = 0
    enable_mdr: bool = False
    stream_early: bool = False
    discardStatusOnTimeout: bool = False
    modem: int = ModemMask.FSK
    timeout: int = 0
    search_strategy: int = 0
    repeat: int = 0
    no_sync: int = 0
    discard_packets: int = 0

    def __repr__(self):
        return f"DualRx({self.frequency0}Hz, {self.frequency1}Hz, stream={self.stream_id}, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, timeout={self.timeout})"


@dataclass
class ExtTxParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    stream_id: int = 0
    frequency: int = 915000000
    reserved: int = 0
    pa: Union[
        TxPa, None
    ] = None  # Initialize it in __post_init__ in case it's not user-initialized, based on the selected modem
    modem: int = ModemMask.FSK
    power: Union[float, TxPowerSpecial, Dict[str, float]] = 10.0
    power_code: int = field(init=False)
    fixed_length: Union[int, ExtTXFixedLengthOverride] = ExtTXFixedLengthOverride.UsePHY

    def __post_init__(self):
        if self.pa == None:
            if self.modem == ModemMask.AUTO:
                # Find PA based on PHY
                if (
                    self.phy0.config == HardcodedPhy.SUNOFDMo1.selector.config
                    and self.phy0.subphy >= HardcodedPhy.SUNOFDMo1.selector.subphy
                ) or self.phy0.config == HardcodedPhy.OFDMLR.selector.config:
                    # SUN OFDM or OFDMLR PHY
                    self.pa = TxPa.High
                else:
                    self.pa = TxPa.Low
            elif self.modem == ModemMask.OFDM:
                self.pa = TxPa.High
            else:
                self.pa = TxPa.Low

        if isinstance(self.power, dict):
            power = self.power["High"] if self.pa == TxPa.High else self.power["Low"]
            self.power = power

        self.power_code = (
            round(self.power * 2) if self.power not in [x.value for x in TxPowerSpecial] else round(self.power)
        )

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "power":
            if self_dictionary["power_code"] in [x.value for x in TxPowerSpecial]:
                return TxPowerSpecial
            return float

    def __repr__(self):
        return f"ExtTx({self.frequency}Hz, stream={self.stream_id}, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, pwr={self.power}, fixed_length={self.fixed_length})"


@dataclass
class ExtRxParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    stream_id: int = 0
    frequency: int = 915000000
    reserved: int = 0
    enable_mdr: bool = False
    no_sync: int = 0
    discard_packets: int = 0
    stream_early: bool = False
    modem: int = ModemMask.FSK
    discardStatusOnTimeout: bool = False
    timeout: int = 0
    search_strategy: int = 0
    repeat: int = 0
    fixed_length: Union[int, ExtTXFixedLengthOverride] = ExtTXFixedLengthOverride.UsePHY
    rssi_threshold: Union[int, ExtTXRXRSSIOverride] = ExtTXRXRSSIOverride.UsePHY

    def __repr__(self):
        return f"ExtRx({self.frequency}Hz, stream={self.stream_id}, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, timeout={self.timeout}, fixed_length={self.fixed_length}, rssi_threshold={self.rssi_threshold})"


@dataclass
class TxPowerRaw:
    reserved1: int = 0
    reserved2: int = 0
    rawValue: int = 0  #
    tempCoeff: int = 0  #


@dataclass
class TxTestParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    stream_id: int = 0
    test_mode: int = TxTestMode.Unmodulated
    pattern: int = 0
    frequency: int = 915000
    pa: int = (
        None
    )  # type:ignore # Initialize it in __post_init__ in case it's not user-initialized, based on the selected modem
    modem: int = ModemMask.FSK
    power: Union[float, TxPowerSpecial, Dict[str, float]] = 0.0
    power_raw: TxPowerRaw = field(default_factory=lambda: TxPowerRaw())
    repeat: Union[int, TxRepeatMode] = TxRepeatMode.RepeatNone
    reserved0: int = 0
    power_code: int = field(init=False)

    def __post_init__(self):
        if self.pa == None:
            self.pa = TxPa.High if self.modem == ModemMask.OFDM else TxPa.Low

        if isinstance(self.power, dict):
            power = self.power["High"] if self.pa == TxPa.High else self.power["Low"]
            self.power = power

        self.power_code = (
            round(self.power * 2) if self.power not in [x.value for x in TxPowerSpecial] else round(self.power)
        )

        if self.power == TxPowerSpecial.Use_Raw:
            if self.pa == TxPa.High:
                if self.power_raw.rawValue > (2**30 - 1):
                    raise Exception("Raw power value exceeds field size of 30 bits")
            else:
                # The raw value may include the tempCoeff in bits 23:16
                if self.power_raw.tempCoeff == 0:
                    if self.power_raw.rawValue > (2**24 - 1):
                        raise Exception("Raw power value with temperature exceeds field size of 24 bits")
                    else:
                        self.power_raw.tempCoeff = (self.power_raw.rawValue >> 16) & 0xFF
                        self.power_raw.rawValue = self.power_raw.rawValue & 0xFFFF

                if self.power_raw.rawValue > (2**13 - 1):
                    raise Exception("Raw power value exceeds field size of 13 bits")

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "repeat":
            return int
        if child == "power":
            if isinstance(child, dict):
                return Dict[str, float]
            elif self_dictionary["power_code"] in [x.value for x in TxPowerSpecial]:
                return TxPowerSpecial
            else:
                return float

    def __repr__(self):
        if self.power == TxPowerSpecial.Use_Raw:
            power_str = f"0x{self.power_raw.rawValue:04X} (tempCoeff=0x{self.power_raw.tempCoeff:02X})"
        elif self.power == TxPowerSpecial.Use_None:
            power_str = "NONE (re-use present)"
        elif self.power == TxPowerSpecial.Use_Min:
            power_str = "MIN"
        elif self.power == TxPowerSpecial.Use_Max:
            power_str = "MAX"
        else:
            power_str = f"{self.power}"
        return f"Tx({self.frequency}kHz, mode={self.test_mode}, pattern={self.pattern}, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, pwr={power_str}, rep={self.repeat})"


@dataclass
class RxTestParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    stream_id: int = 0
    frequency: int = 915000
    reserved: int = 0
    modem: int = ModemMask.FSK
    enable_mdr: bool = False
    stream_early: bool = False
    discardStatusOnTimeout: bool = False
    timeout: int = 0
    search_strategy: int = 0
    repeat: int = 0
    no_sync: int = 0
    discard_packets: int = 0

    def __repr__(self):
        return f"RxTest({self.frequency}kHz, stream={self.stream_id}, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, timeout={self.timeout})"


@dataclass
class CSParams:
    phy0: PhySelector = field(default_factory=lambda: HardcodedPhy.SUNFSK2b.selector)
    phy1: PhySelector = field(default_factory=lambda: PhySelector())
    phy2: PhySelector = field(default_factory=lambda: PhySelector())
    rssi_window: int = 0
    rssi_count: int = 0
    corr_window: int = 0
    corr_count: int = 0
    frequency: int = 915000
    mode: CSMode = CSMode.Energy
    exit_condition: CSExitCondition = CSExitCondition.WaitForIdle
    discardStatusOnTimeout: bool = False
    modem: ModemMask = ModemMask.FSK
    timeout: int = 0
    rssi_override: Union[int, CSRSSIOverride] = CSRSSIOverride.UsePHY
    reserved1: int = 0

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "rssi_override":
            return int

    def __repr__(self):
        return (
            f"CS({self.frequency}kHz, phy=[{self.phy0.config}/{self.phy1.config}/{self.phy2.config}], mdm={self.modem}, timeout={self.timeout}, "
            f"rssi window/count={self.rssi_window}/{self.rssi_count}, "
            f"corr window/count={self.corr_window}/{self.corr_count}, "
            f"mode={self.mode}, exit_condition={self.exit_condition}, rssi_override={self.rssi_override})"
        )


@dataclass
class CommandStore(BaseRequest):
    # Must be overridden
    cmd_id: CommandId
    params: Union[TxParams, TxTestParams, RxParams, RxTestParams, DualRxParams, CSParams, ExtTxParams, ExtRxParams]
    trigger: TriggerParams = field(default_factory=TriggerParams)
    chain: ChainParams = field(default_factory=ChainParams)

    # Request
    id: int = Request.COMMAND_STORE
    len: int = 20

    # CommandReq
    slot: int = 0

    def __post_init__(self):
        self.len = len(SCommandStoreCommon.build(asdict(self)))
        if not isinstance(self.cmd_id, CommandId):
            self.cmd_id = CommandId(self.cmd_id)

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "params":
            lookup = defaultdict(
                lambda: list,
                {
                    CommandId.Transmit: TxParams,
                    CommandId.Receive: RxParams,
                    CommandId.TransmitTest: TxTestParams,
                    CommandId.ReceiveTest: RxTestParams,
                    CommandId.CarrierSense: CSParams,
                    CommandId.DualReceive: DualRxParams,
                    CommandId.ExtTransmit: ExtTxParams,
                    CommandId.ExtReceive: ExtRxParams,
                },
            )
            return lookup[self_dictionary["cmd_id"]]

    def __repr__(self):
        return f"{type(self).__name__}({self.cmd_id.name}, slot={self.slot:02X}, {self.params!r}, {self.trigger!r}, {self.chain!r})"


## SUBMIT
##
SCommandSubmit = Struct(
    "len" / Const(3, Int8ul),
    "id" / Const(Request.COMMAND_SUBMIT, ERequest),
    "slot" / Int8ul,  # type:ignore
)


@dataclass
class CommandSubmit(BaseRequest):
    slot: int
    len: int = 3
    id: int = Request.COMMAND_SUBMIT

    def __repr__(self):
        return f"{type(self).__name__}(slot={self.slot:02X})"


class StopType(enum.IntEnum):
    NoStop = 0  # No stop requested
    DescheduleOnly = 1  # Stop the command if it has not started executing
    Graceful = 2  # Stop the command gracefully, that is finish a packet or transaction in progress before ending
    Hard = 3  # Stop the command as soon as possible


## STOP
##
SCommandStop = Struct(
    "len" / Const(4, Int8ul),
    "id" / Const(Request.COMMAND_STOP, ERequest),
    "slot" / Int8ul,  # type:ignore
    "type" / Enum(Int8ul, StopType),
)


@dataclass
class CommandStop(BaseRequest):
    slot: int
    type: int
    len: int = 4
    id: int = Request.COMMAND_STOP

    def __repr__(self):
        return f"{type(self).__name__}(slot={self.slot:02X}, type={StopType(int(self.type)).name})"


class ChainAction(enum.IntEnum):
    NoAction = 0
    OnTrue = 1
    OnFalse = 2
    OnMatch = 3


E8BitChainAction_ = Enum(BitsInteger(8), ChainAction)
E8BitChainAction = Mapping(E8BitChainAction_, {ChainAction(k): v for k, v in E8BitChainAction_.decmapping.items()})

## COMMAND_STATUS
##
SCommandStatus = BitStruct(
    "len" / BitsInteger(8),
    "id" / Const(Request.COMMAND_STATUS, Bytewise(ERequest)),
    "status" / Bytewise(ECommandStatusCode8),
    "cmd_id" / Bytewise(ECommandId),
    "slot" / BitsInteger(8),
    "chain_action" / E8BitChainAction,
    "chain_target" / BitsInteger(8),
    "reserved0" / Const(0, BitsInteger(8)),
    "params"
    / Bytewise(
        Switch(
            this.cmd_id,
            {
                CommandId.Receive: Struct(
                    "num_sync" / Int16ul,  # type:ignore
                    "num_hdr_ok" / Int16ul,  # type:ignore
                    "num_crc_ok" / Int16ul,  # type:ignore
                    "pending_rx" / Int8ul,  # type:ignore
                    "last_rssi" / Int8sl,  # type:ignore
                ),
                CommandId.DualReceive: Struct(
                    "num_sync" / Int16ul,  # type:ignore
                    "num_hdr_ok" / Int16ul,  # type:ignore
                    "num_crc_ok" / Int16ul,  # type:ignore
                    "pending_rx" / Int8ul,  # type:ignore
                    "last_rssi" / Int8sl,  # type:ignore
                ),
                CommandId.ExtReceive: Struct(
                    "num_sync" / Int16ul,  # type:ignore
                    "num_hdr_ok" / Int16ul,  # type:ignore
                    "num_crc_ok" / Int16ul,  # type:ignore
                    "pending_rx" / Int8ul,  # type:ignore
                    "last_rssi" / Int8sl,  # type:ignore
                ),
                CommandId.ReceiveTest: Struct(
                    "num_sync" / Int16ul,  # type:ignore
                    "num_hdr_ok" / Int16ul,  # type:ignore
                    "num_crc_ok" / Int16ul,  # type:ignore
                    "pending_rx" / Int8ul,  # type:ignore
                    "last_rssi" / Int8sl,  # type:ignore
                    "freqoff" / Int16ul,  # type:ignore
                    "quality0" / Int16ul,  # type:ignore
                    "quality1" / Int16ul,  # type:ignore
                ),
                CommandId.TransmitTest: Struct(
                    "num_sent" / Int32ul,  # type:ignore
                ),
            },
            default=Bytewise(Struct("data" / Int8ul[this._.len - 8])),  # type:ignore
        )
    ),
)


@dataclass
class CommandStatusParamsReceive:
    num_sync: int
    num_hdr_ok: int
    num_crc_ok: int
    pending_rx: bool
    last_rssi: int = 0
    __len__ = lambda x: 8

    def __post_init__(self):
        if isinstance(self.pending_rx, int):
            self.pending_rx = True if self.pending_rx != 0 else False


@dataclass
class CommandStatusParamsReceiveTest:
    num_sync: int
    num_hdr_ok: int
    num_crc_ok: int
    pending_rx: bool
    last_rssi: int
    quality0: int
    quality1: int
    freqoff: int
    __len__ = lambda x: 14

    def __post_init__(self):
        if isinstance(self.pending_rx, int):
            self.pending_rx = True if self.pending_rx != 0 else False


@dataclass
class CommandStatusParamsTransmitTest:
    num_sent: int
    __len__ = lambda x: 2


@dataclass
class CommandStatus(BaseRequest):
    slot: int
    status: CommandStatusCode
    chain_action: ChainAction = ChainAction.NoAction
    chain_target: int = 0
    len: int = 8
    id: int = Request.COMMAND_STATUS
    cmd_id: int = 0
    params: Union[
        GenericParams, CommandStatusParamsReceive, CommandStatusParamsReceiveTest, CommandStatusParamsTransmitTest
    ] = field(default_factory=GenericParams)

    def __post_init__(self):
        if self.len == 4 and hasattr(self.params, "__len__"):
            self.len += len(self.params)  # type:ignore
        if isinstance(self.params, ListContainer):
            self.params = GenericParams(list(self.params))

    def __repr__(self):
        return f"{type(self).__name__}(slot={self.slot:02X}, cmd_id={self.cmd_id}, status={CommandStatusCode(int(self.status)).name}, params={self.params}, chain_action={self.chain_action}, chain_target={self.chain_target})"

    def __eq__(self, other: "CommandStatus"):
        """
        Equality operator for use with tests
        """
        if not isinstance(other, self.__class__):
            return False
        return self.id == other.id and self.slot == other.slot and self.status == other.status

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "params":
            lookup = defaultdict(
                lambda: GenericParams,
                {
                    CommandId.Receive: CommandStatusParamsReceive,
                    CommandId.ReceiveTest: CommandStatusParamsReceiveTest,
                    CommandId.TransmitTest: CommandStatusParamsTransmitTest,
                    CommandId.DualReceive: CommandStatusParamsReceive,
                    CommandId.ExtReceive: CommandStatusParamsReceive,
                },
            )
            return lookup[self_dictionary["cmd_id"]]


class StreamRetention(enum.IntEnum):
    Flush_On_Success = 0
    Flush_Streaming = 1
    Flush_Never = 2
    Flush_Streaming_Continuous = 3


E8BitStreamRetention_ = Enum(BitsInteger(8), StreamRetention)
E8BitStreamRetention = Mapping(
    E8BitStreamRetention_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {StreamRetention(k): v for k, v in E8BitStreamRetention_.decmapping.items()},  # type:ignore
    ),
)

## STREAM_STORE
##
SStreamStore = "root" / BitStruct(
    "len" / BitsInteger(8),
    "id" / Const(Request.STREAM_STORE, Bytewise(ERequest)),
    "md" / BitsInteger(1),
    "start" / BitsInteger(1),
    "stream_id" / BitsInteger(6),
    "retention" / E8BitStreamRetention,
    "data" / BitsInteger(8)[this.len - 4],
)


@dataclass
class StreamStore(BaseRequest):
    stream_id: int
    start: int
    md: int
    retention: StreamRetention = StreamRetention.Flush_On_Success
    data: Union[bytes, List[int]] = field(default_factory=list)
    len: int = 4
    id: int = Request.STREAM_STORE

    def __post_init__(self):
        self.len += len(self.data)
        self.data = list(self.data)
        if isinstance(self.retention, int):
            self.retention = StreamRetention(self.retention)

    def __repr__(self):
        return f"{type(self).__name__}(stream_id={self.stream_id:02X}, start={self.start:02X}, md={self.md:02X}, data={repr(self.data)})"

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "data":
            return list


## Stream FLush
##
SStreamFlush = Struct(
    "len" / Const(3, Int8ul),
    "id" / Const(Request.STREAM_FLUSH, ERequest),
    "stream_id" / Int8ul,  # type:ignore
)


@dataclass
class StreamFlush(BaseRequest):
    stream_id: int
    len: int = 3
    id: int = Request.STREAM_FLUSH

    def __repr__(self):
        return f"{type(self).__name__}(stream_id={self.stream_id:02X})"


## CONFIG_STORE
##
SConfigStore = "root" / BitStruct(
    "len" / BitsInteger(8),
    "id" / Const(Request.CONFIG_STORE, Bytewise(ERequest)),
    "md" / BitsInteger(1),
    "start" / BitsInteger(1),
    "config_id" / BitsInteger(6),
    "reserved" / Const(0, BitsInteger(8)),
    "reference" / BitsInteger(32),
    "data" / BitsInteger(8)[this.len - 8],
)


@dataclass
class ConfigStore(BaseRequest):
    config_id: int
    start: int
    md: int
    data: Union[bytes, List[int]] = field(default_factory=list)
    reference: int = 0
    len: int = 8
    id: int = Request.CONFIG_STORE

    def __post_init__(self):
        self.len += len(self.data)
        self.data = list(self.data)

    def __repr__(self):
        return f"{type(self).__name__}(config_id={self.config_id:02X}, start={self.start:02X}, md={self.md:02X}, ref={self.reference:04X}, data={repr(self.data)})"

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "data":
            return list


## CONFIG_STORE_INTERNAL
##
SConfigStoreInternal = "root" / BitStruct(
    "len" / BitsInteger(16, swapped=True),
    "md" / BitsInteger(1),
    "start" / BitsInteger(1),
    "config_id" / BitsInteger(6),
    "reserved" / Const(0, BitsInteger(8)),
    "reference" / BitsInteger(32),
    "data" / BitsInteger(8)[this.len - 8],
)


@dataclass
class ConfigStoreInternal(BaseRequest):
    config_id: int
    start: int
    md: int
    data: Union[bytes, List[int]] = field(default_factory=list)
    reference: int = 0
    len: int = 8

    def __post_init__(self):
        self.len += len(self.data)
        self.data = list(self.data)

    def __repr__(self):
        return f"{type(self).__name__}(config_id={self.config_id:02X}, start={self.start:02X}, md={self.md:02X}, ref={self.reference:04X}, data={repr(self.data)})"

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "data":
            return list


## Config FLush
##
SConfigFlush = Struct(
    "len" / Const(3, Int8ul),
    "id" / Const(Request.CONFIG_FLUSH, ERequest),
    "config_id" / Int8ul,  # type:ignore
)


@dataclass
class ConfigFlush(BaseRequest):
    config_id: int
    len: int = 3
    id: int = Request.CONFIG_FLUSH

    def __repr__(self):
        return f"{type(self).__name__}(config_id={self.config_id:02X})"


## Config Persist
##
SConfigPersist = Struct(
    "len" / Const(3, Int8ul),
    "id" / Const(Request.CONFIG_PERSIST, ERequest),
    "config_id" / Int8ul,  # type:ignore
)


@dataclass
class ConfigPersist(BaseRequest):
    config_id: int
    len: int = 3
    id: int = Request.CONFIG_PERSIST

    def __repr__(self):
        return f"{type(self).__name__}(config_id={self.config_id:02X})"


## Config List
##
SConfigList = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.CONFIG_LIST, ERequest),
)


@dataclass
class ConfigListReq(BaseRequest):
    len: int = 2
    id: int = Request.CONFIG_LIST

    def __repr__(self):
        return f"{type(self).__name__}()"


## CONFIG_LIST_RESPONSE
##
SStreamDescriptor = ByteSwapped(
    BitStruct(
        "reference" / BitsInteger(32, swapped=True),
        "length" / BitsInteger(16, swapped=False),
        "reserved" / BitsInteger(9),
        "persisted" / BitsInteger(1),
        "stream_id" / BitsInteger(6),
    )
)


@dataclass
class StreamDescriptor:
    stream_id: int
    persisted: bool
    length: int
    reference: int
    reserved: int = 0

    def __post_init__(self):
        self.persisted = self.persisted != 0

    def __repr__(self):
        return f"{type(self).__name__}(stream_id={self.stream_id}, persisted={self.persisted}, length={self.length}, reference={self.reference:08x})"


SConfigListResponse = "root" / Struct(
    "len" / Int8ul,  # type:ignore
    "id" / Const(Request.CONFIG_LIST_RESPONSE, ERequest),
    "start_at" / Int8ul,  # type:ignore
    "count" / Int8ul,  # type:ignore
    "streams" / SStreamDescriptor[this.count],
)


@dataclass
class ConfigListResponse(BaseRequest):
    start_at: int
    count: int
    streams: List[StreamDescriptor]
    len: int = 8
    id: int = Request.CONFIG_LIST_RESPONSE

    def __post_init__(self):
        self.len += len(self.streams) * 8

    def __repr__(self):
        return f"{type(self).__name__}(start_at={self.start_at}, count={self.count}, streams={self.streams})"

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "streams":
            return List[StreamDescriptor]


## GetTime
##
SGetTime = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.UTIL_GETTIME, ERequest),
)


@dataclass
class GetTime(BaseRequest):
    len: int = 2
    id: int = Request.UTIL_GETTIME

    def __repr__(self):
        return f"{type(self).__name__}()"


## GetRssi
##
SGetRssi = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.UTIL_GETRSSI, ERequest),
)


@dataclass
class GetRssi(BaseRequest):
    len: int = 2
    id: int = Request.UTIL_GETRSSI

    def __repr__(self):
        return f"{self.__class__.__name__}()"


## GetVersion
##
SGetVersion = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.UTIL_INFO_VERSION, ERequest),
)


@dataclass
class GetVersion(BaseRequest):
    len: int = 2
    id: int = Request.UTIL_INFO_VERSION

    def __repr__(self):
        return f"{type(self).__name__}()"


## GetMacUuid
##
SgetMacUuid = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.UTIL_INFO_MAC_UUID, ERequest),
)


@dataclass
class GetMacUuid(BaseRequest):
    len: int = 2
    id: int = Request.UTIL_INFO_MAC_UUID

    def __repr__(self):
        return f"{type(self).__name__}()"


## GetDevice
##
SGetDevice = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.UTIL_INFO_DEVICE, ERequest),
)


@dataclass
class GetDevice(BaseRequest):
    len: int = 2
    id: int = Request.UTIL_INFO_DEVICE

    def __repr__(self):
        return f"{type(self).__name__}()"


## PowerMode
##
class PowerPolicy(enum.IntEnum):
    StandbyDisallow = 0
    StandbyAllow = 1


E8BitPowerPolicy_ = Enum(BitsInteger(8), PowerPolicy)
E8BitPowerPolicy = Mapping(
    E8BitPowerPolicy_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {PowerPolicy(k): v for k, v in E8BitPowerPolicy_.decmapping.items()},  # type:ignore
    ),
)


SPowerModeConfig = BitStruct(
    "dwellTimeUs" / Bytewise(Int16ul),  # type:ignore
    "powerPolicy" / E8BitPowerPolicy,
)


SPowerMode = BitStruct(
    "len" / Const(5, Bytewise(Int8ul)),
    "id" / Const(Request.UTIL_POWER_MODE, Bytewise(ERequest)),
    "mode" / Bytewise(SPowerModeConfig),
)


@dataclass
class PowerModeConfig:
    dwellTimeUs: int = 0  # type:ignore
    powerPolicy: PowerPolicy = PowerPolicy.StandbyDisallow  # type:ignore


@dataclass
class PowerMode(BaseRequest):
    mode: PowerModeConfig = field(
        default_factory=lambda: PowerModeConfig(dwellTimeUs=0, powerPolicy=PowerPolicy.StandbyDisallow)
    )
    len: int = 5
    id: int = Request.UTIL_POWER_MODE

    def __repr__(self):
        return f"{type(self).__name__}(powerPolicy={self.mode.powerPolicy:02X}, dwellTimeUs={self.mode.dwellTimeUs})"


## RfMode
##
class RfModeType(enum.IntEnum):
    SUN = 0
    PropFSK = 1
    OOK = 2
    XLR = 3
    WMBus = 4
    FM = 5
    PropFSKPeakAGC = 6
    SUNPeakAGC = 7


E8BitRfModeType_ = Enum(BitsInteger(8), RfModeType)
E8BitRfModeType = Mapping(
    E8BitRfModeType_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {RfModeType(k): v for k, v in E8BitRfModeType_.decmapping.items()},  # type:ignore
    ),
)


SRfMode = BitStruct(
    "len" / Const(3, Bytewise(Int8ul)),
    "id" / Const(Request.UTIL_RF_MODE, Bytewise(ERequest)),
    "mode" / E8BitRfModeType,
)


@dataclass
class RfMode(BaseRequest):
    mode: int = RfModeType.SUN  # type:ignore
    len: int = 3
    id: int = Request.UTIL_RF_MODE

    def __repr__(self):
        return f"{type(self).__name__}(mode={self.mode})"


## Shutdown
##
SShutdown = BitStruct(
    "len" / Const(2, Bytewise(Int8ul)),
    "id" / Const(Request.UTIL_SHUTDOWN, Bytewise(ERequest)),
)


@dataclass
class Shutdown(BaseRequest):
    len: int = 2
    id: int = Request.UTIL_SHUTDOWN


## EnterSsbl
##
SEnterSsbl = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.UTIL_ENTER_SSBL, ERequest),
)


@dataclass
class EnterSsbl(BaseRequest):
    len: int = 2
    id: int = Request.UTIL_ENTER_SSBL

    def __repr__(self):
        return f"{type(self).__name__}()"


## PaTable
##
SPaTable = Struct(
    "num_entries" / Int32ul,  # type:ignore
    "power_table"
    / GreedyRange(
        BitStruct(
            "power_code" / Bytewise(Int8sl),  # type:ignore
            "value" / Bytewise(Int8ul),  # type:ignore
            "power"
            / Computed(
                this.power_code / 2 if this.power_code not in [x.value for x in TxPowerSpecial] else this.power_code
            ),
        )
    ),
)


@dataclass
class PaEntry:
    value: int
    power: Union[float, TxPowerSpecial] = 0
    power_code: int = field(init=False)

    def __post_init__(self):
        self.power_code = (
            round(self.power * 2) if self.power not in [x.value for x in TxPowerSpecial] else round(self.power)
        )

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "power":
            if self_dictionary["power_code"] in [x.value for x in TxPowerSpecial]:
                return TxPowerSpecial
            return float


@dataclass
class PaTable:
    power_table: List[PaEntry]
    num_entries: int = field(init=False)

    def __post_init__(self):
        self.num_entries = len(self.power_table)
        super().__setattr__("power_table", [asdict(x) for x in self.power_table])

    def build(self):
        return SPaTable.build(asdict(self))


## Preamble-length delta settings-segment
##
SPreambleLengthSegmentTyr = Struct(
    Const(2, Int32ul),
    Const(0x21501000, Int32ul),
    "swreps" / Int32ul,  # type:ignore
)


SPreambleLengthSegmentHugin = Struct(
    Const(2, Int32ul),
    Const(0x23A01000, Int32ul),
    "swreps" / Int32ul,  # type:ignore
)


class PreambleLengthSegment:
    def __init__(self, num_bytes: Optional[int] = None, msec: Optional[int] = None, kbps: Optional[int] = None) -> None:
        if num_bytes:
            self.swreps = num_bytes - 1
        elif msec:
            assert kbps is not None
            self.swreps = int((msec * kbps) / 8 + 0.5)
            assert self.swreps < 2**16
        else:
            assert False

    def build(self, device):
        if "CC1308" in device:
            return SPreambleLengthSegmentTyr.build(dict(swreps=self.swreps))
        elif "CC1407" in device:
            return SPreambleLengthSegmentHugin.build(dict(swreps=self.swreps))
        else:
            raise Exception("%s is an unsupported device in this test" % (device))

    def __repr__(self) -> str:
        return f"PreambleLengthSegment(swreps={self.swreps})"


class ConfigRegionOperation(enum.IntEnum):
    HW_Region_Clear = 0
    HW_Write_16bit = 1
    HW_Write_32bit = 2
    HW_Write_16bit_sparse = 3
    SW_Region_Clear = 4
    SW_Write_16bit = 5
    SW_Write_32bit = 6
    SW_Write_16bit_sparse = 7
    Par_Region_Clear = 8
    Par_Reference_32bit = 9
    Par_Write_32bit = 10
    LRF_RegionOperation_Skip = 254
    LRF_RegionOperation_Invalid = 255


E4BitConfigRegionOperation_ = Enum(BitsInteger(4), ConfigRegionOperation)
E4BitConfigRegionOperation = Mapping(
    E4BitConfigRegionOperation_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {ConfigRegionOperation(k): v for k, v in E4BitConfigRegionOperation_.decmapping.items()},  # type:ignore
    ),
)


SConfigWord = construct.Union(
    0,
    "segment"
    / ByteSwapped(
        BitStruct(
            "feature_mask" / BitsInteger(16),
            "compound" / BitsInteger(1),
            "inverted" / BitsInteger(1),
            "reserved" / BitsInteger(1),
            "length" / BitsInteger(13),
        )
    ),
    "region"
    / ByteSwapped(
        BitStruct(
            "start_address" / BitsInteger(16),
            "operation" / E4BitConfigRegionOperation,
            "length_minus_1" / BitsInteger(12),
        )
    ),
    "value32" / Int32ul,  # type:ignore
    "value16" / Int16ul[2],  # type:ignore
    "sparse"
    / Struct(
        "value16" / Int16ul,  # type:ignore
        "address" / Int16ul,  # type:ignore
    ),
)


@dataclass
class ConfigSegment:
    length: int
    feature_mask: int = 0
    inverted: bool = False
    compound: bool = False

    def configdict(self):
        return dict(
            segment=dict(
                length=self.length,
                reserved=0,
                inverted=self.inverted,
                compound=self.compound,
                feature_mask=self.feature_mask,
            )
        )


@dataclass
class ConfigRegion:
    length: int
    operation: ConfigRegionOperation
    address: int = 0

    def configdict(self):
        return dict(region=dict(length_minus_1=self.length - 1, operation=self.operation, start_address=self.address))


@dataclass
class ConfigValue32:
    value32: int

    def configdict(self):
        return dict(value32=self.value32)

    @staticmethod
    def list_from_bytes(b: bytes):
        return [ConfigValue32(int.from_bytes(b[i : i + 4], byteorder="little")) for i in range(0, len(b), 4)]


@dataclass
class ConfigValue16:
    value16: List[int]

    def configdict(self):
        return dict(value16=self.value16)


@dataclass
class ConfigSparse:
    address: int
    value16: int

    def configdict(self):
        return dict(sparse=dict(address=self.address, value16=self.value16))


class ConfigList:
    def __init__(self, list):
        self.list = list

    def build(self):
        return b"".join([SConfigWord.build(x.configdict()) for x in self.list])

    def as_commented_list(self):
        return [(SConfigWord.build(x.configdict()), repr(x)) for x in self.list]

    def __str__(self) -> str:
        return str(self.list)


## NvAllocate
##
SNvAllocate = Struct(
    "len" / Const(6, Int8ul),
    "id" / Const(Request.NV_ALLOCATE, ERequest),
    "record_id" / Int16ul,  # type:ignore
    "length" / Int16ul,  # type:ignore
)


@dataclass
class NvAllocate(BaseRequest):
    record_id: int = 0
    length: int = 0
    len: int = 6
    id: int = Request.NV_ALLOCATE

    def __repr__(self):
        return f"{type(self).__name__}(record_id=0x{self.record_id:04x}, length={self.length})"


## NvWrite
##
SNvWrite = Struct(
    "len" / Int8ul,  # type:ignore
    "id" / Const(Request.NV_WRITE, ERequest),
    "record_id" / Int16ul,  # type:ignore
    "offset" / Int16ul,  # type:ignore
    "reserved" / Int16ul,  # type:ignore
    "data" / Int8ul[this.len - 8],  # type:ignore
)


@dataclass
class NvWrite(BaseRequest):
    record_id: int = 0
    offset: int = 0
    data: Union[bytes, List[int]] = field(default_factory=list)
    len: int = 8
    id: int = Request.NV_WRITE
    reserved: int = 0

    def __post_init__(self):
        self.len += len(self.data)
        self.data = list(self.data)

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "data":
            return list

    def __repr__(self):
        return f"{type(self).__name__}(record_id=0x{self.record_id:04x}, offset={self.offset}, data={nice(self.data)})"


## NvFinalize
##
SNvFinalize = Struct(
    "len" / Const(4, Int8ul),
    "id" / Const(Request.NV_FINALIZE, ERequest),
    "record_id" / Int16ul,  # type:ignore
)


@dataclass
class NvFinalize(BaseRequest):
    record_id: int = 0
    len: int = 4
    id: int = Request.NV_FINALIZE

    def __repr__(self):
        return f"{type(self).__name__}(record_id=0x{self.record_id:04x})"


## NvInvalidate
##
SNvInvalidate = Struct(
    "len" / Const(4, Int8ul),
    "id" / Const(Request.NV_INVALIDATE, ERequest),
    "record_id" / Int16ul,  # type:ignore
)


@dataclass
class NvInvalidate(BaseRequest):
    record_id: int = 0
    len: int = 4
    id: int = Request.NV_INVALIDATE

    def __repr__(self):
        return f"{type(self).__name__}(record_id=0x{self.record_id:04x})"


## NvGet
##
SNvGet = Struct(
    "len" / Const(6, Int8ul),
    "id" / Const(Request.NV_GET, ERequest),
    "record_id" / Int16ul,  # type:ignore
    "record_id_mask" / Int16ul,  # type:ignore
)


@dataclass
class NvGet(BaseRequest):
    record_id: int = 0
    record_id_mask: int = 0
    len: int = 6
    id: int = Request.NV_GET

    def __repr__(self):
        return f"{type(self).__name__}(record_id=0x{self.record_id:04x}, mask={self.record_id_mask})"


## NvInfo
##
SNvInfo = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.NV_INFO, ERequest),
)


@dataclass
class NvInfo(BaseRequest):
    len: int = 2
    id: int = Request.NV_INFO

    def __repr__(self):
        return f"{type(self).__name__}()"


## NvErase
##
SNvErase = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.NV_ERASE, ERequest),
)


@dataclass
class NvCorrupt(BaseRequest):
    len: int = 2
    id: int = Request.NV_CORRUPT

    def __repr__(self):
        return f"{type(self).__name__}()"


## NvCorrupt
##
SNvCorrupt = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.NV_CORRUPT, ERequest),
)


@dataclass
class NvErase(BaseRequest):
    len: int = 2
    id: int = Request.NV_ERASE

    def __repr__(self):
        return f"{type(self).__name__}()"


## IoSetup
##
class Dio0Config(enum.IntEnum):
    DIO0_GPIO_SET_0 = 0x00
    DIO0_GPIO_SET_1 = 0x01
    DIO0_LOW_PA_AND_HIGH_PA = 0x11
    DIO0_LOW_PA_AND_LNA = 0x12
    DIO0_HIGH_PA_AND_LNA = 0x13
    DIO0_HIGH_PA_AND_LOW_PA_AND_LNA = 0x14
    DIO0_TRX_AWAKE = 0x18
    DIO0_HIGH_Z = 0x30
    DIO0_INPUT_TRIGGER = 0x31
    DIO0_SFD = 0x41
    DIO0_ENERGY_DETECT = 0x42
    DIO0_PREAMBLE_DETECT = 0x43
    DIO0_LNA = 0x52
    DIO0_LOW_PA = 0x53
    DIO0_HIGH_PA = 0x54
    DIO0_RF_TRACE = 0x69
    DIO0_UNCHANGED = 0xFF


class Dio2Config(enum.IntEnum):
    DIO2_GPIO_SET_0 = 0x00
    DIO2_GPIO_SET_1 = 0x01
    DIO2_LOW_PA_AND_HIGH_PA = 0x11
    DIO2_LOW_PA_AND_LNA = 0x12
    DIO2_HIGH_PA_AND_LNA = 0x13
    DIO2_HIGH_PA_AND_LOW_PA_AND_LNA = 0x14
    DIO2_TRX_AWAKE = 0x18
    DIO2_TCXO_EN = 0x23
    DIO2_HIGH_Z = 0x30
    DIO2_INPUT_TRIGGER = 0x31
    DIO2_SFD = 0x41
    DIO2_ENERGY_DETECT = 0x42
    DIO2_PREAMBLE_DETECT = 0x43
    DIO2_LNA = 0x52
    DIO2_LOW_PA = 0x53
    DIO2_HIGH_PA = 0x54
    DIO2_RF_TRACE = 0x69
    DIO2_UNCHANGED = 0xFF


class Dio3Config(enum.IntEnum):
    DIO3_GPIO_SET_0 = 0x00
    DIO3_GPIO_SET_1 = 0x01
    DIO3_LOW_PA_AND_HIGH_PA = 0x11
    DIO3_LOW_PA_AND_LNA = 0x12
    DIO3_HIGH_PA_AND_LNA = 0x13
    DIO3_HIGH_PA_AND_LOW_PA_AND_LNA = 0x14
    DIO3_TRX_AWAKE = 0x18
    DIO3_TCXO_EN = 0x23
    DIO3_HIGH_Z = 0x30
    DIO3_INPUT_TRIGGER = 0x31
    DIO3_SFD = 0x41
    DIO3_ENERGY_DETECT = 0x42
    DIO3_PREAMBLE_DETECT = 0x43
    DIO3_LNA = 0x52
    DIO3_LOW_PA = 0x53
    DIO3_HIGH_PA = 0x54
    DIO3_RF_TRACE = 0x69
    DIO3_UNCHANGED = 0xFF


class Dio4Config(enum.IntEnum):
    DIO4_GPIO_SET_0 = 0x00
    DIO4_GPIO_SET_1 = 0x01
    DIO4_LOW_PA_AND_HIGH_PA = 0x11
    DIO4_LOW_PA_AND_LNA = 0x12
    DIO4_HIGH_PA_AND_LNA = 0x13
    DIO4_HIGH_PA_AND_LOW_PA_AND_LNA = 0x14
    DIO4_TRX_AWAKE = 0x18
    DIO4_HIGH_Z = 0x30
    DIO4_INPUT_TRIGGER = 0x31
    DIO4_CKMIN = 0x32
    DIO4_LFCI = 0x33
    DIO4_SFD = 0x41
    DIO4_ENERGY_DETECT = 0x42
    DIO4_PREAMBLE_DETECT = 0x43
    DIO4_LNA = 0x52
    DIO4_LOW_PA = 0x53
    DIO4_HIGH_PA = 0x54
    DIO4_RF_TRACE = 0x69
    DIO4_UNCHANGED = 0xFF


class Dio5Config(enum.IntEnum):
    DIO5_GPIO_SET_0 = 0x00
    DIO5_GPIO_SET_1 = 0x01
    DIO5_LOW_PA_AND_HIGH_PA = 0x11
    DIO5_LOW_PA_AND_LNA = 0x12
    DIO5_HIGH_PA_AND_LNA = 0x13
    DIO5_HIGH_PA_AND_LOW_PA_AND_LNA = 0x14
    DIO5_TRX_AWAKE = 0x18
    DIO5_HIGH_Z = 0x30
    DIO5_INPUT_TRIGGER = 0x31
    DIO5_CKMIN = 0x32
    DIO5_LFCI = 0x33
    DIO5_SFD = 0x41
    DIO5_ENERGY_DETECT = 0x42
    DIO5_PREAMBLE_DETECT = 0x43
    DIO5_LNA = 0x52
    DIO5_LOW_PA = 0x53
    DIO5_HIGH_PA = 0x54
    DIO5_RF_TRACE = 0x69
    DIO5_SWD = 0xFE
    DIO5_UNCHANGED = 0xFF


class Dio6Config(enum.IntEnum):
    DIO6_GPIO_SET_0 = 0x00
    DIO6_GPIO_SET_1 = 0x01
    DIO6_LOW_PA_AND_HIGH_PA = 0x11
    DIO6_LOW_PA_AND_LNA = 0x12
    DIO6_HIGH_PA_AND_LNA = 0x13
    DIO6_HIGH_PA_AND_LOW_PA_AND_LNA = 0x14
    DIO6_TRX_AWAKE = 0x18
    DIO6_TCXO_EN = 0x23
    DIO6_HIGH_Z = 0x30
    DIO6_INPUT_TRIGGER = 0x31
    DIO6_CKMIN = 0x32
    DIO6_SFD = 0x41
    DIO6_ENERGY_DETECT = 0x42
    DIO6_PREAMBLE_DETECT = 0x43
    DIO6_LNA = 0x52
    DIO6_LOW_PA = 0x53
    DIO6_HIGH_PA = 0x54
    DIO6_RF_TRACE = 0x69
    DIO6_SWD = 0xFE
    DIO6_UNCHANGED = 0xFF


class Dio7Config(enum.IntEnum):
    DIO7_GPIO_SET_0 = 0x00
    DIO7_GPIO_SET_1 = 0x01
    DIO7_LOW_PA_AND_HIGH_PA = 0x11
    DIO7_LOW_PA_AND_LNA = 0x12
    DIO7_HIGH_PA_AND_LNA = 0x13
    DIO7_HIGH_PA_AND_LOW_PA_AND_LNA = 0x14
    DIO7_TRX_AWAKE = 0x18
    DIO7_HIGH_Z = 0x30
    DIO7_INPUT_TRIGGER = 0x31
    DIO7_CKMIN = 0x32
    DIO7_LFCI = 0x33
    DIO7_SFD = 0x41
    DIO7_ENERGY_DETECT = 0x42
    DIO7_PREAMBLE_DETECT = 0x43
    DIO7_LNA = 0x52
    DIO7_LOW_PA = 0x53
    DIO7_HIGH_PA = 0x54
    DIO7_RF_TRACE = 0x69
    DIO7_UNCHANGED = 0xFF


E8BitDio0Config_ = Enum(BitsInteger(8), Dio0Config)
E8BitDio0Config = Mapping(
    E8BitDio0Config_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {Dio0Config(k): v for k, v in E8BitDio0Config_.decmapping.items()},  # type:ignore
    ),
)

E8BitDio2Config_ = Enum(BitsInteger(8), Dio2Config)
E8BitDio2Config = Mapping(
    E8BitDio2Config_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {Dio2Config(k): v for k, v in E8BitDio2Config_.decmapping.items()},  # type:ignore
    ),
)

E8BitDio3Config_ = Enum(BitsInteger(8), Dio3Config)
E8BitDio3Config = Mapping(
    E8BitDio3Config_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {Dio3Config(k): v for k, v in E8BitDio3Config_.decmapping.items()},  # type:ignore
    ),
)

E8BitDio4Config_ = Enum(BitsInteger(8), Dio4Config)
E8BitDio4Config = Mapping(
    E8BitDio4Config_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {Dio4Config(k): v for k, v in E8BitDio4Config_.decmapping.items()},  # type:ignore
    ),
)

E8BitDio5Config_ = Enum(BitsInteger(8), Dio5Config)
E8BitDio5Config = Mapping(
    E8BitDio5Config_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {Dio5Config(k): v for k, v in E8BitDio5Config_.decmapping.items()},  # type:ignore
    ),
)

E8BitDio6Config_ = Enum(BitsInteger(8), Dio6Config)
E8BitDio6Config = Mapping(
    E8BitDio6Config_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {Dio6Config(k): v for k, v in E8BitDio6Config_.decmapping.items()},  # type:ignore
    ),
)

E8BitDio7Config_ = Enum(BitsInteger(8), Dio7Config)
E8BitDio7Config = Mapping(
    E8BitDio7Config_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {Dio7Config(k): v for k, v in E8BitDio7Config_.decmapping.items()},  # type:ignore
    ),
)

SDioConfig = Struct(
    "dio0" / E8BitDio0Config,
    "dio2" / E8BitDio2Config,
    "dio3" / E8BitDio3Config,
    "dio4" / E8BitDio4Config,
    "dio5" / E8BitDio5Config,
    "dio6" / E8BitDio6Config,
    "dio7" / E8BitDio7Config,
)
SDioSetup = BitStruct(
    "len" / Const(9, Bytewise(Int8ul)),
    "id" / Const(Request.DIO_SETUP, Bytewise(ERequest)),
    "dioConfig" / SDioConfig,
)


@dataclass
class DioConfig:
    dio0: Dio0Config = Dio0Config.DIO0_UNCHANGED  # type:ignore
    dio2: Dio2Config = Dio2Config.DIO2_UNCHANGED  # type:ignore
    dio3: Dio3Config = Dio3Config.DIO3_UNCHANGED  # type:ignore
    dio4: Dio4Config = Dio4Config.DIO4_UNCHANGED  # type:ignore
    dio5: Dio5Config = Dio5Config.DIO5_UNCHANGED  # type:ignore
    dio6: Dio6Config = Dio6Config.DIO6_UNCHANGED  # type:ignore
    dio7: Dio7Config = Dio7Config.DIO7_UNCHANGED  # type:ignore


@dataclass
class DioSetup(BaseRequest):
    dioConfig: DioConfig = field(default_factory=DioConfig)
    len: int = 9
    id: int = Request.DIO_SETUP

    def __repr__(self):
        return f"{type(self).__name__}(DioConfig(dio0={Dio0Config(int(self.dioConfig.dio0)).name}, dio2={Dio2Config(int(self.dioConfig.dio2)).name}, dio3={Dio3Config(int(self.dioConfig.dio3)).name}, dio4={Dio4Config(int(self.dioConfig.dio4)).name}, dio5={Dio5Config(int(self.dioConfig.dio5)).name}, dio6={Dio6Config(int(self.dioConfig.dio6)).name}, dio7={Dio7Config(int(self.dioConfig.dio7)).name}))"


## DEVICE_CONFIG_STORE
##
class ClockConfigType(enum.IntEnum):
    RCOSC = 0
    XOSC = 1
    TCXO = 2


E8BitClockConfigType_ = Enum(BitsInteger(8), ClockConfigType)
E8BitClockConfigType = Mapping(
    E8BitClockConfigType_,
    defaultdict(
        lambda x: str(x),  # type:ignore
        {ClockConfigType(k): v for k, v in E8BitClockConfigType_.decmapping.items()},  # type:ignore
    ),
)


SDeviceConfigData = BitStruct(
    "dioConfig" / SDioConfig,
    "clockConfig" / E8BitClockConfigType,
    "powerMode" / Bytewise(SPowerModeConfig),
    "rfMode" / E8BitRfModeType,
)


@dataclass
class DeviceConfigData:
    dioConfig: DioConfig = field(default_factory=DioConfig)
    clockConfig: int = ClockConfigType.XOSC  # type:ignore
    powerMode: PowerModeConfig = field(default_factory=PowerModeConfig)
    rfMode: int = RfModeType.SUN  # type:ignore


SDeviceConfigStore = "root" / BitStruct(
    "len" / Const(14, BitsInteger(8)),
    "id" / Const(Request.DEVICE_CONFIG_STORE, Bytewise(ERequest)),
    "data" / Bytewise(SDeviceConfigData),
)


@dataclass
class DeviceConfigStore(BaseRequest):
    data: DeviceConfigData = field(default_factory=DeviceConfigData)
    len: int = 14
    id: int = Request.DEVICE_CONFIG_STORE

    def __repr__(self):
        return f"{type(self).__name__}(data={repr(self.data)})"

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "data":
            return DeviceConfigData


## Device Config Persist
##
SDeviceConfigPersist = BitStruct(
    "len" / Const(3, BitsInteger(8)),
    "id" / Const(Request.DEVICE_CONFIG_PERSIST, Bytewise(ERequest)),
    "reserved" / BitsInteger(7),  # type:ignore
    "allowReplace" / BitsInteger(1),  # type:ignore
)


@dataclass
class DeviceConfigPersist(BaseRequest):
    len: int = 3
    id: int = Request.DEVICE_CONFIG_PERSIST
    allowReplace: bool = False
    reserved: int = 0

    def __post_init__(self):
        self.allowReplace = self.allowReplace != 0

    def __repr__(self):
        return f"{type(self).__name__}(allowReplace={self.allowReplace})"


## Device Config Get
##
SDeviceConfigGet = Struct(
    "len" / Const(2, Int8ul),
    "id" / Const(Request.DEVICE_CONFIG_GET, ERequest),
)


@dataclass
class DeviceConfigGetReq(BaseRequest):
    len: int = 2
    id: int = Request.DEVICE_CONFIG_GET

    def __repr__(self):
        return f"{type(self).__name__}()"


## DEVICE_CONFIG_GET_RESPONSE
##

SDeviceConfigGetResponse = "root" / BitStruct(
    "len" / Const(16, BitsInteger(8)),  # type:ignore
    "id" / Const(Request.DEVICE_CONFIG_GET_RESPONSE, Bytewise(ERequest)),
    "reserved0" / BitsInteger(6),  # type:ignore
    "modified" / BitsInteger(1),  # type:ignore
    "persisted" / BitsInteger(1),  # type:ignore
    "reserved1" / BitsInteger(8),  # type:ignore
    "data" / Bytewise(SDeviceConfigData),  # type:ignore
)


@dataclass
class DeviceConfigGetResponse(BaseRequest):
    persisted: bool
    modified: bool
    data: DeviceConfigData
    len: int = 16
    id: int = Request.DEVICE_CONFIG_GET_RESPONSE
    reserved0: int = 0
    reserved1: int = 0

    def __post_init__(self):
        self.persisted = self.persisted != 0
        self.modified = self.modified != 0
        if isinstance(self.data, Container):
            c = self.data
            dioConfig = DioConfig(
                dio0=c.dioConfig.dio0,
                dio2=c.dioConfig.dio2,
                dio3=c.dioConfig.dio3,
                dio4=c.dioConfig.dio4,
                dio5=c.dioConfig.dio5,
                dio6=c.dioConfig.dio6,
                dio7=c.dioConfig.dio7,
            )
            powerMode = PowerModeConfig(
                dwellTimeUs=c.powerMode.dwellTimeUs,
                powerPolicy=c.powerMode.powerPolicy,
            )
            self.data = DeviceConfigData(
                dioConfig=dioConfig,
                clockConfig=c.clockConfig,
                powerMode=powerMode,
                rfMode=c.rfMode,
            )

    def __repr__(self):
        return f"{type(self).__name__}(persisted={self.persisted}, modified={self.modified}, data={repr(self.data)})"

    @classmethod
    def __resolve_child_type(cls, self_dictionary, child):
        if child == "data":
            return list


SelectRequest = Select(
    SPingRequest,
    SLastStatus,
    STransportError,
    SCommandStoreCommon,
    SCommandSubmit,
    SCommandStop,
    SCommandStatus,
    SStreamStore,
    SStreamFlush,
    SConfigStore,
    SConfigFlush,
    SConfigPersist,
    SConfigList,
    SConfigListResponse,
    SDeviceConfigStore,
    SDeviceConfigPersist,
    SDeviceConfigGet,
    SDeviceConfigGetResponse,
    SGetTime,
    SGetRssi,
    SGetVersion,
    SgetMacUuid,
    SGetDevice,
    SEnterSsbl,
    SPowerMode,
    SRfMode,
    SShutdown,
    SNvAllocate,
    SNvWrite,
    SNvFinalize,
    SNvInvalidate,
    SNvGet,
    SNvErase,
    SNvInfo,
    SNvCorrupt,
    SDioSetup,
    SRequest,
)
RequestMap = defaultdict(
    lambda: RawRequest,
    {
        Request.LAST_STATUS: LastStatus,
        Request.TRANSPORT_ERROR: TransportError,
        Request.PING: PingRequest,
        Request.COMMAND_STORE: CommandStore,
        Request.COMMAND_SUBMIT: CommandSubmit,
        Request.COMMAND_STOP: CommandStop,
        Request.COMMAND_STATUS: CommandStatus,
        Request.STREAM_STORE: StreamStore,
        Request.STREAM_FLUSH: StreamFlush,
        Request.CONFIG_STORE: ConfigStore,
        Request.CONFIG_FLUSH: ConfigFlush,
        Request.CONFIG_PERSIST: ConfigPersist,
        Request.CONFIG_LIST: ConfigListReq,
        Request.CONFIG_LIST_RESPONSE: ConfigListResponse,
        Request.DEVICE_CONFIG_STORE: DeviceConfigStore,
        Request.DEVICE_CONFIG_PERSIST: DeviceConfigPersist,
        Request.DEVICE_CONFIG_GET: DeviceConfigGetReq,
        Request.DEVICE_CONFIG_GET_RESPONSE: DeviceConfigGetResponse,
        Request.UTIL_GETTIME: GetTime,
        Request.UTIL_GETRSSI: GetRssi,
        Request.UTIL_INFO_VERSION: GetVersion,
        Request.UTIL_INFO_MAC_UUID: GetMacUuid,
        Request.UTIL_INFO_DEVICE: GetDevice,
        Request.UTIL_ENTER_SSBL: EnterSsbl,
        Request.UTIL_POWER_MODE: PowerMode,
        Request.UTIL_RF_MODE: RfMode,
        Request.UTIL_SHUTDOWN: Shutdown,
        Request.NV_ALLOCATE: NvAllocate,
        Request.NV_WRITE: NvWrite,
        Request.NV_FINALIZE: NvFinalize,
        Request.NV_INVALIDATE: NvInvalidate,
        Request.NV_GET: NvGet,
        Request.NV_INFO: NvInfo,
        Request.NV_ERASE: NvErase,
        Request.NV_CORRUPT: NvCorrupt,
        Request.DIO_SETUP: DioSetup,
    },
)
ParseMap = defaultdict(
    lambda: SRequest,
    {
        Request.LAST_STATUS: SLastStatus,
        Request.TRANSPORT_ERROR: STransportError,
        Request.PING: SPingRequest,
        Request.COMMAND_STORE: SCommandStoreCommon,
        Request.COMMAND_SUBMIT: SCommandSubmit,
        Request.COMMAND_STOP: SCommandStop,
        Request.COMMAND_STATUS: SCommandStatus,
        Request.STREAM_STORE: SStreamStore,
        Request.STREAM_FLUSH: SStreamFlush,
        Request.CONFIG_STORE: SConfigStore,
        Request.CONFIG_FLUSH: SConfigFlush,
        Request.CONFIG_PERSIST: SConfigPersist,
        Request.CONFIG_LIST: SConfigList,
        Request.DEVICE_CONFIG_STORE: SDeviceConfigStore,
        Request.DEVICE_CONFIG_PERSIST: SDeviceConfigPersist,
        Request.DEVICE_CONFIG_GET: SDeviceConfigGet,
        Request.DEVICE_CONFIG_GET_RESPONSE: SDeviceConfigGetResponse,
        Request.UTIL_GETTIME: SGetTime,
        Request.UTIL_GETRSSI: SGetRssi,
        Request.UTIL_INFO_VERSION: SGetVersion,
        Request.UTIL_INFO_MAC_UUID: SgetMacUuid,
        Request.UTIL_INFO_DEVICE: SGetDevice,
        Request.UTIL_ENTER_SSBL: SEnterSsbl,
        Request.UTIL_POWER_MODE: SPowerMode,
        Request.UTIL_RF_MODE: SRfMode,
        Request.UTIL_SHUTDOWN: SShutdown,
        Request.NV_ALLOCATE: SNvAllocate,
        Request.NV_WRITE: SNvWrite,
        Request.NV_FINALIZE: SNvFinalize,
        Request.NV_INVALIDATE: SNvInvalidate,
        Request.NV_GET: SNvGet,
        Request.NV_INFO: SNvInfo,
        Request.NV_ERASE: SNvErase,
        Request.NV_CORRUPT: SNvCorrupt,
        Request.DIO_SETUP: SDioSetup,
    },
)

PaddedSelectRequest = FocusedSeq(
    "req",
    "offs" / Tell,  # type:ignore
    "pad" / Padding(((this.offs + 3) & ~3) - this.offs),
    "req" / SelectRequest,
    Check(lambda ctx: ctx._building or ctx.offs - 4 + ctx.req.len <= ctx._.header.trx.len),
)

RequestType = Union[
    LastStatus,
    PingRequest,
    TransportError,
    CommandStore,
    CommandSubmit,
    CommandStop,
    CommandStatus,
    StreamStore,
    StreamFlush,
    ConfigStore,
    ConfigFlush,
    ConfigPersist,
    ConfigListReq,
    DeviceConfigStore,
    DeviceConfigPersist,
    DeviceConfigGetReq,
    DeviceConfigGetResponse,
    GetTime,
    GetRssi,
    GetVersion,
    GetMacUuid,
    GetDevice,
    EnterSsbl,
    PowerMode,
    RfMode,
    NvAllocate,
    NvWrite,
    NvFinalize,
    NvInvalidate,
    NvGet,
    NvInfo,
    NvErase,
    NvCorrupt,
    DioSetup,
]
