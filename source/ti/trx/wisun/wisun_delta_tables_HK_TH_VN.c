
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


#include "wisun_delta_tables_HK_TH_VN.h"

// DeltaTable for HK_TH_VN, ChannelPlanID 64
// Size in bytes: 16
const uint32_t wisun_delta_table_HK_TH_VN_from_ChannelPlanID64[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00020064,    //  ConfigValue32(value32=131172)
    0x00000000     //  ConfigValue32(value32=0)
};

// DeltaTable for HK_TH_VN, ChannelPlanID 65
// Size in bytes: 12
const uint32_t wisun_delta_table_HK_TH_VN_from_ChannelPlanID65[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_HK_TH_VN(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 64: return wisun_delta_table_HK_TH_VN_from_ChannelPlanID64;
        case 65: return wisun_delta_table_HK_TH_VN_from_ChannelPlanID65;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_HK_TH_VN(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for HK_TH_VN, ChannelPlanID 64
        case 64: switch (channel)
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
                // Invalid channel for HK_TH_VN, ChannelPlanID 64
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for HK_TH_VN, ChannelPlanID 65
        case 65: switch (channel)
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
                // Invalid channel for HK_TH_VN, ChannelPlanID 65
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_HK_TH_VN(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 64;
        case 1: return 65;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_HK_TH_VN(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for HK_TH_VN, ChannelPlanID 64
        case 64: switch (frequency)
            {
                case 920200: return 0;
                case 920400: return 1;
                case 920600: return 2;
                case 920800: return 3;
                case 921000: return 4;
                case 921200: return 5;
                case 921400: return 6;
                case 921600: return 7;
                case 921800: return 8;
                case 922000: return 9;
                case 922200: return 10;
                case 922400: return 11;
                case 922600: return 12;
                case 922800: return 13;
                case 923000: return 14;
                case 923200: return 15;
                case 923400: return 16;
                case 923600: return 17;
                case 923800: return 18;
                case 924000: return 19;
                case 924200: return 20;
                case 924400: return 21;
                case 924600: return 22;
                case 924800: return 23;
                // Invalid frequency for HK_TH_VN, ChannelPlanID 64
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for HK_TH_VN, ChannelPlanID 65
        case 65: switch (frequency)
            {
                case 920400: return 0;
                case 920800: return 1;
                case 921200: return 2;
                case 921600: return 3;
                case 922000: return 4;
                case 922400: return 5;
                case 922800: return 6;
                case 923200: return 7;
                case 923600: return 8;
                case 924000: return 9;
                case 924400: return 10;
                case 924800: return 11;
                // Invalid frequency for HK_TH_VN, ChannelPlanID 65
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_HK_TH_VN(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for HK_TH_VN, ChannelPlanID 64
        case 64: switch (channel)
            {
                case 0: return 920200;
                case 1: return 920400;
                case 2: return 920600;
                case 3: return 920800;
                case 4: return 921000;
                case 5: return 921200;
                case 6: return 921400;
                case 7: return 921600;
                case 8: return 921800;
                case 9: return 922000;
                case 10: return 922200;
                case 11: return 922400;
                case 12: return 922600;
                case 13: return 922800;
                case 14: return 923000;
                case 15: return 923200;
                case 16: return 923400;
                case 17: return 923600;
                case 18: return 923800;
                case 19: return 924000;
                case 20: return 924200;
                case 21: return 924400;
                case 22: return 924600;
                case 23: return 924800;
                // Invalid channel for HK_TH_VN, ChannelPlanID 64
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for HK_TH_VN, ChannelPlanID 65
        case 65: switch (channel)
            {
                case 0: return 920400;
                case 1: return 920800;
                case 2: return 921200;
                case 3: return 921600;
                case 4: return 922000;
                case 5: return 922400;
                case 6: return 922800;
                case 7: return 923200;
                case 8: return 923600;
                case 9: return 924000;
                case 10: return 924400;
                case 11: return 924800;
                // Invalid channel for HK_TH_VN, ChannelPlanID 65
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

