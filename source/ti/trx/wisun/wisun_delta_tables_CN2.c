
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


#include "wisun_delta_tables_CN2.h"

// DeltaTable for CN2, ChannelPlanID 144
// Size in bytes: 16
const uint32_t wisun_delta_table_CN2_from_ChannelPlanID144[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00020064,    //  ConfigValue32(value32=131172)
    0x00fe0000     //  ConfigValue32(value32=16646144)
};

// DeltaTable for CN2, ChannelPlanID 145
// Size in bytes: 12
const uint32_t wisun_delta_table_CN2_from_ChannelPlanID145[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_CN2(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 144: return wisun_delta_table_CN2_from_ChannelPlanID144;
        case 145: return wisun_delta_table_CN2_from_ChannelPlanID145;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_CN2(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for CN2, ChannelPlanID 144
        case 144: switch (channel)
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
                case 38: return 4;
                // Invalid channel for CN2, ChannelPlanID 144
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for CN2, ChannelPlanID 145
        case 145: switch (channel)
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
                // Invalid channel for CN2, ChannelPlanID 145
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_CN2(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 144;
        case 1: return 145;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_CN2(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for CN2, ChannelPlanID 144
        case 144: switch (frequency)
            {
                case 779200: return 0;
                case 779400: return 1;
                case 779600: return 2;
                case 779800: return 3;
                case 780000: return 4;
                case 780200: return 5;
                case 780400: return 6;
                case 780600: return 7;
                case 780800: return 8;
                case 781000: return 9;
                case 781200: return 10;
                case 781400: return 11;
                case 781600: return 12;
                case 781800: return 13;
                case 782000: return 14;
                case 782200: return 15;
                case 782400: return 16;
                case 782600: return 17;
                case 782800: return 18;
                case 783000: return 19;
                case 783200: return 20;
                case 783400: return 21;
                case 783600: return 22;
                case 783800: return 23;
                case 784000: return 24;
                case 784200: return 25;
                case 784400: return 26;
                case 784600: return 27;
                case 784800: return 28;
                case 785000: return 29;
                case 785200: return 30;
                case 785400: return 31;
                case 785600: return 32;
                case 785800: return 33;
                case 786000: return 34;
                case 786200: return 35;
                case 786400: return 36;
                case 786600: return 37;
                case 786800: return 38;
                // Invalid frequency for CN2, ChannelPlanID 144
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for CN2, ChannelPlanID 145
        case 145: switch (frequency)
            {
                case 779400: return 0;
                case 779800: return 1;
                case 780200: return 2;
                case 780600: return 3;
                case 781000: return 4;
                case 781400: return 5;
                case 781800: return 6;
                case 782200: return 7;
                case 782600: return 8;
                case 783000: return 9;
                case 783400: return 10;
                case 783800: return 11;
                case 784200: return 12;
                case 784600: return 13;
                case 785000: return 14;
                case 785400: return 15;
                case 785800: return 16;
                case 786200: return 17;
                case 786600: return 18;
                // Invalid frequency for CN2, ChannelPlanID 145
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_CN2(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for CN2, ChannelPlanID 144
        case 144: switch (channel)
            {
                case 0: return 779200;
                case 1: return 779400;
                case 2: return 779600;
                case 3: return 779800;
                case 4: return 780000;
                case 5: return 780200;
                case 6: return 780400;
                case 7: return 780600;
                case 8: return 780800;
                case 9: return 781000;
                case 10: return 781200;
                case 11: return 781400;
                case 12: return 781600;
                case 13: return 781800;
                case 14: return 782000;
                case 15: return 782200;
                case 16: return 782400;
                case 17: return 782600;
                case 18: return 782800;
                case 19: return 783000;
                case 20: return 783200;
                case 21: return 783400;
                case 22: return 783600;
                case 23: return 783800;
                case 24: return 784000;
                case 25: return 784200;
                case 26: return 784400;
                case 27: return 784600;
                case 28: return 784800;
                case 29: return 785000;
                case 30: return 785200;
                case 31: return 785400;
                case 32: return 785600;
                case 33: return 785800;
                case 34: return 786000;
                case 35: return 786200;
                case 36: return 786400;
                case 37: return 786600;
                case 38: return 786800;
                // Invalid channel for CN2, ChannelPlanID 144
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for CN2, ChannelPlanID 145
        case 145: switch (channel)
            {
                case 0: return 779400;
                case 1: return 779800;
                case 2: return 780200;
                case 3: return 780600;
                case 4: return 781000;
                case 5: return 781400;
                case 6: return 781800;
                case 7: return 782200;
                case 8: return 782600;
                case 9: return 783000;
                case 10: return 783400;
                case 11: return 783800;
                case 12: return 784200;
                case 13: return 784600;
                case 14: return 785000;
                case 15: return 785400;
                case 16: return 785800;
                case 17: return 786200;
                case 18: return 786600;
                // Invalid channel for CN2, ChannelPlanID 145
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

