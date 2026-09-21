from __future__ import annotations
from dataclasses import InitVar, dataclass, field
import enum
from math import ceil
from multiprocessing.sharedctypes import Value
from typing import Dict, List
import pandas as pd
from trx.trxrequests import ConfigList, ConfigRegion, ConfigRegionOperation, ConfigSegment, ConfigValue32

######################################################################################
# TRX CONSTANTS
######################################################################################

TRX_SWPARAM_DELTA_FREQUENCY_TABLE_ADDR_CC1308 = 2 * 4
TRX_SWPARAM_DELTA_FREQUENCY_TABLE_ADDR_CC1407 = 12 * 4
TRX_DELTA_FREQUENCY_RESOLUTION = 100  # fixed for now, could be made dynamic

######################################################################################
# SPEC VALUES
######################################################################################


class PhyModeId(enum.IntEnum):
    # FSK NO FEC
    FSK1A = 0x01
    FSK1B = 0x02
    FSK2A = 0x03
    FSK2B = 0x04
    FSK3 = 0x05
    FSK4A = 0x06
    FSK4B = 0x07
    FSK5 = 0x08
    # FSK with NRNSC FEC
    FECFSK1A = 0x11
    FECFSK1B = 0x12
    FECFSK2A = 0x13
    FECFSK2B = 0x14
    FECFSK3 = 0x15
    FECFSK4A = 0x16
    FECFSK4B = 0x17
    FECFSK5 = 0x18
    # OFDM Option 1
    OFDMO1M0 = 0x20
    OFDMO1M1 = 0x21
    OFDMO1M2 = 0x22
    OFDMO1M3 = 0x23
    OFDMO1M4 = 0x24
    OFDMO1M5 = 0x25
    OFDMO1M6 = 0x26
    # OFDM Option 2
    OFDMO2M0 = 0x30
    OFDMO2M1 = 0x31
    OFDMO2M2 = 0x32
    OFDMO2M3 = 0x33
    OFDMO2M4 = 0x34
    OFDMO2M5 = 0x35
    OFDMO2M6 = 0x36
    # OFDM Option 3
    OFDMO3M0 = 0x40
    OFDMO3M1 = 0x41
    OFDMO3M2 = 0x42
    OFDMO3M3 = 0x43
    OFDMO3M4 = 0x44
    OFDMO3M5 = 0x45
    OFDMO3M6 = 0x46
    # OFDM Option 4
    OFDMO4M0 = 0x50
    OFDMO4M1 = 0x51
    OFDMO4M2 = 0x52
    OFDMO4M3 = 0x53
    OFDMO4M4 = 0x54
    OFDMO4M5 = 0x55
    OFDMO4M6 = 0x56


all_fsk1a = [PhyModeId.FSK1A, PhyModeId.FECFSK1A]
all_fsk1b = [PhyModeId.FSK1B, PhyModeId.FECFSK1B]
all_fsk2a = [PhyModeId.FSK2A, PhyModeId.FECFSK2A]
all_fsk2b = [PhyModeId.FSK2B, PhyModeId.FECFSK2B]
all_fsk3 = [PhyModeId.FSK3, PhyModeId.FECFSK3]
all_fsk4a = [PhyModeId.FSK4A, PhyModeId.FECFSK4A]
all_fsk4b = [PhyModeId.FSK4B, PhyModeId.FECFSK4B]
all_fsk5 = [PhyModeId.FSK5, PhyModeId.FECFSK5]
base_phys = all_fsk1a + all_fsk1b + all_fsk2a + all_fsk2b + all_fsk3 + all_fsk4a + all_fsk4b + all_fsk5

ofdmo1m2to6 = [PhyModeId.OFDMO1M2, PhyModeId.OFDMO1M3, PhyModeId.OFDMO1M4, PhyModeId.OFDMO1M5, PhyModeId.OFDMO1M6]
ofdmo2m3to6 = [PhyModeId.OFDMO2M3, PhyModeId.OFDMO2M4, PhyModeId.OFDMO2M5, PhyModeId.OFDMO2M6]
ofdmo3m4to6 = [PhyModeId.OFDMO3M4, PhyModeId.OFDMO3M5, PhyModeId.OFDMO3M6]
ofdmo4m4to6 = [PhyModeId.OFDMO4M4, PhyModeId.OFDMO4M5, PhyModeId.OFDMO4M6]

# mapping of ChannelPlanId to spacing and center_freq0
channel_plan_id_params = {
    # NA / BZ / MX
    1: (0.2, 902.2),
    2: (0.4, 902.4),
    3: (0.6, 902.6),
    4: (0.8, 902.8),
    5: (1.2, 903.2),
    # Japan
    21: (0.2, 920.6),
    22: (0.4, 920.9),
    23: (0.6, 920.8),
    24: (0.8, 921.1),
    # EU / IN / SG
    32: (0.1, 863.1),
    33: (0.2, 863.1),
    34: (0.1, 870.1),
    35: (0.2, 870.2),
    36: (0.1, 863.1),
    37: (0.2, 863.1),
    38: (0.4, 863.5),
    # AU / NZ / PH
    48: (0.2, 915.2),
    49: (0.4, 915.4),
    # SG / HK / TH / VN
    64: (0.2, 920.2),
    65: (0.4, 920.4),
    # MY
    80: (0.2, 919.2),
    81: (0.4, 919.2),
    # KR
    96: (0.2, 917.1),
    97: (0.4, 917.3),
    # WW
    112: (0.4, 2400.2),
    113: (0.2, 2400.4),
    # CN
    128: (0.25, 920.625),
    # CN
    144: (0.2, 779.2),
    145: (0.4, 779.4),
    # CN
    160: (0.2, 470.2),
}

# mapping of Regulatory domains to ChannelPlanID, ValidTotalNumChan, ChanMask, PHYs
regulatory_domains = {
    "EU1": [
        (32, 62, "00:00:00:00:00:00:80:E1:E6", all_fsk1a),
        (33, 29, "00:00:00:D8:FB", all_fsk2a + all_fsk3 + ofdmo4m4to6),
    ],
    "EU2": [
        (34, 55, "00:00:00:00:00:00:80", all_fsk1a),
        (35, 27, "00:00:00:F8", all_fsk2a + all_fsk3 + ofdmo4m4to6),
    ],
    "EU3": [
        (36, 118, "00:00:00:00:00:00:80:E1:06:00:00:00:00:00:00:E0", all_fsk1a),
        (37, 56, "00:00:00:D8:03:00:00:C0", all_fsk2a + all_fsk3 + ofdmo4m4to6),
    ],
    "NA": [
        (1, 129, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:FE", all_fsk1b + all_fsk2a + ofdmo4m4to6),
        (2, 64, "00:00:00:00:00:00:00:00", all_fsk3 + all_fsk4a + ofdmo3m4to6),
        (3, 42, "00:00:00:00:00:FC", all_fsk5),
        (4, 32, "00:00:00:00", ofdmo2m3to6),
        (5, 21, "00:00:E0", ofdmo1m2to6),
    ],
    "BZ": [
        (1, 90, "00:00:00:FC:FF:FF:FF:FF:01:00:00:00:00:00:00:00:FE", all_fsk1b + all_fsk2a + ofdmo4m4to6),
        (2, 43, "00:F0:FF:FF:01:00:00:00", all_fsk3 + all_fsk4a + ofdmo3m4to6),
        (3, 28, "00:FF:3F:00:00:FC", all_fsk5),
        (4, 22, "C0:FF:00:00", ofdmo2m3to6),
        (5, 13, "F8:07:E0", ofdmo1m2to6),
    ],
    "JP": [
        (21, 29, "FF:01:00:00:C0", all_fsk1b + ofdmo4m4to6),
        (22, 14, "0F:00:FC", all_fsk2b + all_fsk3 + ofdmo3m4to6),
        (23, 9, "07:F0", all_fsk4b + all_fsk5),
        (24, 7, "03:FE", ofdmo2m3to6),
    ],
    "CN1": [
        (
            160,
            199,
            "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:80",
            all_fsk1b + all_fsk2a + all_fsk3,
        ),
    ],
    "CN2": [
        (144, 39, "00:00:00:00:80", all_fsk1b + all_fsk2a),
        (145, 19, "00:00:F8", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    "CN3": [
        (128, 16, "00:00", all_fsk1b + all_fsk2a + all_fsk3),
    ],
    "IN": [
        (32, 19, "FF:FF:0F:00:80:FF:FF:FF:FF", all_fsk1a),
        (33, 10, "FF:03:F0:FF:FF", all_fsk2a, all_fsk3),
    ],
    "SG1": [
        (32, 29, "FF:FF:FF:3F:00:00:00:F8:FF", all_fsk1a),
        (33, 15, "FF:7F:00:C0:FF", all_fsk2a + all_fsk3),
        (38, 7, "7F:C0:FF:FF", all_fsk4a + all_fsk5),
    ],
    "SG2": [
        (64, 24, "00:00:00", all_fsk1b + all_fsk2a),
        (65, 12, "00:F0", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    "MX": [
        (1, 129, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:FE", all_fsk1b + all_fsk2a),
        (2, 64, "00:00:00:00:00:00:00:00", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    "AU_NZ": [
        (48, 64, "00:00:00:00:00:00:00:00", all_fsk1b + all_fsk2a),
        (49, 32, "00:00:00:00", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    "KR": [
        (96, 32, "00:00:00:00", all_fsk1b + all_fsk2a),
        (97, 16, "00:00", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    "PH": [
        (48, 14, "00:C0:FF:FF:FF:FF:FF:FF", all_fsk1b + all_fsk2a),
        (49, 7, "80:FF:FF:FF", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    "MY": [
        (80, 19, "00:00:F8", all_fsk1b + all_fsk2a),
        (81, 10, "00:FC", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    "HK_TH_VN": [
        (64, 24, "00:00:00", all_fsk1b + all_fsk2a),
        (65, 12, "00:F0", all_fsk3 + all_fsk4a + all_fsk5),
    ],
    # This region uses 2.4GHz, so it won't be supported for now.
    # In addition, there seems to be a bug in the spec here (wrong channel spacing?)
    # There are many more channels in channelPlanId 112 which has a wider spacing.
    # So the frequency range of 112 is much larger than 113, resulting in delta frequencies of 10s of MHz.
    # "WW": [
    #    (112, 416, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00", all_fsk1b + all_fsk2a),
    #    (113, 207, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:80", all_fsk3 + all_fsk4a + all_fsk5),
    # ],
}

######################################################################################
# LOGIC
######################################################################################

# Fundamental ChannelPlan
# Maps an ID to spacing and channel 0 center frequency (according to spec)
# Calculates frequency for a given channel
@dataclass
class ChannelPlan:
    id: int
    spacing: float = field(init=False)
    chan_center_freq0: float = field(init=False)

    def __post_init__(self):
        if self.id in channel_plan_id_params:
            params = channel_plan_id_params[self.id]
            self.spacing = params[0]
            self.chan_center_freq0 = params[1]
        else:
            raise ValueError("Unsupported ChannelPlanID!")

    def get_channel_frequency(self, channel):
        return round(self.chan_center_freq0 + channel * self.spacing, 2)


# Single DeltaTable
# Maps a single channel/frequency in a (base) ChannelPlan
# to a dict newPhy-ChannelPlan -> DeltaFrequency
@dataclass
class DeltaTable:
    channel: int
    frequency: float
    deltas: Dict[int, float]

    def as_bytes(self, resolution):
        bytelist = b"".join(
            [(round(x * 1000 / resolution)).to_bytes(1, "little", signed=True) for x in self.deltas.values()]
        )
        return bytelist

    def get_min(self):
        return min(self.deltas.values())

    def get_max(self):
        return max(self.deltas.values())


# Multiple DeltaTables
# Should be used to combine all DeltaTables for a given (base) ChannelPlan
@dataclass
class DeltaTables:
    tables: List[DeltaTable]
    baseindex: List[int] = field(init=False)

    def __post_init__(self):
        # Calculate baseIndex, i.e. for a concatenated list of all unique
        # DeltaTables, the index that points to the current table
        self.baseindex = [0] * len(self.tables)
        nextidx = len(self.tables[0].deltas)
        for cur in range(1, len(self.tables)):
            for prev in range(cur):
                if self.tables[cur].deltas == self.tables[prev].deltas:
                    self.baseindex[cur] = self.baseindex[prev]
                    break
            else:
                self.baseindex[cur] = nextidx
                nextidx += len(self.tables[cur].deltas)

    def get_baseindex_for_channel(self, channel):
        for idx, t in enumerate(self.tables):
            if t.channel == channel:
                return self.baseindex[idx]
        raise ValueError(f"No baseindex found for channel {channel}")

    def get_baseindex_dict(self):
        ret = {}
        for idx, t in enumerate(self.tables):
            ret[t.channel] = self.baseindex[idx]
        return ret

    def get_baseindex_for_frequency(self, frequency):
        for idx, t in enumerate(self.tables):
            if t.frequency == frequency:
                return self.baseindex[idx]
        raise ValueError(f"No featuremask found for frequency {frequency}")

    def as_config_list(self, device):
        if "CC1308" in device:
            TRX_SWPARAM_DELTA_FREQUENCY_TABLE_ADDR = TRX_SWPARAM_DELTA_FREQUENCY_TABLE_ADDR_CC1308
        elif "CC1407" in device:
            TRX_SWPARAM_DELTA_FREQUENCY_TABLE_ADDR = TRX_SWPARAM_DELTA_FREQUENCY_TABLE_ADDR_CC1407
        else:
            raise ValueError(f"Unsupported device {device}")

        # first entry is the resolution
        bytelist = TRX_DELTA_FREQUENCY_RESOLUTION.to_bytes(1, "little")
        seen_baseindex = []
        for idx, t in enumerate(self.tables):
            if self.baseindex[idx] not in seen_baseindex:
                seen_baseindex.append(self.baseindex[idx])
                bytelist += t.as_bytes(TRX_DELTA_FREQUENCY_RESOLUTION)
        confvals = ConfigValue32.list_from_bytes(bytelist)
        len32 = len(confvals)
        return ConfigList(
            [
                ConfigSegment(len32 + 1),
                ConfigRegion(
                    len32, ConfigRegionOperation.Par_Reference_32bit, address=TRX_SWPARAM_DELTA_FREQUENCY_TABLE_ADDR
                ),
            ]
            + confvals
        )

    def as_dataframe(self):
        all_channel_plan_ids = [x.deltas.keys() for x in self.tables]
        all_channel_plan_ids = sorted(list(set().union(*all_channel_plan_ids)))
        retdict: dict[str, list[float]] = {
            str(col): [0.0] * len(self.tables) for col in ["Channel", "Frequency", "BaseIndex"] + all_channel_plan_ids
        }
        for idx, t in enumerate(self.tables):
            retdict["Channel"][idx] = t.channel
            retdict["Frequency"][idx] = t.frequency
            retdict["BaseIndex"][idx] = self.baseindex[idx]
            for cpid in t.deltas.keys():
                retdict[str(cpid)][idx] = t.deltas[cpid]
        return pd.DataFrame(retdict)

    def get_min(self):
        return min([x.get_min() for x in self.tables])

    def get_max(self):
        return max([x.get_max() for x in self.tables])


# Regulatory ChannelPlan
# adds channel mask, number of channels and list of PHYs
@dataclass
class RegulatoryChannelPlan(ChannelPlan):
    num_total_channels: int = field(init=False)
    num_valid_channels: int
    chan_mask: int = field(init=False)
    chan_mask_str: InitVar[str]
    phys: List[PhyModeId]

    def __post_init__(self, chan_mask_str):
        super().__post_init__()
        self.num_total_channels = 0
        msk = 0
        for idx, bytestr in enumerate(chan_mask_str.split(":")):
            self.num_total_channels += 8
            msk = msk + (int(bytestr, 16) << (idx * 8))
        self.chan_mask = msk

    def get_valid_channels(self):
        return [x for x in range(self.num_total_channels) if self.chan_mask & (1 << x) == 0]

    def get_channel_frequency(self, channel):
        if channel not in self.get_valid_channels():
            raise ValueError("Forbidden channel")
        return super().get_channel_frequency(channel)

    def get_frequencies(self) -> list:
        return [self.get_channel_frequency(x) for x in self.get_valid_channels()]

    def get_closest_frequency(self, freq):
        # adding 10kHz to take the higher frequency if in the middle
        return min(self.get_frequencies(), key=lambda x: abs(x - (freq + 0.01)))

    def used_by_base_phy(self):
        return any([x in base_phys for x in self.phys])

    def get_delta_table_for_channel(self, others: List[RegulatoryChannelPlan], channel) -> DeltaTable:
        freq = self.get_channel_frequency(channel)
        deltas = {}
        for rcp in others:
            deltas[rcp.id] = round(rcp.get_closest_frequency(freq) - freq, 2)
        return DeltaTable(channel, freq, deltas)

    def get_delta_tables(self, others: List[RegulatoryChannelPlan]) -> DeltaTables:
        return DeltaTables([self.get_delta_table_for_channel(others, c) for c in self.get_valid_channels()])


# A generic Region consists of several RegulatoryChannelPlans
# Provides functions to compute different tables
# Use child-class RegulatoryDomain to get one of the predefined Regions from the spec
@dataclass
class Region:
    name: str
    regulatory_channel_plans: List[RegulatoryChannelPlan] = field(default_factory=list)

    def add_plan(self, rcp: RegulatoryChannelPlan):
        self.regulatory_channel_plans.append(rcp)

    def get_rcp_for_phy(self, phy: PhyModeId):
        for rcp in self.regulatory_channel_plans:
            if phy in rcp.phys:
                return rcp
        raise ValueError(f"No RegulatoryChannelPlan supports {phy}!")

    def get_channel_frequency_for_phy(self, phy: PhyModeId, channel):
        rcp = self.get_rcp_for_phy(phy)
        return rcp.get_channel_frequency(channel)

    def get_closest_frequency_for_phy(self, phy: PhyModeId, frequency):
        rcp = self.get_rcp_for_phy(phy)
        return rcp.get_closest_frequency(frequency)

    def get_delta_table_for_phy_channel(self, phy: PhyModeId, channel):
        rcp = self.get_rcp_for_phy(phy)
        return rcp.get_delta_table_for_channel(self.regulatory_channel_plans, channel)

    def get_delta_tables_for_phy(self, phy: PhyModeId):
        rcp = self.get_rcp_for_phy(phy)
        return rcp.get_delta_tables(self.regulatory_channel_plans)

    # returns all delta_tables as a dict basePhy-ChannelPlanId -> DeltaTables
    def get_delta_tables(self):
        ret = {}
        for base in self.regulatory_channel_plans:
            if base.used_by_base_phy() == False:
                continue
            ret[base.id] = base.get_delta_tables(self.regulatory_channel_plans)
        return ret

    # Get all possible frequencies (all RCPs) for this Region,
    # in a DataFrame table ordered by frequency
    def get_frequency_table_as_dataframe(self):
        frequencies = [rcp.get_frequencies() for rcp in self.regulatory_channel_plans]
        allfreqs = sorted(list(set().union(*frequencies)))
        freqtable = [[""] * len(allfreqs) for numplans in range(len(self.regulatory_channel_plans))]
        for row, f in enumerate(allfreqs):
            for col in range(len(self.regulatory_channel_plans)):
                if f in frequencies[col]:
                    freqtable[col][row] = f
        freqdict = {
            f"ChannelPlanID {self.regulatory_channel_plans[idx].id}": freqtable[idx]
            for idx in range(len(self.regulatory_channel_plans))
        }
        return pd.DataFrame(freqdict)


# Predefined Regions in the spec
class RegulatoryDomain(Region):
    def __init__(self, name):
        Region.__init__(self, name)
        if self.name in regulatory_domains:
            for params in regulatory_domains[self.name]:
                self.add_plan(RegulatoryChannelPlan(params[0], params[1], params[2], params[3]))
        else:
            raise ValueError(f"Unsupported Region: {self.name}!")

    @staticmethod
    def get_regions():
        return regulatory_domains.keys()
