
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


#include "wisun_delta_tables_MX.h"

// DeltaTable for MX, ChannelPlanID 1
// Size in bytes: 16
const uint32_t wisun_delta_table_MX_from_ChannelPlanID1[] =
{
    0x00000003,    //  ConfigSegment(length=3, feature_mask=0, inverted=False, compound=False)
    0x00309001,    //  ConfigRegion(length=2, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00020064,    //  ConfigValue32(value32=131172)
    0x00fe0000     //  ConfigValue32(value32=16646144)
};

// DeltaTable for MX, ChannelPlanID 2
// Size in bytes: 12
const uint32_t wisun_delta_table_MX_from_ChannelPlanID2[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_MX(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 1: return wisun_delta_table_MX_from_ChannelPlanID1;
        case 2: return wisun_delta_table_MX_from_ChannelPlanID2;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_MX(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for MX, ChannelPlanID 1
        case 1: switch (channel)
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
                case 64: return 0;
                case 65: return 2;
                case 66: return 0;
                case 67: return 2;
                case 68: return 0;
                case 69: return 2;
                case 70: return 0;
                case 71: return 2;
                case 72: return 0;
                case 73: return 2;
                case 74: return 0;
                case 75: return 2;
                case 76: return 0;
                case 77: return 2;
                case 78: return 0;
                case 79: return 2;
                case 80: return 0;
                case 81: return 2;
                case 82: return 0;
                case 83: return 2;
                case 84: return 0;
                case 85: return 2;
                case 86: return 0;
                case 87: return 2;
                case 88: return 0;
                case 89: return 2;
                case 90: return 0;
                case 91: return 2;
                case 92: return 0;
                case 93: return 2;
                case 94: return 0;
                case 95: return 2;
                case 96: return 0;
                case 97: return 2;
                case 98: return 0;
                case 99: return 2;
                case 100: return 0;
                case 101: return 2;
                case 102: return 0;
                case 103: return 2;
                case 104: return 0;
                case 105: return 2;
                case 106: return 0;
                case 107: return 2;
                case 108: return 0;
                case 109: return 2;
                case 110: return 0;
                case 111: return 2;
                case 112: return 0;
                case 113: return 2;
                case 114: return 0;
                case 115: return 2;
                case 116: return 0;
                case 117: return 2;
                case 118: return 0;
                case 119: return 2;
                case 120: return 0;
                case 121: return 2;
                case 122: return 0;
                case 123: return 2;
                case 124: return 0;
                case 125: return 2;
                case 126: return 0;
                case 127: return 2;
                case 128: return 4;
                // Invalid channel for MX, ChannelPlanID 1
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Channel to BaseIndex mapping for MX, ChannelPlanID 2
        case 2: switch (channel)
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
                case 32: return 0;
                case 33: return 0;
                case 34: return 0;
                case 35: return 0;
                case 36: return 0;
                case 37: return 0;
                case 38: return 0;
                case 39: return 0;
                case 40: return 0;
                case 41: return 0;
                case 42: return 0;
                case 43: return 0;
                case 44: return 0;
                case 45: return 0;
                case 46: return 0;
                case 47: return 0;
                case 48: return 0;
                case 49: return 0;
                case 50: return 0;
                case 51: return 0;
                case 52: return 0;
                case 53: return 0;
                case 54: return 0;
                case 55: return 0;
                case 56: return 0;
                case 57: return 0;
                case 58: return 0;
                case 59: return 0;
                case 60: return 0;
                case 61: return 0;
                case 62: return 0;
                case 63: return 0;
                // Invalid channel for MX, ChannelPlanID 2
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_MX(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 1;
        case 1: return 2;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_MX(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for MX, ChannelPlanID 1
        case 1: switch (frequency)
            {
                case 902200: return 0;
                case 902400: return 1;
                case 902600: return 2;
                case 902800: return 3;
                case 903000: return 4;
                case 903200: return 5;
                case 903400: return 6;
                case 903600: return 7;
                case 903800: return 8;
                case 904000: return 9;
                case 904200: return 10;
                case 904400: return 11;
                case 904600: return 12;
                case 904800: return 13;
                case 905000: return 14;
                case 905200: return 15;
                case 905400: return 16;
                case 905600: return 17;
                case 905800: return 18;
                case 906000: return 19;
                case 906200: return 20;
                case 906400: return 21;
                case 906600: return 22;
                case 906800: return 23;
                case 907000: return 24;
                case 907200: return 25;
                case 907400: return 26;
                case 907600: return 27;
                case 907800: return 28;
                case 908000: return 29;
                case 908200: return 30;
                case 908400: return 31;
                case 908600: return 32;
                case 908800: return 33;
                case 909000: return 34;
                case 909200: return 35;
                case 909400: return 36;
                case 909600: return 37;
                case 909800: return 38;
                case 910000: return 39;
                case 910200: return 40;
                case 910400: return 41;
                case 910600: return 42;
                case 910800: return 43;
                case 911000: return 44;
                case 911200: return 45;
                case 911400: return 46;
                case 911600: return 47;
                case 911800: return 48;
                case 912000: return 49;
                case 912200: return 50;
                case 912400: return 51;
                case 912600: return 52;
                case 912800: return 53;
                case 913000: return 54;
                case 913200: return 55;
                case 913400: return 56;
                case 913600: return 57;
                case 913800: return 58;
                case 914000: return 59;
                case 914200: return 60;
                case 914400: return 61;
                case 914600: return 62;
                case 914800: return 63;
                case 915000: return 64;
                case 915200: return 65;
                case 915400: return 66;
                case 915600: return 67;
                case 915800: return 68;
                case 916000: return 69;
                case 916200: return 70;
                case 916400: return 71;
                case 916600: return 72;
                case 916800: return 73;
                case 917000: return 74;
                case 917200: return 75;
                case 917400: return 76;
                case 917600: return 77;
                case 917800: return 78;
                case 918000: return 79;
                case 918200: return 80;
                case 918400: return 81;
                case 918600: return 82;
                case 918800: return 83;
                case 919000: return 84;
                case 919200: return 85;
                case 919400: return 86;
                case 919600: return 87;
                case 919800: return 88;
                case 920000: return 89;
                case 920200: return 90;
                case 920400: return 91;
                case 920600: return 92;
                case 920800: return 93;
                case 921000: return 94;
                case 921200: return 95;
                case 921400: return 96;
                case 921600: return 97;
                case 921800: return 98;
                case 922000: return 99;
                case 922200: return 100;
                case 922400: return 101;
                case 922600: return 102;
                case 922800: return 103;
                case 923000: return 104;
                case 923200: return 105;
                case 923400: return 106;
                case 923600: return 107;
                case 923800: return 108;
                case 924000: return 109;
                case 924200: return 110;
                case 924400: return 111;
                case 924600: return 112;
                case 924800: return 113;
                case 925000: return 114;
                case 925200: return 115;
                case 925400: return 116;
                case 925600: return 117;
                case 925800: return 118;
                case 926000: return 119;
                case 926200: return 120;
                case 926400: return 121;
                case 926600: return 122;
                case 926800: return 123;
                case 927000: return 124;
                case 927200: return 125;
                case 927400: return 126;
                case 927600: return 127;
                case 927800: return 128;
                // Invalid frequency for MX, ChannelPlanID 1
                default: return WISUN_CHANNEL_INVALID;
            }
        // Frequency to channel mapping for MX, ChannelPlanID 2
        case 2: switch (frequency)
            {
                case 902400: return 0;
                case 902800: return 1;
                case 903200: return 2;
                case 903600: return 3;
                case 904000: return 4;
                case 904400: return 5;
                case 904800: return 6;
                case 905200: return 7;
                case 905600: return 8;
                case 906000: return 9;
                case 906400: return 10;
                case 906800: return 11;
                case 907200: return 12;
                case 907600: return 13;
                case 908000: return 14;
                case 908400: return 15;
                case 908800: return 16;
                case 909200: return 17;
                case 909600: return 18;
                case 910000: return 19;
                case 910400: return 20;
                case 910800: return 21;
                case 911200: return 22;
                case 911600: return 23;
                case 912000: return 24;
                case 912400: return 25;
                case 912800: return 26;
                case 913200: return 27;
                case 913600: return 28;
                case 914000: return 29;
                case 914400: return 30;
                case 914800: return 31;
                case 915200: return 32;
                case 915600: return 33;
                case 916000: return 34;
                case 916400: return 35;
                case 916800: return 36;
                case 917200: return 37;
                case 917600: return 38;
                case 918000: return 39;
                case 918400: return 40;
                case 918800: return 41;
                case 919200: return 42;
                case 919600: return 43;
                case 920000: return 44;
                case 920400: return 45;
                case 920800: return 46;
                case 921200: return 47;
                case 921600: return 48;
                case 922000: return 49;
                case 922400: return 50;
                case 922800: return 51;
                case 923200: return 52;
                case 923600: return 53;
                case 924000: return 54;
                case 924400: return 55;
                case 924800: return 56;
                case 925200: return 57;
                case 925600: return 58;
                case 926000: return 59;
                case 926400: return 60;
                case 926800: return 61;
                case 927200: return 62;
                case 927600: return 63;
                // Invalid frequency for MX, ChannelPlanID 2
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_MX(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for MX, ChannelPlanID 1
        case 1: switch (channel)
            {
                case 0: return 902200;
                case 1: return 902400;
                case 2: return 902600;
                case 3: return 902800;
                case 4: return 903000;
                case 5: return 903200;
                case 6: return 903400;
                case 7: return 903600;
                case 8: return 903800;
                case 9: return 904000;
                case 10: return 904200;
                case 11: return 904400;
                case 12: return 904600;
                case 13: return 904800;
                case 14: return 905000;
                case 15: return 905200;
                case 16: return 905400;
                case 17: return 905600;
                case 18: return 905800;
                case 19: return 906000;
                case 20: return 906200;
                case 21: return 906400;
                case 22: return 906600;
                case 23: return 906800;
                case 24: return 907000;
                case 25: return 907200;
                case 26: return 907400;
                case 27: return 907600;
                case 28: return 907800;
                case 29: return 908000;
                case 30: return 908200;
                case 31: return 908400;
                case 32: return 908600;
                case 33: return 908800;
                case 34: return 909000;
                case 35: return 909200;
                case 36: return 909400;
                case 37: return 909600;
                case 38: return 909800;
                case 39: return 910000;
                case 40: return 910200;
                case 41: return 910400;
                case 42: return 910600;
                case 43: return 910800;
                case 44: return 911000;
                case 45: return 911200;
                case 46: return 911400;
                case 47: return 911600;
                case 48: return 911800;
                case 49: return 912000;
                case 50: return 912200;
                case 51: return 912400;
                case 52: return 912600;
                case 53: return 912800;
                case 54: return 913000;
                case 55: return 913200;
                case 56: return 913400;
                case 57: return 913600;
                case 58: return 913800;
                case 59: return 914000;
                case 60: return 914200;
                case 61: return 914400;
                case 62: return 914600;
                case 63: return 914800;
                case 64: return 915000;
                case 65: return 915200;
                case 66: return 915400;
                case 67: return 915600;
                case 68: return 915800;
                case 69: return 916000;
                case 70: return 916200;
                case 71: return 916400;
                case 72: return 916600;
                case 73: return 916800;
                case 74: return 917000;
                case 75: return 917200;
                case 76: return 917400;
                case 77: return 917600;
                case 78: return 917800;
                case 79: return 918000;
                case 80: return 918200;
                case 81: return 918400;
                case 82: return 918600;
                case 83: return 918800;
                case 84: return 919000;
                case 85: return 919200;
                case 86: return 919400;
                case 87: return 919600;
                case 88: return 919800;
                case 89: return 920000;
                case 90: return 920200;
                case 91: return 920400;
                case 92: return 920600;
                case 93: return 920800;
                case 94: return 921000;
                case 95: return 921200;
                case 96: return 921400;
                case 97: return 921600;
                case 98: return 921800;
                case 99: return 922000;
                case 100: return 922200;
                case 101: return 922400;
                case 102: return 922600;
                case 103: return 922800;
                case 104: return 923000;
                case 105: return 923200;
                case 106: return 923400;
                case 107: return 923600;
                case 108: return 923800;
                case 109: return 924000;
                case 110: return 924200;
                case 111: return 924400;
                case 112: return 924600;
                case 113: return 924800;
                case 114: return 925000;
                case 115: return 925200;
                case 116: return 925400;
                case 117: return 925600;
                case 118: return 925800;
                case 119: return 926000;
                case 120: return 926200;
                case 121: return 926400;
                case 122: return 926600;
                case 123: return 926800;
                case 124: return 927000;
                case 125: return 927200;
                case 126: return 927400;
                case 127: return 927600;
                case 128: return 927800;
                // Invalid channel for MX, ChannelPlanID 1
                default: return WISUN_FREQ_INVALID;
            }
        // Frequency to channel mapping for MX, ChannelPlanID 2
        case 2: switch (channel)
            {
                case 0: return 902400;
                case 1: return 902800;
                case 2: return 903200;
                case 3: return 903600;
                case 4: return 904000;
                case 5: return 904400;
                case 6: return 904800;
                case 7: return 905200;
                case 8: return 905600;
                case 9: return 906000;
                case 10: return 906400;
                case 11: return 906800;
                case 12: return 907200;
                case 13: return 907600;
                case 14: return 908000;
                case 15: return 908400;
                case 16: return 908800;
                case 17: return 909200;
                case 18: return 909600;
                case 19: return 910000;
                case 20: return 910400;
                case 21: return 910800;
                case 22: return 911200;
                case 23: return 911600;
                case 24: return 912000;
                case 25: return 912400;
                case 26: return 912800;
                case 27: return 913200;
                case 28: return 913600;
                case 29: return 914000;
                case 30: return 914400;
                case 31: return 914800;
                case 32: return 915200;
                case 33: return 915600;
                case 34: return 916000;
                case 35: return 916400;
                case 36: return 916800;
                case 37: return 917200;
                case 38: return 917600;
                case 39: return 918000;
                case 40: return 918400;
                case 41: return 918800;
                case 42: return 919200;
                case 43: return 919600;
                case 44: return 920000;
                case 45: return 920400;
                case 46: return 920800;
                case 47: return 921200;
                case 48: return 921600;
                case 49: return 922000;
                case 50: return 922400;
                case 51: return 922800;
                case 52: return 923200;
                case 53: return 923600;
                case 54: return 924000;
                case 55: return 924400;
                case 56: return 924800;
                case 57: return 925200;
                case 58: return 925600;
                case 59: return 926000;
                case 60: return 926400;
                case 61: return 926800;
                case 62: return 927200;
                case 63: return 927600;
                // Invalid channel for MX, ChannelPlanID 2
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

