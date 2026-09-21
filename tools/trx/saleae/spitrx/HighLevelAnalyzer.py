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

# High Level Analyzer
# For more information and documentation, please go to https://support.saleae.com/extensions/high-level-analyzer-extensions
from dataclasses import asdict
import sys
from pathlib import Path

lib_path = str(Path(__file__).parent.parent.absolute() / "lib")
if lib_path not in sys.path:
    sys.path.append(lib_path)

##
##

import binascii
import enum
import struct
import os
from typing import List
from urllib import request

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, StringSetting, NumberSetting, ChoicesSetting

from construct import Struct
from trx.transaction import SHostHeader, STrxHeader, TrxTransaction, TrxHeader, HostHeader
from trx.trxrequests import PaddedSelectRequest, ParseMap, Request, RequestMap, SRequest, SelectRequest, from_dict


def nice(b):
    return ":".join(["%02X" % x for x in b])


def get_bytes(frames: List[AnalyzerFrame], property: str):
    return bytearray([f.data[property][0] for f in frames])


class ParseState(enum.Enum):
    WAIT_TRANS_HEADER = enum.auto()
    WAIT_REQ_HDR = enum.auto()
    WAIT_REQ_PAYLOAD = enum.auto()
    WAIT_CRC = enum.auto()
    WAIT_PADDING = enum.auto()
    ERROR = enum.auto()
    DONE = enum.auto()


# High level analyzers must subclass the HighLevelAnalyzer class.
class Hla(HighLevelAnalyzer):
    # List of settings that a user can set for this High Level Analyzer.
    # my_number_setting = NumberSetting(min_value=0, max_value=100)
    device_choice = ChoicesSetting(choices=("TRX", "Host"))
    verbose = ChoicesSetting(choices=("False", "True"))

    result_types = {"host": {"format": "{{{data.host}}}"}, "trx": {"format": "{{{data.trx}}}"}}

    def __init__(self):
        """
        Initialize HLA.

        Settings can be accessed using the same name used above.
        """

        self.spi_frames = []
        self.source = "miso" if self.device_choice == "TRX" else "mosi"
        self.value_field = "trx" if self.device_choice == "TRX" else "host"
        self.trans_len = 0
        self.curr_req_len = 0
        self.parse_idx = 0
        self.start_time = 0
        self.is_verbose = self.verbose == "True"

    def decode(self, frame: AnalyzerFrame):
        """
        Process a frame from the input analyzer, and optionally return a single `AnalyzerFrame` or a list of `AnalyzerFrame`s.

        The type and data values in `frame` will depend on the input analyzer.
        """
        if frame.type == "enable":
            self.spi_frames: List[AnalyzerFrame] = []
            self.start_time = frame.start_time
            self.trans_len = 0
            self.curr_req_len = 0
            self.parse_idx = 0
            self.state = ParseState.WAIT_TRANS_HEADER
            if self.is_verbose:
                print("=== New packet ", self.state)
            self.curr_header = None
            self.curr_requests = []
            self.curr_crc = None

        if frame.type == "result":
            self.spi_frames.append(frame)
            unparsed = self.spi_frames[self.parse_idx :]

            if self.is_verbose:
                print(f"[{len(self.spi_frames)} / {self.state.name}]")

            if self.state == ParseState.WAIT_TRANS_HEADER:
                if len(unparsed) >= 4:
                    header_frames = unparsed[:4]
                    data = get_bytes(header_frames, self.source)
                    hdr_name = "Header"

                    try:
                        if self.device_choice == "TRX":
                            c = STrxHeader.parse(data)
                            self.curr_header = TrxHeader.from_container(c)
                            hdr_name = self.curr_header.__class__.__name__
                            hdr_body = str(self.curr_header)[len(hdr_name) :]
                            header_dict = dict(hdr=hdr_body)

                        else:
                            c = SHostHeader.parse(data)
                            self.curr_header = HostHeader.from_container(c)
                            hdr_name = self.curr_header.__class__.__name__
                            hdr_body = str(self.curr_header)[len(hdr_name) :]
                            header_dict = dict(hdr=hdr_body)
                    except Exception as e:
                        print(e)
                        header_dict = dict(failure=str(e), data=nice(data))

                    self.trans_len = data[1]
                    if self.trans_len > 0:
                        self.state = ParseState.WAIT_REQ_HDR
                    else:
                        if all([_ == 0 for _ in data]):
                            self.state = ParseState.DONE
                        else:
                            self.state = ParseState.WAIT_CRC

                    self.parse_idx += 4
                    if self.is_verbose:
                        print(self.state)

                    return AnalyzerFrame(
                        self.value_field,
                        header_frames[0].start_time,
                        header_frames[-1].end_time,
                        {self.value_field: str(self.curr_header)},
                    )

            if self.state == ParseState.WAIT_CRC:
                if len(unparsed) >= 2:
                    crcbytes = unparsed[:2]
                    transaction_bytes = get_bytes(self.spi_frames, self.source)
                    crc = binascii.crc_hqx(transaction_bytes, 0xFFFF)
                    self.curr_crc = "OK" if crc == 0 else "FAIL"
                    self.state = ParseState.DONE
                    self.parse_idx += 2
                    if self.is_verbose:
                        print(self.state)

                    return AnalyzerFrame(
                        self.value_field,
                        crcbytes[0].start_time,
                        crcbytes[-1].end_time,
                        {self.value_field: f"Crc({self.curr_crc})"},
                    )

            if self.state == ParseState.WAIT_REQ_HDR:
                if len(unparsed) >= 1:
                    len_frame = unparsed[0]
                    self.curr_req_len = len_frame.data[self.source][0]

                    if self.curr_req_len >= 2:
                        self.state = ParseState.WAIT_REQ_PAYLOAD
                    else:
                        self.state = ParseState.ERROR

                    self.parse_idx += 0  # Wait for body
                    if self.is_verbose:
                        print(self.state)

            if self.state == ParseState.WAIT_REQ_PAYLOAD:
                if len(unparsed) >= self.curr_req_len:
                    req_frames = unparsed[: self.curr_req_len]
                    data = get_bytes(req_frames, self.source)

                    if len(self.spi_frames) >= (self.trans_len + 4):
                        self.state = ParseState.WAIT_CRC
                    else:
                        if len(self.spi_frames) % 4 == 0:
                            self.state = ParseState.WAIT_REQ_HDR
                        else:
                            self.state = ParseState.WAIT_PADDING

                    self.parse_idx += self.curr_req_len

                    if self.is_verbose:
                        print(self.state, f"len of all {len(self.spi_frames)}, trans_len+4 {self.trans_len + 4}")

                    try:
                        req = SelectRequest.parse(data)
                        req_dataclass = from_dict(RequestMap[req.id], req)
                        self.curr_requests.append(req_dataclass)

                        req_name = req_dataclass.__class__.__name__
                        req_body = str(req_dataclass)[len(req_name) :]

                        return AnalyzerFrame(
                            self.value_field,
                            req_frames[0].start_time,
                            req_frames[-1].end_time,
                            {
                                self.value_field: str(req_dataclass),
                            },
                        )
                    except Exception as e:
                        print(e)
                        if len(data) >= 2:
                            req_id = data[1]
                            print(f"Could not parse: {nice(data)}")
                            req_type = Request.INVALID
                            try:
                                req_type = Request(req_id)
                                req_struct = ParseMap[req_type]
                            except ValueError:
                                print(f"Unknown request id: {req_id}")
                                req_struct = SRequest
                            print(f"Attempting to parse as {req_type.name}")
                            try:
                                print(req_struct.parse(data))
                            except Exception as e:
                                print(e)

                        return AnalyzerFrame(
                            self.value_field,
                            req_frames[0].start_time,
                            req_frames[-1].end_time,
                            {self.value_field: "Request parse error: " + nice(data)},
                        )

            if self.state == ParseState.WAIT_PADDING:
                if len(self.spi_frames) % 4 == 0:
                    if len(self.spi_frames) >= self.trans_len + 4:
                        self.state = ParseState.WAIT_CRC
                    else:
                        self.state = ParseState.WAIT_REQ_HDR

                    self.parse_idx += len(unparsed)

                    if self.is_verbose:
                        print(self.state)
                    return AnalyzerFrame(
                        self.value_field,
                        unparsed[0].start_time,
                        unparsed[-1].end_time,
                        {self.value_field: f"Pad({nice(get_bytes(unparsed, self.source))})"},
                    )

        if frame.type == "disable":
            if self.curr_header is not None:
                print()
                print(self.curr_header)
            for req in self.curr_requests:
                print(f"    {req}")
            if self.curr_crc is not None:
                print(f"    CRC {self.curr_crc}")
