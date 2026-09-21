
/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "wisun_delta_tables_AU_NZ.h"

// DeltaTable for AU_NZ, ChannelPlanID 48
// Size in bytes: 16
const uint32_t wisun_delta_table_AU_NZ_from_ChannelPlanID48[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00020064,    //  ConfigValue32(value32=131172)
    0x00000000     //  ConfigValue32(value32=0)
};

// DeltaTable for AU_NZ, ChannelPlanID 49
// Size in bytes: 12
const uint32_t wisun_delta_table_AU_NZ_from_ChannelPlanID49[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_AU_NZ(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 48: return wisun_delta_table_AU_NZ_from_ChannelPlanID48;
        case 49: return wisun_delta_table_AU_NZ_from_ChannelPlanID49;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_AU_NZ(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for AU_NZ, ChannelPlanID 48
        case 48: switch (channel)
            {
                case 0: return 0;
                case 1: return 2;
                case 2: return 0;
                case 3: return 2;
                case 4: return 0;
                case 5: return 2;
                case 6: return 0;
                case 7: return 2;
                case 8: return 0;
                case 9: return 2;
                case 10: return 0;
                case 11: return 2;
                case 12: return 0;
                case 13: return 2;
                case 14: return 0;
                case 15: return 2;
                case 16: return 0;
                case 17: return 2;
                case 18: return 0;
                case 19: return 2;
                case 20: return 0;
                case 21: return 2;
                case 22: return 0;
                case 23: return 2;
                case 24: return 0;
                case 25: return 2;
                case 26: return 0;
                case 27: return 2;
                case 28: return 0;
                case 29: return 2;
                case 30: return 0;
                case 31: return 2;
                case 32: return 0;
                case 33: return 2;
                case 34: return 0;
                case 35: return 2;
                case 36: return 0;
                case 37: return 2;
                case 38: return 0;
                case 39: return 2;
                case 40: return 0;
                case 41: return 2;
                case 42: return 0;
                case 43: return 2;
                case 44: return 0;
                case 45: return 2;
                case 46: return 0;
                case 47: return 2;
                case 48: return 0;
                case 49: return 2;
                case 50: return 0;
                case 51: return 2;
                case 52: return 0;
                case 53: return 2;
                case 54: return 0;
                case 55: return 2;
                case 56: return 0;
                case 57: return 2;
                case 58: return 0;
                case 59: return 2;
                case 60: return 0;
                case 61: return 2;
                case 62: return 0;
                case 63: return 2;
                // Invalid channel for AU_NZ, ChannelPlanID 48
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for AU_NZ, ChannelPlanID 49
        case 49: switch (channel)
            {
                case 0: return 0;
                case 1: return 0;
                case 2: return 0;
                case 3: return 0;
                case 4: return 0;
                case 5: return 0;
                case 6: return 0;
                case 7: return 0;
                case 8: return 0;
                case 9: return 0;
                case 10: return 0;
                case 11: return 0;
                case 12: return 0;
                case 13: return 0;
                case 14: return 0;
                case 15: return 0;
                case 16: return 0;
                case 17: return 0;
                case 18: return 0;
                case 19: return 0;
                case 20: return 0;
                case 21: return 0;
                case 22: return 0;
                case 23: return 0;
                case 24: return 0;
                case 25: return 0;
                case 26: return 0;
                case 27: return 0;
                case 28: return 0;
                case 29: return 0;
                case 30: return 0;
                case 31: return 0;
                // Invalid channel for AU_NZ, ChannelPlanID 49
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_AU_NZ(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 48;
        case 1: return 49;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_AU_NZ(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for AU_NZ, ChannelPlanID 48
        case 48: switch (frequency)
            {
                case 915200: return 0;
                case 915400: return 1;
                case 915600: return 2;
                case 915800: return 3;
                case 916000: return 4;
                case 916200: return 5;
                case 916400: return 6;
                case 916600: return 7;
                case 916800: return 8;
                case 917000: return 9;
                case 917200: return 10;
                case 917400: return 11;
                case 917600: return 12;
                case 917800: return 13;
                case 918000: return 14;
                case 918200: return 15;
                case 918400: return 16;
                case 918600: return 17;
                case 918800: return 18;
                case 919000: return 19;
                case 919200: return 20;
                case 919400: return 21;
                case 919600: return 22;
                case 919800: return 23;
                case 920000: return 24;
                case 920200: return 25;
                case 920400: return 26;
                case 920600: return 27;
                case 920800: return 28;
                case 921000: return 29;
                case 921200: return 30;
                case 921400: return 31;
                case 921600: return 32;
                case 921800: return 33;
                case 922000: return 34;
                case 922200: return 35;
                case 922400: return 36;
                case 922600: return 37;
                case 922800: return 38;
                case 923000: return 39;
                case 923200: return 40;
                case 923400: return 41;
                case 923600: return 42;
                case 923800: return 43;
                case 924000: return 44;
                case 924200: return 45;
                case 924400: return 46;
                case 924600: return 47;
                case 924800: return 48;
                case 925000: return 49;
                case 925200: return 50;
                case 925400: return 51;
                case 925600: return 52;
                case 925800: return 53;
                case 926000: return 54;
                case 926200: return 55;
                case 926400: return 56;
                case 926600: return 57;
                case 926800: return 58;
                case 927000: return 59;
                case 927200: return 60;
                case 927400: return 61;
                case 927600: return 62;
                case 927800: return 63;
                // Invalid frequency for AU_NZ, ChannelPlanID 48
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for AU_NZ, ChannelPlanID 49
        case 49: switch (frequency)
            {
                case 915400: return 0;
                case 915800: return 1;
                case 916200: return 2;
                case 916600: return 3;
                case 917000: return 4;
                case 917400: return 5;
                case 917800: return 6;
                case 918200: return 7;
                case 918600: return 8;
                case 919000: return 9;
                case 919400: return 10;
                case 919800: return 11;
                case 920200: return 12;
                case 920600: return 13;
                case 921000: return 14;
                case 921400: return 15;
                case 921800: return 16;
                case 922200: return 17;
                case 922600: return 18;
                case 923000: return 19;
                case 923400: return 20;
                case 923800: return 21;
                case 924200: return 22;
                case 924600: return 23;
                case 925000: return 24;
                case 925400: return 25;
                case 925800: return 26;
                case 926200: return 27;
                case 926600: return 28;
                case 927000: return 29;
                case 927400: return 30;
                case 927800: return 31;
                // Invalid frequency for AU_NZ, ChannelPlanID 49
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_AU_NZ(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for AU_NZ, ChannelPlanID 48
        case 48: switch (channel)
            {
                case 0: return 915200;
                case 1: return 915400;
                case 2: return 915600;
                case 3: return 915800;
                case 4: return 916000;
                case 5: return 916200;
                case 6: return 916400;
                case 7: return 916600;
                case 8: return 916800;
                case 9: return 917000;
                case 10: return 917200;
                case 11: return 917400;
                case 12: return 917600;
                case 13: return 917800;
                case 14: return 918000;
                case 15: return 918200;
                case 16: return 918400;
                case 17: return 918600;
                case 18: return 918800;
                case 19: return 919000;
                case 20: return 919200;
                case 21: return 919400;
                case 22: return 919600;
                case 23: return 919800;
                case 24: return 920000;
                case 25: return 920200;
                case 26: return 920400;
                case 27: return 920600;
                case 28: return 920800;
                case 29: return 921000;
                case 30: return 921200;
                case 31: return 921400;
                case 32: return 921600;
                case 33: return 921800;
                case 34: return 922000;
                case 35: return 922200;
                case 36: return 922400;
                case 37: return 922600;
                case 38: return 922800;
                case 39: return 923000;
                case 40: return 923200;
                case 41: return 923400;
                case 42: return 923600;
                case 43: return 923800;
                case 44: return 924000;
                case 45: return 924200;
                case 46: return 924400;
                case 47: return 924600;
                case 48: return 924800;
                case 49: return 925000;
                case 50: return 925200;
                case 51: return 925400;
                case 52: return 925600;
                case 53: return 925800;
                case 54: return 926000;
                case 55: return 926200;
                case 56: return 926400;
                case 57: return 926600;
                case 58: return 926800;
                case 59: return 927000;
                case 60: return 927200;
                case 61: return 927400;
                case 62: return 927600;
                case 63: return 927800;
                // Invalid channel for AU_NZ, ChannelPlanID 48
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for AU_NZ, ChannelPlanID 49
        case 49: switch (channel)
            {
                case 0: return 915400;
                case 1: return 915800;
                case 2: return 916200;
                case 3: return 916600;
                case 4: return 917000;
                case 5: return 917400;
                case 6: return 917800;
                case 7: return 918200;
                case 8: return 918600;
                case 9: return 919000;
                case 10: return 919400;
                case 11: return 919800;
                case 12: return 920200;
                case 13: return 920600;
                case 14: return 921000;
                case 15: return 921400;
                case 16: return 921800;
                case 17: return 922200;
                case 18: return 922600;
                case 19: return 923000;
                case 20: return 923400;
                case 21: return 923800;
                case 22: return 924200;
                case 23: return 924600;
                case 24: return 925000;
                case 25: return 925400;
                case 26: return 925800;
                case 27: return 926200;
                case 28: return 926600;
                case 29: return 927000;
                case 30: return 927400;
                case 31: return 927800;
                // Invalid channel for AU_NZ, ChannelPlanID 49
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

