/******************************************************************************
 Group: CMCU LPRF
 Target Device: cc13xx_cc26xx

 ******************************************************************************
 
 Copyright (c) 2024-2026, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 
 
 *****************************************************************************/


// Includes
// Standard C Libraries
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// TI Drivers
#include <ti/drivers/GPIO.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

// Board Header files
#include "ti_drivers_config.h"

// TRX RF Header files
#include <ti/trx/TRX.h>
#include <ti/trx/host/trx_host.h>
#include <ti/trx/host/hal/platform.h>
#include <ti/trx/wisun/wisun_delta_tables_JP.h>

/* LRF register configurations */
#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_wisun.h"
#define RF_CONFIG_SIZE (LRF_mainRegConfig_wisun_byteCount)
#define RF_CONFIG_PTR  ((uint8_t *)LRF_mainRegConfig_wisun)
#define FE_CONFIG_SIZE (LRF_frontendRegConfig_wisun_byteCount)
#define FE_CONFIG_PTR  ((uint8_t *)LRF_frontendRegConfig_wisun)


/*
 * Select PHY for the MDR Header (Modem and Option Mask)
 * The option mask for the header can be found in the rcl_settings_<PHY>.h file
 */
#define MDR_HEADER_OPTION_MASK      TRX_PHY_FEATURE_FSK_MODE_2B_WISUN

/* Select modem for the MDR Header is always TRX_RadioCommand_Modem_FSK */
#define MDR_HEADER_MODEM            TRX_RadioCommand_Modem_FSK

/*
 * Select MDR Header channel
 * Can be in the range:
 * MDR_HEADER_OPTION_MASK = TRX_PHY_FEATURE_FSK_MODE_2B_WISUN ->  3 < MDR_HEADER_CHANNEL < 18 (ChannelPlanID 22)
 * MDR_HEADER_OPTION_MASK = TRX_PHY_FEATURE_FSK_MODE_4B_WISUN ->  2 < MDR_HEADER_CHANNEL < 12 (ChannelPlanID 23)
 */
#define MDR_HEADER_CHANNEL          10
//------------------------------------------------------------------------------


/*
 * Select PHY for the MDR Packet (Modem and Option Mask)
 * The option mask for the header can be found in the rcl_settings_<PHY>.h file
 */

#define MDR_PACKET_OPTION_MASK      TRX_PHY_FEATURE_OFDM_OPTION_2_WISUN

/* Select modem for the MDR Packet */
#define MDR_PACKET_MODEM            TRX_RadioCommand_Modem_OFDM

/* Select the rate for the MDR Packet */
#define MDR_PACKET_RATE             TRX_PayloadHeader_SunOFDM_Rate_MCS5
//------------------------------------------------------------------------------

// Application specific defines
#define TX_PAYLOAD_LENGTH           (10U)   	// Number of payload bytes to be sent
#define NUMBER_OF_PACKETS           (100U)    	// Number of packet to be sent every time the button is pushed
#define PACKET_INTERVAL             (100000U)	// Delay between packets in us
#define RSSI_THRESHOLD              (-80)   	// RSSI threshold used for CS. In this example, the same threshold is used for both the
                                            	// MDR Header and the MDR Packet, regardless of PHY
// ID of the PHY configuration on the TRX. Can be anything in the range 1, 15 inclusive
#define RF_CONFIG_ID                 (1U)
#define DELTA_TABLE_CONFIG_ID        (2U)

// ID of the MDR header and packet data payload on the TRX. Can be anything in the range 0, 15 inclusive
#define MDR_HEADER_STREAM_ID         (0U)
#define MDR_PACKET_STREAM_ID         (1U)

// Slots for the MDR commands on the TRX. Can be anything in the range 0, 5 inclusive
#define CMD_CS_MDR_HEADER_SLOT       (0U)
#define CMD_TX_MDR_HEADER_SLOT       (1U)
#define CMD_CS_MDR_PACKET_SLOT       (2U)
#define CMD_TX_MDR_PACKET_SLOT       (3U)

// Reference of the Front End (FE) configuration on the TRX. Can be any 32-bit value
#define FE_CONFIG_REFERENCE (0xFEFEFEFE)

// Reference of the PHY configuration on the TRX. Can be any 32-bit value
#define RF_CONFIG_REFERENCE (0xF000000D)

// Reference of the frequency delta configuration on the TRX. Can be any 32-bit value
#define DELTA_CONFIG_REFERENCE (0xDE1ADE1A)

// Derived defines (not to be changed)
#define TX_PACKET_LENGTH            (sizeof(TRX_PayloadHeader) + TX_PAYLOAD_LENGTH) // Do not change

// Variable declarations

// Radio commands
typedef struct MDR_CCA_cmds_t
{
    TRX_Request_CommandStore cmdCsMdrHeader;
    TRX_Request_CommandStore cmdTxMdrHeader;
    TRX_Request_CommandStore cmdCsMdrPacket;
    TRX_Request_CommandStore cmdTxMdrPacket;
} MDR_CCA_cmds;

// The following chain is used in this example:
// cmdCsMdrHeader -> cmdTxMdrHeader -> cmdCsMdrPacket -> cmdTxMdrPacket

// TX packets
uint8_t txMdrHeader[sizeof(TRX_PayloadHeader)] = {0, 0, 0, 0};  // TRX_PayloadHeader
                                                                // Init the Header to 0
uint8_t txMdrPacket[TX_PACKET_LENGTH] = {0, 0, 0, 0};           // TRX_PayloadHeader + payload
                                                                // Init the Header to 0

volatile bool resetTrx = false;
volatile bool eraseNv = false;

uint16_t sequenceNumber = 0U;
static uint8_t majorVersion;
static uint8_t minorVersion;

// Debug
uint8_t eventError = 0U;
uint8_t eventCmdDoneCs = 0U;
uint8_t eventLastCmdDoneCs = 0U;
uint8_t eventCmdDoneTx = 0U;
uint8_t eventLastCmdDoneTx = 0U;
uint8_t commandStatus_ChannelIdle = 0U;
uint8_t commandStatus_ChannelBusy = 0U;
uint32_t packetSent = 0U;

SemaphoreP_Handle configSemaphore;
SemaphoreP_Handle versionSemaphore;
SemaphoreP_Handle csTxSemaphore;
SemaphoreP_Handle buttonSemaphore;
SemaphoreP_Handle storeDeviceConfigCompleteSem;

static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);
static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);
static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg);
static void versionCallback(TRX_Host_Handle handle, uintptr_t pNULL,
                           TRX_Request *request, uint64_t events, uintptr_t arg);
static void csTxCallback(TRX_Host_Handle handle, uintptr_t pCmdOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);
static void buttonCallback(uint_least8_t index);
static uint8_t getMdrHeaderIdx(TRX_WisunMdrMappingTable *map);
static uint8_t getMdrPacketIdx(TRX_WisunMdrMappingTable *map);
static int16_t getMdrHeaderChannelPlanId(TRX_WisunMdrMappingTable *map, uint8_t mdrHeaderIdx);
static int16_t getMdrPacketChannelPlanId(TRX_WisunMdrMappingTable *map, uint8_t mdrPacketIdx);
static uint32_t getMdrPacketTxFrequency(int16_t mdrPacketChannelPlanId, int32_t mdrHeaderTxFrequency);
static void setupCmdMdrCsHdr(TRX_Request_CommandStore *pCmdMdrCsHeader, int32_t mdrHeaderTxFrequency);
static void setupCmdMdrTxHdr(TRX_Request_CommandStore *pCmdMdrTxHeader, uint16_t newPhyId);
static void setupCmdMdrCsPacket(TRX_Request_CommandStore *pCmdMdrCsPacket, int32_t mdrPacketTxFrequency);
static void setupCmdMdrTxPacket(TRX_Request_CommandStore *pCmdMdrTxPacket, int32_t mdrPacketTxFrequency);
static TRX_Host_Handle setupTrxForMdrTxWithCca(MDR_CCA_cmds *pCmds);

//------------------------------------------------------------------------------
void *mainThread(void *arg0)
{
    //--------------------------------------------------------------------------
    GPIO_setConfig(CONFIG_GPIO_GLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_RLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    GPIO_write(CONFIG_GPIO_GLED, CONFIG_GPIO_LED_OFF);
    GPIO_write(CONFIG_GPIO_RLED, CONFIG_GPIO_LED_OFF);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Init the semaphores
    SemaphoreP_Params semParamsConfig;
    SemaphoreP_Params_init(&semParamsConfig);
    configSemaphore = SemaphoreP_create(0, &semParamsConfig);

    SemaphoreP_Params semParamsVer;
    SemaphoreP_Params_init(&semParamsVer);
    versionSemaphore = SemaphoreP_create(0, &semParamsVer);

    SemaphoreP_Params semParamsCsTx;
    SemaphoreP_Params_init(&semParamsCsTx);
    csTxSemaphore = SemaphoreP_create(0, &semParamsCsTx);

    SemaphoreP_Params semParamsButton;
    SemaphoreP_Params_init(&semParamsButton);
    buttonSemaphore = SemaphoreP_create(0, &semParamsButton);

    SemaphoreP_Params semParamsStoreDeviceConfig;
    SemaphoreP_Params_init(&semParamsStoreDeviceConfig);
    storeDeviceConfigCompleteSem = SemaphoreP_create(0, &semParamsStoreDeviceConfig);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    GPIO_setConfig(CONFIG_GPIO_BTN1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    // Install Button callback
    GPIO_setCallback(CONFIG_GPIO_BTN1, buttonCallback);

    // Enable interrupts
    GPIO_enableInt(CONFIG_GPIO_BTN1);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    MDR_CCA_cmds cmds;
    TRX_Host_Handle rf_handle = setupTrxForMdrTxWithCca(&cmds);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Infinite loop for transmitting NUMBER_OF_PACKETS when the button is pushed
    while(1)
    {
        SemaphoreP_pend(buttonSemaphore, SemaphoreP_WAIT_FOREVER);

        if(resetTrx)
        {
            resetTrx = false;
            // An error occurred that requires the TRX to be reset
            TRX_Host_close(rf_handle);

            // Setup the TRX again
            rf_handle = setupTrxForMdrTxWithCca(&cmds);
        }
        else if(eraseNv)
        {
            eraseNv = false;
            (void)TRX_Host_eraseNv(rf_handle, NULL);
        }
        else // Button push
        {
            // Create a txPacket
            uint8_t i = 0U;

            for(i = 6U; i < TX_PACKET_LENGTH; i++)
            {
                txMdrPacket[i] = i - 5U;
            }

            sequenceNumber = 0U;
            while (sequenceNumber < NUMBER_OF_PACKETS)
            {
                // Update Seq. Number
                txMdrPacket[4U] = (uint8_t)(sequenceNumber >> 8U);
                txMdrPacket[5U] = (uint8_t)(sequenceNumber++);

                /** Load the stream of data to transmit for the MDR header */
                TRX_Host_Status status = TRX_Host_storeStream(rf_handle, MDR_HEADER_STREAM_ID,
                                                                txMdrHeader, sizeof(txMdrHeader),
                                                                Stream_Retention_Flush_Streaming,
                                                                csTxCallback, Stream_Retention_Flush_Never);
                while(TRX_Host_Success != status);

                /** Load the stream of data to transmit for the MDR packet */
               status = TRX_Host_storeStream(rf_handle, MDR_PACKET_STREAM_ID,
                                             txMdrPacket, sizeof(txMdrPacket),
                                             Stream_Retention_Flush_Streaming,
                                             csTxCallback, TRX_EventStreamStoreComplete);
                while(TRX_Host_Success != status);

                /** Submit the command chain to the radio (MDR Header Cs is the first command) */
                status = TRX_Host_submitCmd(rf_handle, CMD_CS_MDR_HEADER_SLOT);
                while(TRX_Host_Success != status);

                SemaphoreP_pend(csTxSemaphore, SemaphoreP_WAIT_FOREVER);
                GPIO_toggle(CONFIG_GPIO_GLED);
                usleep(PACKET_INTERVAL);
            }
        }
    }
    //--------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
static TRX_Host_Handle setupTrxForMdrTxWithCca(MDR_CCA_cmds *pCmds)
{
    //--------------------------------------------------------------------------
    TRX_Host_Params params = {
        .bitRate = PLATFORM_DEFAULT_SPI_BITRATE,
        .generalCb = generalCallback,
        .arg = (uintptr_t)NULL
    };

    // Init the RF driver and load the configuration
    // Reset TRX by toggling the reset pin
    GPIO_setConfig(TRX_RESET, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH);
    GPIO_write(TRX_RESET, 0);
    usleep(100); // keep reset low for 100us (empirically found)
    GPIO_write(TRX_RESET, 1);
    usleep(100); // wait for 100us after releasing reset

    TRX_Host_Handle rf_handle = TRX_Host_open(&params);

    TRX_Host_Status status;

    // Store the device configuration (DIOs, clock, power mode, RF mode)
    TRX_DeviceConfigData deviceConfig = {
        .dioConfig = {
            .dio0 = DIO0_UNCHANGED,
            .dio2 = DIO2_HIGH_PA,        // Must not be changed when using the boosterpack
            .dio3 = DIO3_LOW_PA_AND_LNA, // Must not be changed when using the boosterpack
            .dio4 = DIO4_UNCHANGED,
            .dio5 = DIO5_UNCHANGED,
            .dio6 = DIO6_UNCHANGED,
            .dio7 = DIO7_UNCHANGED,
        },
        .clockConfig = TRX_ClockConfig_XOSC,
        .powerMode = {
            .dwellTimeUs = 0U,
            .powerPolicy = (TRX_PowerPolicy)0,
        },
        .rfMode = TRX_RfMode_SUN,
    };
    status = TRX_Host_storeDeviceConfig(rf_handle, deviceConfig, storeDeviceConfigCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(storeDeviceConfigCompleteSem, SemaphoreP_WAIT_FOREVER);

    //erase NV
    status = TRX_Host_eraseNv(rf_handle, configCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configSemaphore, SemaphoreP_WAIT_FOREVER);

    // Load the configuration
    status = TRX_Host_storeConfig(rf_handle, RF_CONFIG_ID, RF_CONFIG_PTR, RF_CONFIG_SIZE,
                                  RF_CONFIG_REFERENCE, true, configCallback, TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configSemaphore, SemaphoreP_WAIT_FOREVER);

    status = TRX_Host_persistConfig(rf_handle, RF_CONFIG_ID);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configSemaphore, SemaphoreP_WAIT_FOREVER);

    // Load the PA table
    status = TRX_Host_storeConfig(rf_handle, TRX_CONFIG_ID_FRONTEND, FE_CONFIG_PTR, FE_CONFIG_SIZE,
                                  FE_CONFIG_REFERENCE, true, configCallback, TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configSemaphore, SemaphoreP_WAIT_FOREVER);
#ifdef TRX_CC1407
    status = TRX_Host_persistConfig(rf_handle, TRX_CONFIG_ID_FRONTEND);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configSemaphore, SemaphoreP_WAIT_FOREVER);
#endif
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Check for TRX Firmware version
    status = TRX_Host_utilGetVersion(rf_handle, versionCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(versionSemaphore, SemaphoreP_WAIT_FOREVER);

    //--------------------------------------------------------------------------
    // Get a pointer to the mapping table (part of LRF_mainRegConfig_wisun)
    uint32_t *pData = (uint32_t *)RF_CONFIG_PTR;
    TRX_WisunMdrMappingTable *map = (TRX_WisunMdrMappingTable *)&(pData[15]);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // CONFIG for MDR HEADER
    uint8_t mdrHeaderIdx = getMdrHeaderIdx(map);
    int16_t mdrHeaderChannelPlanId = getMdrHeaderChannelPlanId(map, mdrHeaderIdx);
    int32_t mdrHeaderTxFrequency = get_wisun_frequency_from_channel_JP(mdrHeaderChannelPlanId, MDR_HEADER_CHANNEL);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // CONFIG for MDR PACKET
    uint8_t mdrPacketIdx = getMdrPacketIdx(map);
    int16_t mdrPacketChannelPlanId = getMdrPacketChannelPlanId(map, mdrPacketIdx);
    uint32_t mdrPacketTxFrequency = getMdrPacketTxFrequency(mdrPacketChannelPlanId, mdrHeaderTxFrequency);
    int32_t mdrPacketDeltaTxFrequency = (int32_t)(mdrPacketTxFrequency - mdrHeaderTxFrequency); // This the frequency of the MDR packet is
                                                                                                // given as a delta from the mdrHeaderTxFrequency to avoid
                                                                                                // having to calibrate
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Load the new delta configuration
    const uint32_t *newDeltaConfig = get_wisun_delta_table_JP(mdrHeaderChannelPlanId);
    uint16_t deltaTableLength = (((uint16_t)(newDeltaConfig[0] & 0x0FFF)) * sizeof(uint32_t)) + sizeof(uint32_t);

    status = TRX_Host_storeConfig(rf_handle, DELTA_TABLE_CONFIG_ID, (uint8_t *)newDeltaConfig, deltaTableLength,
                                  DELTA_CONFIG_REFERENCE, true, configCallback, TRX_EventConfigStoreComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configSemaphore, SemaphoreP_WAIT_FOREVER);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // CMD setup
    uint16_t newPhyId = map->mappingTable[mdrPacketIdx].mdrByte;
    memset((void *)pCmds, 0, sizeof(MDR_CCA_cmds));
    setupCmdMdrCsHdr(&(pCmds->cmdCsMdrHeader), mdrHeaderTxFrequency);
    setupCmdMdrTxHdr(&(pCmds->cmdTxMdrHeader), newPhyId);
    setupCmdMdrCsPacket(&(pCmds->cmdCsMdrPacket), mdrPacketDeltaTxFrequency);
    setupCmdMdrTxPacket(&(pCmds->cmdTxMdrPacket), mdrPacketDeltaTxFrequency);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // STORE cmds on TRX
    uint64_t subscribedCmdEvents = TRX_EventCmdStatus | TRX_EventFinalCmdStatus |
                                   TRX_EventCmdStoreComplete | TRX_EventCmdSubmitComplete |
                                   TRX_EventLastStatusError;
    status = TRX_Host_storeCmds(rf_handle, (TRX_Request_CommandStore *)pCmds, 4U, NULL, csTxCallback, subscribedCmdEvents);
    while(TRX_Host_Success != status);
    //--------------------------------------------------------------------------

    return(rf_handle);
}

static uint8_t getMdrHeaderIdx(TRX_WisunMdrMappingTable *map)
{
    // Find the PHY index in the mapping table
    uint8_t mdrHeaderIdx;
    for(mdrHeaderIdx = 0; mdrHeaderIdx < map->numEntries; mdrHeaderIdx++)
    {
        if(map->mappingTable[mdrHeaderIdx].modem == MDR_HEADER_MODEM)
        {
            if(map->mappingTable[mdrHeaderIdx].optionMask == MDR_HEADER_OPTION_MASK)
            {
                break;
            }
        }
    }
    return(mdrHeaderIdx);
}

static uint8_t getMdrPacketIdx(TRX_WisunMdrMappingTable *map)
{
    // Find the PHY index in the mapping table (part of LRF_mainRegConfig_wisun)
    uint8_t mdrPacketIdx;
    for(mdrPacketIdx = 0U; mdrPacketIdx < map->numEntries; mdrPacketIdx++)
    {
        if(map->mappingTable[mdrPacketIdx].modem == MDR_PACKET_MODEM)
        {
            if(map->mappingTable[mdrPacketIdx].optionMask == MDR_PACKET_OPTION_MASK)
            {
                break;
            }
        }
    }
    return(mdrPacketIdx);
}

static int16_t getMdrHeaderChannelPlanId(TRX_WisunMdrMappingTable *map, uint8_t mdrHeaderIdx)
{
    uint8_t mdrHeaderChannelPlanIdIdx = map->mappingTable[mdrHeaderIdx].channelPlanIdIdx;
    int16_t mdrHeaderChannelPlanId = get_wisun_channelPlanId_from_channelPlanIdIdx_JP(mdrHeaderChannelPlanIdIdx);
    return(mdrHeaderChannelPlanId);
}

static int16_t getMdrPacketChannelPlanId(TRX_WisunMdrMappingTable *map, uint8_t mdrPacketIdx)
{
    uint8_t mdrPacketChannelPlanIdIdx = map->mappingTable[mdrPacketIdx].channelPlanIdIdx;
    int16_t mdrPacketChannelPlanId = get_wisun_channelPlanId_from_channelPlanIdIdx_JP(mdrPacketChannelPlanIdIdx);
    return(mdrPacketChannelPlanId);
}

static uint32_t getMdrPacketTxFrequency(int16_t mdrPacketChannelPlanId, int32_t mdrHeaderTxFrequency)
{
    // Find min. and max. channel number to be able to find the mdrPacketTxFrequency
    uint16_t minMdrPacketChan;
    uint16_t maxMdrPacketChan;

    if (mdrPacketChannelPlanId == 21)
    {
        minMdrPacketChan = 9U;  // WiSUN MDR Spec
        maxMdrPacketChan = 37U; // WiSUN MDR Spec
    }
    if (mdrPacketChannelPlanId == 22)
    {
        minMdrPacketChan = 4U;  // WiSUN MDR Spec
        maxMdrPacketChan = 17U; // WiSUN MDR Spec
    }
    if (mdrPacketChannelPlanId == 23)
    {
        minMdrPacketChan = 3U;  // WiSUN MDR Spec
        maxMdrPacketChan = 11U; // WiSUN MDR Spec
    }
    if (mdrPacketChannelPlanId == 24)
    {
        minMdrPacketChan = 2U;  // WiSUN MDR Spec
        maxMdrPacketChan = 8U;  // WiSUN MDR Spec
    }

    uint32_t newFreq = WISUN_FREQ_INVALID;
    for(int16_t i = minMdrPacketChan; i <= maxMdrPacketChan; i++)
    {
       newFreq = get_wisun_frequency_from_channel_JP(mdrPacketChannelPlanId, i);
        if((mdrHeaderTxFrequency < newFreq) && (WISUN_FREQ_INVALID != newFreq))
        {
            uint32_t prevFreq = get_wisun_frequency_from_channel_JP(mdrPacketChannelPlanId, i-1);
            if((newFreq - mdrHeaderTxFrequency) > (mdrHeaderTxFrequency - prevFreq))
            {
                newFreq = prevFreq;
            }
            break;
        }
    }

    return(newFreq);
}

static void setupCmdMdrCsHdr(TRX_Request_CommandStore *pCmdMdrCsHeader, int32_t mdrHeaderTxFrequency)
{
    //set it all to zeros
    memset(pCmdMdrCsHeader, 0, sizeof(TRX_Request_CommandStore));

    //--------------------------------------------------------------------------
    // Create the CS command for the MDR Header
    pCmdMdrCsHeader->slot = CMD_CS_MDR_HEADER_SLOT;
    pCmdMdrCsHeader->cmd_id = TRX_RadioCommand_CarrierSense;
    pCmdMdrCsHeader->slot_on_false = CMD_TX_MDR_HEADER_SLOT; // Chain to the MDR Header TX on Channel Idle
    pCmdMdrCsHeader->enable_on_false = true;                 // Chain to the MDR Header TX on Channel Idle
    pCmdMdrCsHeader->enable_on_true = false;
    pCmdMdrCsHeader->enable_on_compare = false;
    pCmdMdrCsHeader->trigger = Command_Trigger_Immediate;
    pCmdMdrCsHeader->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdMdrCsHeader->allow_delay = false;
    pCmdMdrCsHeader->params.cs.modem = MDR_HEADER_MODEM;
    pCmdMdrCsHeader->params.cs.frequency = mdrHeaderTxFrequency;
    pCmdMdrCsHeader->params.cs.phy0.config_id = RF_CONFIG_ID;
    pCmdMdrCsHeader->params.cs.phy0.option_mask = MDR_HEADER_OPTION_MASK;
    pCmdMdrCsHeader->params.cs.phy1.config_id = 0U;
    pCmdMdrCsHeader->params.cs.phy1.option_mask = 0U;
    pCmdMdrCsHeader->params.cs.phy2.config_id = 0U;
    pCmdMdrCsHeader->params.cs.phy2.option_mask = 0U;
    pCmdMdrCsHeader->params.cs.mode = TRX_CarrierSense_Mode_Energy; // Channel busy if energy above threshold
    pCmdMdrCsHeader->params.cs.rssi_override = RSSI_THRESHOLD;
    pCmdMdrCsHeader->params.cs.rssi_window = 5U;
    pCmdMdrCsHeader->params.cs.rssi_count = 5U;
    pCmdMdrCsHeader->params.cs.exit_condition = TRX_CarrierSense_ExitCondition_WaitForBusy; // Finish on channel busy or timeout

    // Time, in microseconds, after which CS command is finished regardless of results.
    // If this is not long enough, the command will return IDLE at timeout even if the RSSI is not valid yet.
    // The time it takes for CS to be valid varies with the different PHYs
    // This example uses the same timeouts as rfDiagnostics

    if(pCmdMdrCsHeader->params.cs.phy0.option_mask == TRX_PHY_FEATURE_FSK_MODE_2B_WISUN)
    {
        pCmdMdrCsHeader->params.cs.timeout = 276U; // 51.3 + 96 + 128 = 275.3 // Same as rfDiagnostics
    }
    else // TRX_PHY_FEATURE_FSK_MODE_4B_WISUN
    {
        pCmdMdrCsHeader->params.cs.timeout  = 217U; // 40.2 + 48 + 128 = 216.2 // Same as rfDiagnostics
    }
}

static void setupCmdMdrTxHdr(TRX_Request_CommandStore *pCmdMdrTxHeader, uint16_t newPhyId)
{

    //set it all to zeros
    memset(pCmdMdrTxHeader, 0, sizeof(TRX_Request_CommandStore));

    //--------------------------------------------------------------------------
    // Create the TX command for the MDR Header (only supported on an FSK PHY)
    pCmdMdrTxHeader->slot = CMD_TX_MDR_HEADER_SLOT;
    pCmdMdrTxHeader->cmd_id = TRX_RadioCommand_Transmit;
    pCmdMdrTxHeader->enable_on_true = true; // Run next CS command when first TX has finished
    pCmdMdrTxHeader->slot_on_true = CMD_CS_MDR_PACKET_SLOT;
    pCmdMdrTxHeader->enable_on_false = false;
    pCmdMdrTxHeader->enable_on_compare = false;
    pCmdMdrTxHeader->chain_trigger = Command_Trigger_Immediate;
    pCmdMdrTxHeader->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdMdrTxHeader->allow_delay = false;
    pCmdMdrTxHeader->params.tx.stream_id = MDR_HEADER_STREAM_ID;
    pCmdMdrTxHeader->params.tx.phy0.config_id = RF_CONFIG_ID;
    pCmdMdrTxHeader->params.tx.phy0.option_mask = MDR_HEADER_OPTION_MASK;
    pCmdMdrTxHeader->params.tx.phy1.config_id = 0U;
    pCmdMdrTxHeader->params.tx.phy1.option_mask = 0U;
    pCmdMdrTxHeader->params.tx.phy2.config_id = 0U;
    pCmdMdrTxHeader->params.tx.phy2.option_mask = 0U;
    pCmdMdrTxHeader->params.tx.pa = TRX_TxPa_High;
    pCmdMdrTxHeader->params.tx.frequency = 0U; // Set this to 0 as this will make it equal to the mdrHeaderTxFrequency
                                               // but without having to calibrate;
    pCmdMdrTxHeader->params.tx.modem = MDR_HEADER_MODEM;
    pCmdMdrTxHeader->params.tx.power.dBm = 10;
    pCmdMdrTxHeader->params.tx.power.fraction = 0;

    ((TRX_PayloadHeader_SunFSK *)txMdrHeader)->length = 0U;                                     // No Payload
    ((TRX_PayloadHeader_SunFSK *)txMdrHeader)->modulation = TRX_PayloadHeader_Modulation_FSK;   // 0U = FSK
    ((TRX_PayloadHeader_SunFSK *)txMdrHeader)->mode_switch = 1U;                                // 1U = mode switch enabled
    ((TRX_PayloadHeader_SunFSK *)txMdrHeader)->fcs_mode = 0U;                                   // 0U = CRC32; 1U = CRC16
    ((TRX_PayloadHeader_SunFSK *)txMdrHeader)->whitening = 1U;                                  // 1U = Whitening enabled
    ((TRX_PayloadHeader_SunFSK *)txMdrHeader)->newPhyId = (newPhyId | MDR_PACKET_RATE);
}

static void setupCmdMdrCsPacket(TRX_Request_CommandStore *pCmdMdrCsPacket, int32_t mdrPacketTxFrequency)
{

    //set it all to zeros
    memset(pCmdMdrCsPacket, 0, sizeof(TRX_Request_CommandStore));

    //--------------------------------------------------------------------------
    // Create the CS command for the MDR Header
    pCmdMdrCsPacket->slot = CMD_CS_MDR_PACKET_SLOT;
    pCmdMdrCsPacket->cmd_id = TRX_RadioCommand_CarrierSense;
    pCmdMdrCsPacket->slot_on_false = CMD_TX_MDR_PACKET_SLOT; // Chain to the MDR Packet TX on Channel Idle
    pCmdMdrCsPacket->enable_on_false = true;                 // Chain to the MDR Packet TX on Channel Idle
    pCmdMdrCsPacket->enable_on_true = false;
    pCmdMdrCsPacket->enable_on_compare = false;
    pCmdMdrCsPacket->chain_trigger = Command_Trigger_Immediate;
    pCmdMdrCsPacket->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdMdrCsPacket->allow_delay = false;
    pCmdMdrCsPacket->params.cs.modem = MDR_PACKET_MODEM;
    pCmdMdrCsPacket->params.cs.frequency = mdrPacketTxFrequency;
    pCmdMdrCsPacket->params.cs.phy0.config_id = RF_CONFIG_ID;
    pCmdMdrCsPacket->params.cs.phy0.option_mask = MDR_PACKET_OPTION_MASK;
    pCmdMdrCsPacket->params.cs.phy1.config_id = 0U;
    pCmdMdrCsPacket->params.cs.phy1.option_mask = 0U;
    pCmdMdrCsPacket->params.cs.phy2.config_id = 0U;
    pCmdMdrCsPacket->params.cs.phy2.option_mask = 0U;
    pCmdMdrCsPacket->params.cs.mode = TRX_CarrierSense_Mode_Energy; // Channel busy if energy above threshold
    pCmdMdrCsPacket->params.cs.rssi_override = RSSI_THRESHOLD;
    pCmdMdrCsPacket->params.cs.corr_window = 0U;
    pCmdMdrCsPacket->params.cs.corr_count = 0U;
    pCmdMdrCsPacket->params.cs.exit_condition = TRX_CarrierSense_ExitCondition_WaitForBusy; // Finish on channel busy or timeout


    if(pCmdMdrCsPacket->params.cs.modem == TRX_RadioCommand_Modem_FSK)
    {
        pCmdMdrCsPacket->params.cs.rssi_window = 5U;
        pCmdMdrCsPacket->params.cs.rssi_count = 5U;


        if(pCmdMdrCsPacket->params.cs.phy0.option_mask == TRX_PHY_FEATURE_FSK_MODE_2B_WISUN)
        {
            pCmdMdrCsPacket->params.cs.timeout = 276U; 	// 51.3 + 96 + 128 = 275.3 // Same as rfDiagnostics
        }
        else // TRX_PHY_FEATURE_FSK_MODE_4B_WISUN
        {
            pCmdMdrCsPacket->params.cs.timeout = 217U;	// 40.2 + 48 + 128 = 216.2 // Same as rfDiagnostics
        }
    }
    else // TRX_RadioCommand_Modem_OFDM
    {
        pCmdMdrCsPacket->params.cs.rssi_window = 3U;
        pCmdMdrCsPacket->params.cs.rssi_count = 3U;
        pCmdMdrCsPacket->params.cs.timeout = 193U;	// Same as rfDiagnostics

    }
}

static void setupCmdMdrTxPacket(TRX_Request_CommandStore *pCmdMdrTxPacket, int32_t mdrPacketTxFrequency)
{

    //set it all to zeros
    memset(pCmdMdrTxPacket, 0, sizeof(TRX_Request_CommandStore));

    //--------------------------------------------------------------------------
    // Create the TX command for the MDR Packet
    pCmdMdrTxPacket->slot = CMD_TX_MDR_PACKET_SLOT;
    pCmdMdrTxPacket->cmd_id = TRX_RadioCommand_Transmit;
    pCmdMdrTxPacket->enable_on_true = false;
    pCmdMdrTxPacket->enable_on_false = false;
    pCmdMdrTxPacket->enable_on_compare = false;
    pCmdMdrTxPacket->chain_trigger = Command_Trigger_Immediate;
    pCmdMdrTxPacket->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdMdrTxPacket->allow_delay = false;
    pCmdMdrTxPacket->params.tx.stream_id = MDR_PACKET_STREAM_ID;
    pCmdMdrTxPacket->params.tx.phy0.config_id = RF_CONFIG_ID;
    pCmdMdrTxPacket->params.tx.phy0.option_mask = MDR_PACKET_OPTION_MASK;
    pCmdMdrTxPacket->params.tx.phy1.config_id = 0U;
    pCmdMdrTxPacket->params.tx.phy1.option_mask = 0U;
    pCmdMdrTxPacket->params.tx.phy2.config_id = 0U;
    pCmdMdrTxPacket->params.tx.phy2.option_mask = 0U;
    pCmdMdrTxPacket->params.tx.pa = TRX_TxPa_High;
    pCmdMdrTxPacket->params.tx.frequency = mdrPacketTxFrequency;
    pCmdMdrTxPacket->params.tx.modem = MDR_PACKET_MODEM;
    pCmdMdrTxPacket->params.tx.power.dBm = 10;
    pCmdMdrTxPacket->params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm

    // Update the PHY header of the MDR Packet
    if(pCmdMdrTxPacket->params.tx.modem == TRX_RadioCommand_Modem_FSK)
    {
        ((TRX_PayloadHeader_SunFSK *)txMdrPacket)->length = TX_PAYLOAD_LENGTH; // This length excludes the header
        ((TRX_PayloadHeader_SunFSK *)txMdrPacket)->modulation = TRX_PayloadHeader_Modulation_FSK;
        ((TRX_PayloadHeader_SunFSK *)txMdrPacket)->mode_switch = 0U;           // 0U = No mode switch
        ((TRX_PayloadHeader_SunFSK *)txMdrPacket)->fcs_mode = 0U;              // 0U = CRC32; 1U = CRC16
        ((TRX_PayloadHeader_SunFSK *)txMdrPacket)->whitening = 1U;             // 1U = Whitening enabled
        ((TRX_PayloadHeader_SunFSK *)txMdrPacket)->newPhyId = 0U;
    }
    else if(pCmdMdrTxPacket->params.tx.modem == TRX_RadioCommand_Modem_OFDM)
    {
        ((TRX_PayloadHeader_SunOFDM *)txMdrPacket)->length = TX_PAYLOAD_LENGTH;    // This length excludes the header
        ((TRX_PayloadHeader_SunOFDM *)txMdrPacket)->modulation = TRX_PayloadHeader_Modulation_OFDM;
        ((TRX_PayloadHeader_SunOFDM *)txMdrPacket)->rate = MDR_PACKET_RATE;
        ((TRX_PayloadHeader_SunOFDM *)txMdrPacket)->scrambler = 0U;
        ((TRX_PayloadHeader_SunOFDM *)txMdrPacket)->newPhyId = 0U;
    }
    else
    {
        // Unsupported modem
        while(1);
    }
}

//------------------------------------------------------------------------------
static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* If this loops, something went wrong when a TRX_Request_DeviceConfigStore
     * was sent to the TRX */
    while(TRX_EventLastStatusError & events);

    if(TRX_EventDeviceConfigStoreComplete & events)
    {
        SemaphoreP_post(storeDeviceConfigCompleteSem);
    }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    if(events & TRX_EventNvCorrupt)
    {
        eraseNv = true;
    }
    else
    {
        // TRX_EventTransportError
        resetTrx = true;
    }
    SemaphoreP_post(buttonSemaphore);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    // If this loops, something went wrong when a TRX_Request_ConfigStore was sent to the TRX
    while(TRX_EventLastStatusError & events);

    if(TRX_EventNvEraseComplete & events)
    {
        SemaphoreP_post(configSemaphore);
    }

    if(TRX_EventConfigStoreComplete & events)
    {
        // Would typically post a semaphore being pended on elsewhere
        SemaphoreP_post(configSemaphore);
    }

    if(TRX_EventConfigPersistComplete & events)
    {
        SemaphoreP_post(configSemaphore);
    }

    // If status needs to be checked:
    // TRX_Request_LastStatus *pReqLastStatus = (TRX_Request_LastStatus *)request;
    // TRX_CommandStatus cmdStatus = pReqLastStatus->status;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void versionCallback(TRX_Host_Handle handle, uintptr_t pNULL,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    // If this loops, something went wrong when a TRX_Request_UtilInfoGetVersion was sent to the TRX
    while(TRX_EventLastStatusError & events);

    if (TRX_EventGetVersionComplete & events)
    {
        TRX_Request_LastStatus_Param_Version* version = (TRX_Request_LastStatus_Param_Version*)arg;
        majorVersion = version->major;
        minorVersion = version->minor;
        SemaphoreP_post(versionSemaphore);
    }

    // If status needs to be checked:
    // TRX_Request_LastStatus *pReqLastStatus = (TRX_Request_LastStatus *)request;
    // TRX_CommandStatus cmdStatus = pReqLastStatus->status;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void buttonCallback(uint_least8_t index)
{
    //Simple debounce logic, only toggle if the button is still pushed (low)
    CPUdelay((uint32_t)((48000000/3)*0.050f));
    if (!GPIO_read(index))
    {
        SemaphoreP_post(buttonSemaphore);
    }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void csTxCallback(TRX_Host_Handle handle, uintptr_t pCmdOrData,
                         TRX_Request *request, uint64_t events, uintptr_t arg)
{
    if (TRX_EventLastStatusError & events)
    {
        // If status needs to be checked:
        // TRX_Request_LastStatus *pReqLastStatus = (TRX_Request_LastStatus *)request;
        // TRX_CommandStatus cmdStatus = pReqLastStatus->status;
        eventError++;
        SemaphoreP_post(csTxSemaphore);
    }

    if (TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        //--------------------------------------------------------------------------
        // CS command
        //--------------------------------------------------------------------------
        if ((CMD_CS_MDR_HEADER_SLOT == pCmdStatusRequest->slot) || (CMD_CS_MDR_PACKET_SLOT == pCmdStatusRequest->slot))
        {
            eventCmdDoneCs++; // Debug

            if (TRX_EventFinalCmdStatus & events) // The CS command will be the last command if there
            {                                        // is an error or if the channel was BUSY
                eventLastCmdDoneCs++; // Debug
                SemaphoreP_post(csTxSemaphore);
            }

            TRX_CommandStatus cmdStatus = pCmdStatusRequest->status;
            switch(cmdStatus)
            {
                case TRX_CommandStatus_ChannelIdle:
                    // CS command has finished with channel idle
                    commandStatus_ChannelIdle++;
                    break;

                case TRX_CommandStatus_ChannelBusy:
                    // CS command has finished with channel busy
                    commandStatus_ChannelBusy++;
                    break;

                default:
                    break;
            }
        }
        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        // TX command
        //--------------------------------------------------------------------------
        else if ((CMD_TX_MDR_HEADER_SLOT == pCmdStatusRequest->slot) || (CMD_TX_MDR_PACKET_SLOT == pCmdStatusRequest->slot))
        {
            packetSent++;
            eventCmdDoneTx++; // Debug

            if (TRX_EventFinalCmdStatus & events)
            {
                // If status needs to be checked:
                // TRX_CommandStatus cmdStatus = pCmdStatusRequest->status;
                eventLastCmdDoneTx++; // Debug
                SemaphoreP_post(csTxSemaphore);
            }
        }
        //--------------------------------------------------------------------------
    }
}
//------------------------------------------------------------------------------
