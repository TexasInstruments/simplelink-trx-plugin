
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


#include "wisun_delta_tables_EU2.h"

// DeltaTable for EU2, ChannelPlanID 34
// Size in bytes: 16
const uint32_t wisun_delta_table_EU2_from_ChannelPlanID34[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00010064,    //  ConfigValue32(value32=65636)
    0x00ff0000     //  ConfigValue32(value32=16711680)
};

// DeltaTable for EU2, ChannelPlanID 35
// Size in bytes: 12
const uint32_t wisun_delta_table_EU2_from_ChannelPlanID35[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_EU2(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 34: return wisun_delta_table_EU2_from_ChannelPlanID34;
        case 35: return wisun_delta_table_EU2_from_ChannelPlanID35;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_EU2(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for EU2, ChannelPlanID 34
        case 34: switch (channel)
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
                case 54: return 4;
                // Invalid channel for EU2, ChannelPlanID 34
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for EU2, ChannelPlanID 35
        case 35: switch (channel)
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
                // Invalid channel for EU2, ChannelPlanID 35
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_EU2(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 34;
        case 1: return 35;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_EU2(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for EU2, ChannelPlanID 34
        case 34: switch (frequency)
            {
                case 870100: return 0;
                case 870200: return 1;
                case 870300: return 2;
                case 870400: return 3;
                case 870500: return 4;
                case 870600: return 5;
                case 870700: return 6;
                case 870800: return 7;
                case 870900: return 8;
                case 871000: return 9;
                case 871100: return 10;
                case 871200: return 11;
                case 871300: return 12;
                case 871400: return 13;
                case 871500: return 14;
                case 871600: return 15;
                case 871700: return 16;
                case 871800: return 17;
                case 871900: return 18;
                case 872000: return 19;
                case 872100: return 20;
                case 872200: return 21;
                case 872300: return 22;
                case 872400: return 23;
                case 872500: return 24;
                case 872600: return 25;
                case 872700: return 26;
                case 872800: return 27;
                case 872900: return 28;
                case 873000: return 29;
                case 873100: return 30;
                case 873200: return 31;
                case 873300: return 32;
                case 873400: return 33;
                case 873500: return 34;
                case 873600: return 35;
                case 873700: return 36;
                case 873800: return 37;
                case 873900: return 38;
                case 874000: return 39;
                case 874100: return 40;
                case 874200: return 41;
                case 874300: return 42;
                case 874400: return 43;
                case 874500: return 44;
                case 874600: return 45;
                case 874700: return 46;
                case 874800: return 47;
                case 874900: return 48;
                case 875000: return 49;
                case 875100: return 50;
                case 875200: return 51;
                case 875300: return 52;
                case 875400: return 53;
                case 875500: return 54;
                // Invalid frequency for EU2, ChannelPlanID 34
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for EU2, ChannelPlanID 35
        case 35: switch (frequency)
            {
                case 870200: return 0;
                case 870400: return 1;
                case 870600: return 2;
                case 870800: return 3;
                case 871000: return 4;
                case 871200: return 5;
                case 871400: return 6;
                case 871600: return 7;
                case 871800: return 8;
                case 872000: return 9;
                case 872200: return 10;
                case 872400: return 11;
                case 872600: return 12;
                case 872800: return 13;
                case 873000: return 14;
                case 873200: return 15;
                case 873400: return 16;
                case 873600: return 17;
                case 873800: return 18;
                case 874000: return 19;
                case 874200: return 20;
                case 874400: return 21;
                case 874600: return 22;
                case 874800: return 23;
                case 875000: return 24;
                case 875200: return 25;
                case 875400: return 26;
                // Invalid frequency for EU2, ChannelPlanID 35
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_EU2(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for EU2, ChannelPlanID 34
        case 34: switch (channel)
            {
                case 0: return 870100;
                case 1: return 870200;
                case 2: return 870300;
                case 3: return 870400;
                case 4: return 870500;
                case 5: return 870600;
                case 6: return 870700;
                case 7: return 870800;
                case 8: return 870900;
                case 9: return 871000;
                case 10: return 871100;
                case 11: return 871200;
                case 12: return 871300;
                case 13: return 871400;
                case 14: return 871500;
                case 15: return 871600;
                case 16: return 871700;
                case 17: return 871800;
                case 18: return 871900;
                case 19: return 872000;
                case 20: return 872100;
                case 21: return 872200;
                case 22: return 872300;
                case 23: return 872400;
                case 24: return 872500;
                case 25: return 872600;
                case 26: return 872700;
                case 27: return 872800;
                case 28: return 872900;
                case 29: return 873000;
                case 30: return 873100;
                case 31: return 873200;
                case 32: return 873300;
                case 33: return 873400;
                case 34: return 873500;
                case 35: return 873600;
                case 36: return 873700;
                case 37: return 873800;
                case 38: return 873900;
                case 39: return 874000;
                case 40: return 874100;
                case 41: return 874200;
                case 42: return 874300;
                case 43: return 874400;
                case 44: return 874500;
                case 45: return 874600;
                case 46: return 874700;
                case 47: return 874800;
                case 48: return 874900;
                case 49: return 875000;
                case 50: return 875100;
                case 51: return 875200;
                case 52: return 875300;
                case 53: return 875400;
                case 54: return 875500;
                // Invalid channel for EU2, ChannelPlanID 34
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for EU2, ChannelPlanID 35
        case 35: switch (channel)
            {
                case 0: return 870200;
                case 1: return 870400;
                case 2: return 870600;
                case 3: return 870800;
                case 4: return 871000;
                case 5: return 871200;
                case 6: return 871400;
                case 7: return 871600;
                case 8: return 871800;
                case 9: return 872000;
                case 10: return 872200;
                case 11: return 872400;
                case 12: return 872600;
                case 13: return 872800;
                case 14: return 873000;
                case 15: return 873200;
                case 16: return 873400;
                case 17: return 873600;
                case 18: return 873800;
                case 19: return 874000;
                case 20: return 874200;
                case 21: return 874400;
                case 22: return 874600;
                case 23: return 874800;
                case 24: return 875000;
                case 25: return 875200;
                case 26: return 875400;
                // Invalid channel for EU2, ChannelPlanID 35
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

