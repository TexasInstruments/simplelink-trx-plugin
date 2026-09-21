
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


#include "wisun_delta_tables_SG1.h"

// DeltaTable for SG1, ChannelPlanID 32
// Size in bytes: 24
const uint32_t wisun_delta_table_SG1_from_ChannelPlanID32[] =
{
    0x00000005,    //  ConfigSegment(length=5, feature_mask=0, inverted=False, compound=False)
    0x00309003,    //  ConfigRegion(length=4, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x02000064,    //  ConfigValue32(value32=33554532)
    0x00010100,    //  ConfigValue32(value32=65792)
    0x01000000,    //  ConfigValue32(value32=16777216)
    0xfe0000ff     //  ConfigValue32(value32=4261413119)
};

// DeltaTable for SG1, ChannelPlanID 33
// Size in bytes: 20
const uint32_t wisun_delta_table_SG1_from_ChannelPlanID33[] =
{
    0x00000004,    //  ConfigSegment(length=4, feature_mask=0, inverted=False, compound=False)
    0x00309002,    //  ConfigRegion(length=3, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x02000064,    //  ConfigValue32(value32=33554532)
    0x00000000,    //  ConfigValue32(value32=0)
    0x0000fe00     //  ConfigValue32(value32=65024)
};

// DeltaTable for SG1, ChannelPlanID 38
// Size in bytes: 12
const uint32_t wisun_delta_table_SG1_from_ChannelPlanID38[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_SG1(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 32: return wisun_delta_table_SG1_from_ChannelPlanID32;
        case 33: return wisun_delta_table_SG1_from_ChannelPlanID33;
        case 38: return wisun_delta_table_SG1_from_ChannelPlanID38;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_SG1(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for SG1, ChannelPlanID 32
        case 32: switch (channel)
            {
                case 30: return 0;
                case 31: return 3;
                case 32: return 6;
                case 33: return 9;
                case 34: return 0;
                case 35: return 3;
                case 36: return 6;
                case 37: return 9;
                case 38: return 0;
                case 39: return 3;
                case 40: return 6;
                case 41: return 9;
                case 42: return 0;
                case 43: return 3;
                case 44: return 6;
                case 45: return 9;
                case 46: return 0;
                case 47: return 3;
                case 48: return 6;
                case 49: return 9;
                case 50: return 0;
                case 51: return 3;
                case 52: return 6;
                case 53: return 9;
                case 54: return 0;
                case 55: return 3;
                case 56: return 6;
                case 57: return 9;
                case 58: return 12;
                // Invalid channel for SG1, ChannelPlanID 32
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for SG1, ChannelPlanID 33
        case 33: switch (channel)
            {
                case 15: return 0;
                case 16: return 3;
                case 17: return 0;
                case 18: return 3;
                case 19: return 0;
                case 20: return 3;
                case 21: return 0;
                case 22: return 3;
                case 23: return 0;
                case 24: return 3;
                case 25: return 0;
                case 26: return 3;
                case 27: return 0;
                case 28: return 3;
                case 29: return 6;
                // Invalid channel for SG1, ChannelPlanID 33
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for SG1, ChannelPlanID 38
        case 38: switch (channel)
            {
                case 7: return 0;
                case 8: return 0;
                case 9: return 0;
                case 10: return 0;
                case 11: return 0;
                case 12: return 0;
                case 13: return 0;
                // Invalid channel for SG1, ChannelPlanID 38
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_SG1(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 32;
        case 1: return 33;
        case 2: return 38;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_SG1(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for SG1, ChannelPlanID 32
        case 32: switch (frequency)
            {
                case 866100: return 30;
                case 866200: return 31;
                case 866300: return 32;
                case 866400: return 33;
                case 866500: return 34;
                case 866600: return 35;
                case 866700: return 36;
                case 866800: return 37;
                case 866900: return 38;
                case 867000: return 39;
                case 867100: return 40;
                case 867200: return 41;
                case 867300: return 42;
                case 867400: return 43;
                case 867500: return 44;
                case 867600: return 45;
                case 867700: return 46;
                case 867800: return 47;
                case 867900: return 48;
                case 868000: return 49;
                case 868100: return 50;
                case 868200: return 51;
                case 868300: return 52;
                case 868400: return 53;
                case 868500: return 54;
                case 868600: return 55;
                case 868700: return 56;
                case 868800: return 57;
                case 868900: return 58;
                // Invalid frequency for SG1, ChannelPlanID 32
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for SG1, ChannelPlanID 33
        case 33: switch (frequency)
            {
                case 866100: return 15;
                case 866300: return 16;
                case 866500: return 17;
                case 866700: return 18;
                case 866900: return 19;
                case 867100: return 20;
                case 867300: return 21;
                case 867500: return 22;
                case 867700: return 23;
                case 867900: return 24;
                case 868100: return 25;
                case 868300: return 26;
                case 868500: return 27;
                case 868700: return 28;
                case 868900: return 29;
                // Invalid frequency for SG1, ChannelPlanID 33
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for SG1, ChannelPlanID 38
        case 38: switch (frequency)
            {
                case 866300: return 7;
                case 866700: return 8;
                case 867100: return 9;
                case 867500: return 10;
                case 867900: return 11;
                case 868300: return 12;
                case 868700: return 13;
                // Invalid frequency for SG1, ChannelPlanID 38
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_SG1(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for SG1, ChannelPlanID 32
        case 32: switch (channel)
            {
                case 30: return 866100;
                case 31: return 866200;
                case 32: return 866300;
                case 33: return 866400;
                case 34: return 866500;
                case 35: return 866600;
                case 36: return 866700;
                case 37: return 866800;
                case 38: return 866900;
                case 39: return 867000;
                case 40: return 867100;
                case 41: return 867200;
                case 42: return 867300;
                case 43: return 867400;
                case 44: return 867500;
                case 45: return 867600;
                case 46: return 867700;
                case 47: return 867800;
                case 48: return 867900;
                case 49: return 868000;
                case 50: return 868100;
                case 51: return 868200;
                case 52: return 868300;
                case 53: return 868400;
                case 54: return 868500;
                case 55: return 868600;
                case 56: return 868700;
                case 57: return 868800;
                case 58: return 868900;
                // Invalid channel for SG1, ChannelPlanID 32
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for SG1, ChannelPlanID 33
        case 33: switch (channel)
            {
                case 15: return 866100;
                case 16: return 866300;
                case 17: return 866500;
                case 18: return 866700;
                case 19: return 866900;
                case 20: return 867100;
                case 21: return 867300;
                case 22: return 867500;
                case 23: return 867700;
                case 24: return 867900;
                case 25: return 868100;
                case 26: return 868300;
                case 27: return 868500;
                case 28: return 868700;
                case 29: return 868900;
                // Invalid channel for SG1, ChannelPlanID 33
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for SG1, ChannelPlanID 38
        case 38: switch (channel)
            {
                case 7: return 866300;
                case 8: return 866700;
                case 9: return 867100;
                case 10: return 867500;
                case 11: return 867900;
                case 12: return 868300;
                case 13: return 868700;
                // Invalid channel for SG1, ChannelPlanID 38
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

