
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


#include "wisun_delta_tables_CN1.h"

// DeltaTable for CN1, ChannelPlanID 160
// Size in bytes: 12
const uint32_t wisun_delta_table_CN1_from_ChannelPlanID160[] =
{
    0x00000002,    //  ConfigSegment(length=2, feature_mask=0, inverted=False, compound=False)
    0x00309000,    //  ConfigRegion(length=1, operation=<ConfigRegionOperation.Par_Reference_32bit: 9>, address=48)
    0x00000064     //  ConfigValue32(value32=100)
};

/*****************************************
 * HOST FUNCTIONS
 ****************************************/
const uint32_t* get_wisun_delta_table_CN1(uint16_t channelPlanId)
{
    switch (channelPlanId)
    {
        case 160: return wisun_delta_table_CN1_from_ChannelPlanID160;
        default: return 0;
    }
}

int16_t get_wisun_delta_table_baseindex_CN1(uint16_t channelPlanId, uint16_t channel)
{
    switch (channelPlanId)
    {
        // Channel to BaseIndex mapping for CN1, ChannelPlanID 160
        case 160: switch (channel)
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
                case 64: return 0;
                case 65: return 0;
                case 66: return 0;
                case 67: return 0;
                case 68: return 0;
                case 69: return 0;
                case 70: return 0;
                case 71: return 0;
                case 72: return 0;
                case 73: return 0;
                case 74: return 0;
                case 75: return 0;
                case 76: return 0;
                case 77: return 0;
                case 78: return 0;
                case 79: return 0;
                case 80: return 0;
                case 81: return 0;
                case 82: return 0;
                case 83: return 0;
                case 84: return 0;
                case 85: return 0;
                case 86: return 0;
                case 87: return 0;
                case 88: return 0;
                case 89: return 0;
                case 90: return 0;
                case 91: return 0;
                case 92: return 0;
                case 93: return 0;
                case 94: return 0;
                case 95: return 0;
                case 96: return 0;
                case 97: return 0;
                case 98: return 0;
                case 99: return 0;
                case 100: return 0;
                case 101: return 0;
                case 102: return 0;
                case 103: return 0;
                case 104: return 0;
                case 105: return 0;
                case 106: return 0;
                case 107: return 0;
                case 108: return 0;
                case 109: return 0;
                case 110: return 0;
                case 111: return 0;
                case 112: return 0;
                case 113: return 0;
                case 114: return 0;
                case 115: return 0;
                case 116: return 0;
                case 117: return 0;
                case 118: return 0;
                case 119: return 0;
                case 120: return 0;
                case 121: return 0;
                case 122: return 0;
                case 123: return 0;
                case 124: return 0;
                case 125: return 0;
                case 126: return 0;
                case 127: return 0;
                case 128: return 0;
                case 129: return 0;
                case 130: return 0;
                case 131: return 0;
                case 132: return 0;
                case 133: return 0;
                case 134: return 0;
                case 135: return 0;
                case 136: return 0;
                case 137: return 0;
                case 138: return 0;
                case 139: return 0;
                case 140: return 0;
                case 141: return 0;
                case 142: return 0;
                case 143: return 0;
                case 144: return 0;
                case 145: return 0;
                case 146: return 0;
                case 147: return 0;
                case 148: return 0;
                case 149: return 0;
                case 150: return 0;
                case 151: return 0;
                case 152: return 0;
                case 153: return 0;
                case 154: return 0;
                case 155: return 0;
                case 156: return 0;
                case 157: return 0;
                case 158: return 0;
                case 159: return 0;
                case 160: return 0;
                case 161: return 0;
                case 162: return 0;
                case 163: return 0;
                case 164: return 0;
                case 165: return 0;
                case 166: return 0;
                case 167: return 0;
                case 168: return 0;
                case 169: return 0;
                case 170: return 0;
                case 171: return 0;
                case 172: return 0;
                case 173: return 0;
                case 174: return 0;
                case 175: return 0;
                case 176: return 0;
                case 177: return 0;
                case 178: return 0;
                case 179: return 0;
                case 180: return 0;
                case 181: return 0;
                case 182: return 0;
                case 183: return 0;
                case 184: return 0;
                case 185: return 0;
                case 186: return 0;
                case 187: return 0;
                case 188: return 0;
                case 189: return 0;
                case 190: return 0;
                case 191: return 0;
                case 192: return 0;
                case 193: return 0;
                case 194: return 0;
                case 195: return 0;
                case 196: return 0;
                case 197: return 0;
                case 198: return 0;
                // Invalid channel for CN1, ChannelPlanID 160
                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
            }
        // Invalid ChannelPlanID
        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;
    }
}

/*****************************************
 * INTERNAL HELPER FUNCTIONS
 ****************************************/
int16_t get_wisun_channelPlanId_from_channelPlanIdIdx_CN1(uint8_t channelPlanIdIdx)
{
    switch (channelPlanIdIdx)
    {
        case 0: return 160;
        default: return WISUN_CHANNELPLANID_INVALID;
    }
}

int16_t get_wisun_channel_from_frequency_CN1(uint16_t channelPlanId, uint32_t frequency)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for CN1, ChannelPlanID 160
        case 160: switch (frequency)
            {
                case 470200: return 0;
                case 470400: return 1;
                case 470600: return 2;
                case 470800: return 3;
                case 471000: return 4;
                case 471200: return 5;
                case 471400: return 6;
                case 471600: return 7;
                case 471800: return 8;
                case 472000: return 9;
                case 472200: return 10;
                case 472400: return 11;
                case 472600: return 12;
                case 472800: return 13;
                case 473000: return 14;
                case 473200: return 15;
                case 473400: return 16;
                case 473600: return 17;
                case 473800: return 18;
                case 474000: return 19;
                case 474200: return 20;
                case 474400: return 21;
                case 474600: return 22;
                case 474800: return 23;
                case 475000: return 24;
                case 475200: return 25;
                case 475400: return 26;
                case 475600: return 27;
                case 475800: return 28;
                case 476000: return 29;
                case 476200: return 30;
                case 476400: return 31;
                case 476600: return 32;
                case 476800: return 33;
                case 477000: return 34;
                case 477200: return 35;
                case 477400: return 36;
                case 477600: return 37;
                case 477800: return 38;
                case 478000: return 39;
                case 478200: return 40;
                case 478400: return 41;
                case 478600: return 42;
                case 478800: return 43;
                case 479000: return 44;
                case 479200: return 45;
                case 479400: return 46;
                case 479600: return 47;
                case 479800: return 48;
                case 480000: return 49;
                case 480200: return 50;
                case 480400: return 51;
                case 480600: return 52;
                case 480800: return 53;
                case 481000: return 54;
                case 481200: return 55;
                case 481400: return 56;
                case 481600: return 57;
                case 481800: return 58;
                case 482000: return 59;
                case 482200: return 60;
                case 482400: return 61;
                case 482600: return 62;
                case 482800: return 63;
                case 483000: return 64;
                case 483200: return 65;
                case 483400: return 66;
                case 483600: return 67;
                case 483800: return 68;
                case 484000: return 69;
                case 484200: return 70;
                case 484400: return 71;
                case 484600: return 72;
                case 484800: return 73;
                case 485000: return 74;
                case 485200: return 75;
                case 485400: return 76;
                case 485600: return 77;
                case 485800: return 78;
                case 486000: return 79;
                case 486200: return 80;
                case 486400: return 81;
                case 486600: return 82;
                case 486800: return 83;
                case 487000: return 84;
                case 487200: return 85;
                case 487400: return 86;
                case 487600: return 87;
                case 487800: return 88;
                case 488000: return 89;
                case 488200: return 90;
                case 488400: return 91;
                case 488600: return 92;
                case 488800: return 93;
                case 489000: return 94;
                case 489200: return 95;
                case 489400: return 96;
                case 489600: return 97;
                case 489800: return 98;
                case 490000: return 99;
                case 490200: return 100;
                case 490400: return 101;
                case 490600: return 102;
                case 490800: return 103;
                case 491000: return 104;
                case 491200: return 105;
                case 491400: return 106;
                case 491600: return 107;
                case 491800: return 108;
                case 492000: return 109;
                case 492200: return 110;
                case 492400: return 111;
                case 492600: return 112;
                case 492800: return 113;
                case 493000: return 114;
                case 493200: return 115;
                case 493400: return 116;
                case 493600: return 117;
                case 493800: return 118;
                case 494000: return 119;
                case 494200: return 120;
                case 494400: return 121;
                case 494600: return 122;
                case 494800: return 123;
                case 495000: return 124;
                case 495200: return 125;
                case 495400: return 126;
                case 495600: return 127;
                case 495800: return 128;
                case 496000: return 129;
                case 496200: return 130;
                case 496400: return 131;
                case 496600: return 132;
                case 496800: return 133;
                case 497000: return 134;
                case 497200: return 135;
                case 497400: return 136;
                case 497600: return 137;
                case 497800: return 138;
                case 498000: return 139;
                case 498200: return 140;
                case 498400: return 141;
                case 498600: return 142;
                case 498800: return 143;
                case 499000: return 144;
                case 499200: return 145;
                case 499400: return 146;
                case 499600: return 147;
                case 499800: return 148;
                case 500000: return 149;
                case 500200: return 150;
                case 500400: return 151;
                case 500600: return 152;
                case 500800: return 153;
                case 501000: return 154;
                case 501200: return 155;
                case 501400: return 156;
                case 501600: return 157;
                case 501800: return 158;
                case 502000: return 159;
                case 502200: return 160;
                case 502400: return 161;
                case 502600: return 162;
                case 502800: return 163;
                case 503000: return 164;
                case 503200: return 165;
                case 503400: return 166;
                case 503600: return 167;
                case 503800: return 168;
                case 504000: return 169;
                case 504200: return 170;
                case 504400: return 171;
                case 504600: return 172;
                case 504800: return 173;
                case 505000: return 174;
                case 505200: return 175;
                case 505400: return 176;
                case 505600: return 177;
                case 505800: return 178;
                case 506000: return 179;
                case 506200: return 180;
                case 506400: return 181;
                case 506600: return 182;
                case 506800: return 183;
                case 507000: return 184;
                case 507200: return 185;
                case 507400: return 186;
                case 507600: return 187;
                case 507800: return 188;
                case 508000: return 189;
                case 508200: return 190;
                case 508400: return 191;
                case 508600: return 192;
                case 508800: return 193;
                case 509000: return 194;
                case 509200: return 195;
                case 509400: return 196;
                case 509600: return 197;
                case 509800: return 198;
                // Invalid frequency for CN1, ChannelPlanID 160
                default: return WISUN_CHANNEL_INVALID;
            }
        default: return WISUN_CHANNEL_INVALID;
    }
}

uint32_t get_wisun_frequency_from_channel_CN1(uint16_t channelPlanId, int16_t channel)
{
    switch (channelPlanId)
    {
        // Frequency to channel mapping for CN1, ChannelPlanID 160
        case 160: switch (channel)
            {
                case 0: return 470200;
                case 1: return 470400;
                case 2: return 470600;
                case 3: return 470800;
                case 4: return 471000;
                case 5: return 471200;
                case 6: return 471400;
                case 7: return 471600;
                case 8: return 471800;
                case 9: return 472000;
                case 10: return 472200;
                case 11: return 472400;
                case 12: return 472600;
                case 13: return 472800;
                case 14: return 473000;
                case 15: return 473200;
                case 16: return 473400;
                case 17: return 473600;
                case 18: return 473800;
                case 19: return 474000;
                case 20: return 474200;
                case 21: return 474400;
                case 22: return 474600;
                case 23: return 474800;
                case 24: return 475000;
                case 25: return 475200;
                case 26: return 475400;
                case 27: return 475600;
                case 28: return 475800;
                case 29: return 476000;
                case 30: return 476200;
                case 31: return 476400;
                case 32: return 476600;
                case 33: return 476800;
                case 34: return 477000;
                case 35: return 477200;
                case 36: return 477400;
                case 37: return 477600;
                case 38: return 477800;
                case 39: return 478000;
                case 40: return 478200;
                case 41: return 478400;
                case 42: return 478600;
                case 43: return 478800;
                case 44: return 479000;
                case 45: return 479200;
                case 46: return 479400;
                case 47: return 479600;
                case 48: return 479800;
                case 49: return 480000;
                case 50: return 480200;
                case 51: return 480400;
                case 52: return 480600;
                case 53: return 480800;
                case 54: return 481000;
                case 55: return 481200;
                case 56: return 481400;
                case 57: return 481600;
                case 58: return 481800;
                case 59: return 482000;
                case 60: return 482200;
                case 61: return 482400;
                case 62: return 482600;
                case 63: return 482800;
                case 64: return 483000;
                case 65: return 483200;
                case 66: return 483400;
                case 67: return 483600;
                case 68: return 483800;
                case 69: return 484000;
                case 70: return 484200;
                case 71: return 484400;
                case 72: return 484600;
                case 73: return 484800;
                case 74: return 485000;
                case 75: return 485200;
                case 76: return 485400;
                case 77: return 485600;
                case 78: return 485800;
                case 79: return 486000;
                case 80: return 486200;
                case 81: return 486400;
                case 82: return 486600;
                case 83: return 486800;
                case 84: return 487000;
                case 85: return 487200;
                case 86: return 487400;
                case 87: return 487600;
                case 88: return 487800;
                case 89: return 488000;
                case 90: return 488200;
                case 91: return 488400;
                case 92: return 488600;
                case 93: return 488800;
                case 94: return 489000;
                case 95: return 489200;
                case 96: return 489400;
                case 97: return 489600;
                case 98: return 489800;
                case 99: return 490000;
                case 100: return 490200;
                case 101: return 490400;
                case 102: return 490600;
                case 103: return 490800;
                case 104: return 491000;
                case 105: return 491200;
                case 106: return 491400;
                case 107: return 491600;
                case 108: return 491800;
                case 109: return 492000;
                case 110: return 492200;
                case 111: return 492400;
                case 112: return 492600;
                case 113: return 492800;
                case 114: return 493000;
                case 115: return 493200;
                case 116: return 493400;
                case 117: return 493600;
                case 118: return 493800;
                case 119: return 494000;
                case 120: return 494200;
                case 121: return 494400;
                case 122: return 494600;
                case 123: return 494800;
                case 124: return 495000;
                case 125: return 495200;
                case 126: return 495400;
                case 127: return 495600;
                case 128: return 495800;
                case 129: return 496000;
                case 130: return 496200;
                case 131: return 496400;
                case 132: return 496600;
                case 133: return 496800;
                case 134: return 497000;
                case 135: return 497200;
                case 136: return 497400;
                case 137: return 497600;
                case 138: return 497800;
                case 139: return 498000;
                case 140: return 498200;
                case 141: return 498400;
                case 142: return 498600;
                case 143: return 498800;
                case 144: return 499000;
                case 145: return 499200;
                case 146: return 499400;
                case 147: return 499600;
                case 148: return 499800;
                case 149: return 500000;
                case 150: return 500200;
                case 151: return 500400;
                case 152: return 500600;
                case 153: return 500800;
                case 154: return 501000;
                case 155: return 501200;
                case 156: return 501400;
                case 157: return 501600;
                case 158: return 501800;
                case 159: return 502000;
                case 160: return 502200;
                case 161: return 502400;
                case 162: return 502600;
                case 163: return 502800;
                case 164: return 503000;
                case 165: return 503200;
                case 166: return 503400;
                case 167: return 503600;
                case 168: return 503800;
                case 169: return 504000;
                case 170: return 504200;
                case 171: return 504400;
                case 172: return 504600;
                case 173: return 504800;
                case 174: return 505000;
                case 175: return 505200;
                case 176: return 505400;
                case 177: return 505600;
                case 178: return 505800;
                case 179: return 506000;
                case 180: return 506200;
                case 181: return 506400;
                case 182: return 506600;
                case 183: return 506800;
                case 184: return 507000;
                case 185: return 507200;
                case 186: return 507400;
                case 187: return 507600;
                case 188: return 507800;
                case 189: return 508000;
                case 190: return 508200;
                case 191: return 508400;
                case 192: return 508600;
                case 193: return 508800;
                case 194: return 509000;
                case 195: return 509200;
                case 196: return 509400;
                case 197: return 509600;
                case 198: return 509800;
                // Invalid channel for CN1, ChannelPlanID 160
                default: return WISUN_FREQ_INVALID;
            }
        default: return WISUN_FREQ_INVALID;
    }
}

