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

from dataclasses import asdict, dataclass
import re
from pathlib import Path
import struct
from typing import Dict, Tuple, List

from trx.trxrequests import ConfigStore, HardcodedPhy, SConfigStoreInternal, ConfigStoreInternal

ROUND32BIT = lambda x: ((x + 3) & ~0x3)


@dataclass
class Config:
    name: str
    data: bytes

    def as_store_requests(self, config_id, reference=0):
        max_request_length = 128 - 4 - 4 - 2
        # Number of bytes packed into StreamStore Requests
        data_packed = 0
        fragments = []
        packet = self.data
        while data_packed < len(packet):
            fragment_size = min(len(packet) - data_packed, max_request_length) & ~0x3
            fragments.append(packet[data_packed : data_packed + fragment_size])
            data_packed += fragment_size
        return [
            ConfigStore(
                config_id=config_id,
                start=1 if idx == 0 else 0,
                md=1 if idx + 1 < len(fragments) else 0,
                data=fragment,
                reference=reference,
            )
            for idx, fragment in enumerate(fragments)
        ]

    def as_nv_entry(self, config_id, reference):
        blob = bytearray(
            SConfigStoreInternal.build(
                asdict(ConfigStoreInternal(config_id=config_id, start=1, md=0, reference=reference, data=self.data))
            )
        )
        headeroffset = ROUND32BIT(len(blob))
        headerpadding = headeroffset - len(blob)
        blob.extend(bytearray([0xFF] * headerpadding))
        hdrlow = len(blob) | (0xC << 12)
        hdrhi = 0xCF00 | config_id
        blob.extend(struct.pack("HH", hdrhi, hdrlow))
        return bytes(blob)


def read_config_directory_with_power_levels(
    path: Path, get_power_levels=True
) -> Tuple[Dict[str, Config], Dict[str, List[float]], Dict[str, List[float]]]:
    c_configs = path.glob("*.c")  # Path(__file__).absolute().parent.glob('lrf_register*.c')
    configs = {}
    converted = {}
    powerLevels = {}
    powerLevelsHigh = {}
    for c in c_configs:
        filetext = open(c).read()
        if matches := re.finditer(
            r"^const uint32_t LRF_(frontend|main)?[Rr]egConfig_(.*?)\[\] =\n{\n([^}]+)", filetext, re.M
        ):
            for m in matches:
                type, name, config = m.groups()
                rows = [re.match(r"^\s+(0x[0-9a-zA-Z]+),?\s+\/\/\s+(.*)$", cf) for cf in config.split("\n") if len(cf)]
                if any(_ is None for _ in rows):
                    raise ValueError(f"Problem parsing file {c}")
                words = [int(x.group(1), 16) for x in rows if x]
                comments = [x.group(2) for x in rows if x]
                if type == "frontend":
                    configs[name + "_" + type] = Config(name, struct.pack(f"{len(words)}I", *words))
                else:
                    configs[name] = Config(name, struct.pack(f"{len(words)}I", *words))
                converted[name] = [
                    f"{', '.join([hex(_) for _ in struct.unpack('BBBB', struct.pack('I', w))])},  # {c}"
                    for w, c in zip(words, comments)
                ]
        if get_power_levels and (
            matches := re.finditer(
                r"^const TRX_TxPowerLevels TRX_(txPowerLevels|txPowerLevelsHigh)_(.\w+) *= *{\n *\.numEntries *= *0x([0-9A-F]+),\n *\.powerLevels *= *\{([^;]+)",
                filetext,
                re.M,
            )
        ):
            for m in matches:
                type, name, len_hex, contents = m.groups()
                length = int(len_hex, 16)
                rows = [
                    re.match(r"^\s+\{ *\.fraction *= *([01]), *\.dBm *= *([+-]?\d+) *\}.*$", cf)
                    for cf in contents.split("\n")
                    if len(cf) > 6
                ]
                if any(_ is None for _ in rows):
                    raise ValueError(f"Problem parsing file {c}")
                fraction = [int(x.group(1)) for x in rows if x]
                dBm = [int(x.group(2)) for x in rows if x]
                if length != len(dBm):
                    raise ValueError(f"Length mismatch parsing file {c}")
                levels = []
                for fb, db in zip(fraction, dBm):
                    levels.append(int(db) + (int(fb) * 0.5))
                if type == "txPowerLevelsHigh":
                    powerLevelsHigh[name] = levels
                else:
                    powerLevels[name] = levels

    return configs, powerLevels, powerLevelsHigh


def read_config_directory(path: Path) -> Dict[str, Config]:
    configs, _, _ = read_config_directory_with_power_levels(path, get_power_levels=False)
    return configs


class ConfigDatabase:
    def __init__(self, configs_path, target_device):
        self.configs, self.powerLevels, self.powerLevelsHigh = read_config_directory_with_power_levels(configs_path)
        self.target_device = target_device
        # Each PHY option points to the combined PHY that contains it.
        # The HarcodedPhy class can be used to extract the option mask needed
        # for each of the following PHYs.
        self.mapping_tyr = {
            HardcodedPhy.LoraSF11: "sidewalk",
            HardcodedPhy.LoraSF7: "sidewalk",
            HardcodedPhy.LoraSF11_125kbps: "sidewalk",
            HardcodedPhy.LoraSF7_125kbps: "sidewalk",
            HardcodedPhy.OFDMLR: "lrofdm",
            HardcodedPhy.SidewalkFSK50kbps: "sidewalk",
            HardcodedPhy.SidewalkFSK150kbps: "sidewalk",
            HardcodedPhy.SidewalkFSK250kbps: "sidewalk",
            HardcodedPhy.SUNOFDMo2: "wisun",
            HardcodedPhy.SUNOFDMo3MCS0: "wisun",
            HardcodedPhy.SUNOFDMo3MCS16: "wisun",
            HardcodedPhy.SUNOFDMo4MCS01: "wisun",
            HardcodedPhy.SUNOFDMo4MCS26: "wisun",
            HardcodedPhy.SUNFSK2b: "wisun",
            HardcodedPhy.SUNFSK4b: "wisun",
        }

        self.mapping_hugin = {
            HardcodedPhy.SUNFSK1a: "wisun",
            HardcodedPhy.SUNFSK1b: "wisun",
            HardcodedPhy.SUNFSK2a: "wisun",
            HardcodedPhy.SUNFSK2b: "wisun",
            HardcodedPhy.SUNFSK4b: "wisun",
            HardcodedPhy.SUNOFDMo1: "wisun",
            HardcodedPhy.SUNOFDMo2: "wisun",
            HardcodedPhy.SUNOFDMo3MCS0: "wisun",
            HardcodedPhy.SUNOFDMo3MCS16: "wisun",
            HardcodedPhy.SUNOFDMo4MCS01: "wisun",
            HardcodedPhy.SUNOFDMo4MCS26: "wisun",
            HardcodedPhy.OFDMLR: "ofdmlr16p67ksps",
            HardcodedPhy.PROPFSK50kbps: "2gfsk50kbps",
            HardcodedPhy.PROPFSK9p6kbps: "2gfsk9p6kbps",
            HardcodedPhy.PROPFSK38p4kbps: "2gfsk38p4kbps",
            HardcodedPhy.PROPFSK50kbps_LowPower: "2gfsk50kbpsLp",
            HardcodedPhy.PROPFSK50kbps_GeneralPurpose: "generalPurposeGfsk",
            HardcodedPhy.PROPOOK4_8kbps: "propook",
            HardcodedPhy.SUNOQPSK: "sunoqpsk",
            HardcodedPhy.XLR_20Ksps_Dss2: "xlr20ksps",
            HardcodedPhy.XLR_20Ksps_Dss4: "xlr20ksps",
            HardcodedPhy.XLR_20Ksps_Dss8: "xlr20ksps",
            HardcodedPhy.WMBus_Tmode_3of6Encoding: "wmbus",
            HardcodedPhy.WMBus_Smode_ManchesterEncoding: "wmbus",
            HardcodedPhy.Wmbus_Cmode_M2O_FFB: "wmbus",
            HardcodedPhy.Wmbus_Cmode_M2O_FFA: "wmbus",
            HardcodedPhy.Wmbus_Cmode_O2M_FFA: "wmbus",
            HardcodedPhy.Wmbus_Cmode_O2M_FFB: "wmbus",
            HardcodedPhy.DUALRX_SUNOQPSK_PROPFSK38p4kbps: "dualrxSunOqpsk2gfsk38p4kbps",
            HardcodedPhy.DUALRX_SUNOQPSK_PROPFSK50kbps: "dualrxSunOqpsk2gfsk50kbps",
            HardcodedPhy.DUALRX_SUNOFDM_O3_PROPFSK50kbps: "dualrxSunOfdmO32gfsk50kbps",
        }

    def get(self, config: HardcodedPhy):
        if "CC1308" in self.target_device:
            return self.configs[self.mapping_tyr[config]]
        elif "CC1407" in self.target_device:
            return self.configs[self.mapping_hugin[config]]
        else:
            raise Exception("%s is an unsupported device in this test" % (self.target_device))

    def get_power_levels(self, config: HardcodedPhy, high_power: bool = False):
        if "CC1308" in self.target_device:
            # Power table export not supported for Tyr
            return None
        elif "CC1407" in self.target_device:
            if high_power:
                if self.mapping_hugin[config] in self.powerLevelsHigh:
                    return self.powerLevelsHigh[self.mapping_hugin[config]]
                else:
                    return None
            else:
                if self.mapping_hugin[config] in self.powerLevels:
                    return self.powerLevels[self.mapping_hugin[config]]
                else:
                    return None
        else:
            raise Exception("%s is an unsupported device in this test" % (self.target_device))
