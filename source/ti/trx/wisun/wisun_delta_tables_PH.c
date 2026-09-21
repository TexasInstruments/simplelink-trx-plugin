
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


#include "wisun_delta_tables_PH.h"

// DeltaTable for PH, ChannelPlanID 48
// Size in bytes: 16
const uint32_t wisun_delta_table_PH_from_ChannelPlanID48[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00020064,    //  ConfigValue32(value32=131172)
    0x00000000     //  ConfigValue32(value32=0)
};

// DeltaTable for PH, ChannelPlanID 49
// Size in bytes: 12
const uint32_t wisun_delta_table_PH_from_ChannelPlanID49[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_PH(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 48: return wisun_delta_table_PH_from_ChannelPlanID48;
        case 49: return wisun_delta_table_PH_from_ChannelPlanID49;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_PH(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for PH, ChannelPlanID 48
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
                // Invalid channel for PH, ChannelPlanID 48
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for PH, ChannelPlanID 49
        case 49: switch (channel)
            {
                case 0: return 0;
                case 1: return 0;
                case 2: return 0;
                case 3: return 0;
                case 4: return 0;
                case 5: return 0;
                case 6: return 0;
                // Invalid channel for PH, ChannelPlanID 49
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_PH(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 48;
        case 1: return 49;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_PH(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for PH, ChannelPlanID 48
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
                // Invalid frequency for PH, ChannelPlanID 48
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for PH, ChannelPlanID 49
        case 49: switch (frequency)
            {
                case 915400: return 0;
                case 915800: return 1;
                case 916200: return 2;
                case 916600: return 3;
                case 917000: return 4;
                case 917400: return 5;
                case 917800: return 6;
                // Invalid frequency for PH, ChannelPlanID 49
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_PH(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for PH, ChannelPlanID 48
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
                // Invalid channel for PH, ChannelPlanID 48
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for PH, ChannelPlanID 49
        case 49: switch (channel)
            {
                case 0: return 915400;
                case 1: return 915800;
                case 2: return 916200;
                case 3: return 916600;
                case 4: return 917000;
                case 5: return 917400;
                case 6: return 917800;
                // Invalid channel for PH, ChannelPlanID 49
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

