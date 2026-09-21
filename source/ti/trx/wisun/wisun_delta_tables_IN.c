
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


#include "wisun_delta_tables_IN.h"

// DeltaTable for IN, ChannelPlanID 32
// Size in bytes: 16
const uint32_t wisun_delta_table_IN_from_ChannelPlanID32[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064,    //  ConfigValue32(value32=100)
    0x00000001     //  ConfigValue32(value32=1)
};

// DeltaTable for IN, ChannelPlanID 33
// Size in bytes: 12
const uint32_t wisun_delta_table_IN_from_ChannelPlanID33[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_IN(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 32: return wisun_delta_table_IN_from_ChannelPlanID32;
        case 33: return wisun_delta_table_IN_from_ChannelPlanID33;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_IN(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for IN, ChannelPlanID 32
        case 32: switch (channel)
            {
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
                // Invalid channel for IN, ChannelPlanID 32
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for IN, ChannelPlanID 33
        case 33: switch (channel)
            {
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
                // Invalid channel for IN, ChannelPlanID 33
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_IN(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 32;
        case 1: return 33;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_IN(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for IN, ChannelPlanID 32
        case 32: switch (frequency)
            {
                case 865100: return 20;
                case 865200: return 21;
                case 865300: return 22;
                case 865400: return 23;
                case 865500: return 24;
                case 865600: return 25;
                case 865700: return 26;
                case 865800: return 27;
                case 865900: return 28;
                case 866000: return 29;
                case 866100: return 30;
                case 866200: return 31;
                case 866300: return 32;
                case 866400: return 33;
                case 866500: return 34;
                case 866600: return 35;
                case 866700: return 36;
                case 866800: return 37;
                case 866900: return 38;
                // Invalid frequency for IN, ChannelPlanID 32
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for IN, ChannelPlanID 33
        case 33: switch (frequency)
            {
                case 865100: return 10;
                case 865300: return 11;
                case 865500: return 12;
                case 865700: return 13;
                case 865900: return 14;
                case 866100: return 15;
                case 866300: return 16;
                case 866500: return 17;
                case 866700: return 18;
                case 866900: return 19;
                // Invalid frequency for IN, ChannelPlanID 33
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_IN(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for IN, ChannelPlanID 32
        case 32: switch (channel)
            {
                case 20: return 865100;
                case 21: return 865200;
                case 22: return 865300;
                case 23: return 865400;
                case 24: return 865500;
                case 25: return 865600;
                case 26: return 865700;
                case 27: return 865800;
                case 28: return 865900;
                case 29: return 866000;
                case 30: return 866100;
                case 31: return 866200;
                case 32: return 866300;
                case 33: return 866400;
                case 34: return 866500;
                case 35: return 866600;
                case 36: return 866700;
                case 37: return 866800;
                case 38: return 866900;
                // Invalid channel for IN, ChannelPlanID 32
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for IN, ChannelPlanID 33
        case 33: switch (channel)
            {
                case 10: return 865100;
                case 11: return 865300;
                case 12: return 865500;
                case 13: return 865700;
                case 14: return 865900;
                case 15: return 866100;
                case 16: return 866300;
                case 17: return 866500;
                case 18: return 866700;
                case 19: return 866900;
                // Invalid channel for IN, ChannelPlanID 33
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

