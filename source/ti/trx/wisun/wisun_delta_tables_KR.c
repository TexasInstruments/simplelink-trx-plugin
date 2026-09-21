
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


#include "wisun_delta_tables_KR.h"

// DeltaTable for KR, ChannelPlanID 96
// Size in bytes: 16
const uint32_t wisun_delta_table_KR_from_ChannelPlanID96[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00020064,    //  ConfigValue32(value32=131172)
    0x00000000     //  ConfigValue32(value32=0)
};

// DeltaTable for KR, ChannelPlanID 97
// Size in bytes: 12
const uint32_t wisun_delta_table_KR_from_ChannelPlanID97[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_KR(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 96: return wisun_delta_table_KR_from_ChannelPlanID96;
        case 97: return wisun_delta_table_KR_from_ChannelPlanID97;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_KR(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for KR, ChannelPlanID 96
        case 96: switch (channel)
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
                // Invalid channel for KR, ChannelPlanID 96
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for KR, ChannelPlanID 97
        case 97: switch (channel)
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
                // Invalid channel for KR, ChannelPlanID 97
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_KR(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 96;
        case 1: return 97;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_KR(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for KR, ChannelPlanID 96
        case 96: switch (frequency)
            {
                case 917100: return 0;
                case 917300: return 1;
                case 917500: return 2;
                case 917700: return 3;
                case 917900: return 4;
                case 918100: return 5;
                case 918300: return 6;
                case 918500: return 7;
                case 918700: return 8;
                case 918900: return 9;
                case 919100: return 10;
                case 919300: return 11;
                case 919500: return 12;
                case 919700: return 13;
                case 919900: return 14;
                case 920100: return 15;
                case 920300: return 16;
                case 920500: return 17;
                case 920700: return 18;
                case 920900: return 19;
                case 921100: return 20;
                case 921300: return 21;
                case 921500: return 22;
                case 921700: return 23;
                case 921900: return 24;
                case 922100: return 25;
                case 922300: return 26;
                case 922500: return 27;
                case 922700: return 28;
                case 922900: return 29;
                case 923100: return 30;
                case 923300: return 31;
                // Invalid frequency for KR, ChannelPlanID 96
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for KR, ChannelPlanID 97
        case 97: switch (frequency)
            {
                case 917300: return 0;
                case 917700: return 1;
                case 918100: return 2;
                case 918500: return 3;
                case 918900: return 4;
                case 919300: return 5;
                case 919700: return 6;
                case 920100: return 7;
                case 920500: return 8;
                case 920900: return 9;
                case 921300: return 10;
                case 921700: return 11;
                case 922100: return 12;
                case 922500: return 13;
                case 922900: return 14;
                case 923300: return 15;
                // Invalid frequency for KR, ChannelPlanID 97
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_KR(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for KR, ChannelPlanID 96
        case 96: switch (channel)
            {
                case 0: return 917100;
                case 1: return 917300;
                case 2: return 917500;
                case 3: return 917700;
                case 4: return 917900;
                case 5: return 918100;
                case 6: return 918300;
                case 7: return 918500;
                case 8: return 918700;
                case 9: return 918900;
                case 10: return 919100;
                case 11: return 919300;
                case 12: return 919500;
                case 13: return 919700;
                case 14: return 919900;
                case 15: return 920100;
                case 16: return 920300;
                case 17: return 920500;
                case 18: return 920700;
                case 19: return 920900;
                case 20: return 921100;
                case 21: return 921300;
                case 22: return 921500;
                case 23: return 921700;
                case 24: return 921900;
                case 25: return 922100;
                case 26: return 922300;
                case 27: return 922500;
                case 28: return 922700;
                case 29: return 922900;
                case 30: return 923100;
                case 31: return 923300;
                // Invalid channel for KR, ChannelPlanID 96
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for KR, ChannelPlanID 97
        case 97: switch (channel)
            {
                case 0: return 917300;
                case 1: return 917700;
                case 2: return 918100;
                case 3: return 918500;
                case 4: return 918900;
                case 5: return 919300;
                case 6: return 919700;
                case 7: return 920100;
                case 8: return 920500;
                case 9: return 920900;
                case 10: return 921300;
                case 11: return 921700;
                case 12: return 922100;
                case 13: return 922500;
                case 14: return 922900;
                case 15: return 923300;
                // Invalid channel for KR, ChannelPlanID 97
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

