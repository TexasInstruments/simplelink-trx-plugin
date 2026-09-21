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
#include <stdbool.h>

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

// Application specific defines
#define RX_PAYLOAD_LENGTH       (10U)   // Maximum payload in a receive operation
#define REPEAT_MODE             (1U)    // 1: Enable
                                        // 0: Disable
// Derived defines (not to be changed)
// The data received will contain both timestamp (4B) and RSSI (1B) information
#define RSSI_SIZE_BYTES         (1U)
#define TIMESTAMP_SIZE_BYTES    (4U)
#define RX_PACKET_LENGTH        (sizeof(TRX_PayloadHeader) + RX_PAYLOAD_LENGTH + RSSI_SIZE_BYTES + TIMESTAMP_SIZE_BYTES)

// ID of the PHY configuration on the TRX. Can be anything in the range 1, TRX_MAX_CONFIGURATIONS inclusive
#define RF_CONFIG_ID                 (1U)
#define DELTA_TABLE_CONFIG_ID        (2U)

// ID of the MDR packet data payload on the TRX. Can be anything in the range 0, TRX_MAX_STREAMS inclusive
#define MDR_PACKET_STREAM_ID         (0U)

// Slots for the MDR commands on the TRX. Can be anything in the range 0, TRX_MAX_NUM_COMMANDS inclusive
#define CMD_RX_MDR_PACKET_SLOT       (0U)

// Reference of the Front End (FE) configuration on the TRX. Can be any 32-bit value
#define FE_CONFIG_REFERENCE (0xFEFEFEFE)

// Reference of the PHY configuration on the TRX. Can be any 32-bit value
#define RF_CONFIG_REFERENCE (0xF000000D)

// Reference of the frequency delta configuration on the TRX. Can be any 32-bit value
#define DELTA_CONFIG_REFERENCE (0xDE1ADE1A)

// Receive buffer
uint8_t rxBuffer[RX_PACKET_LENGTH] = {0};

// Buffers for the different part of the packet
uint8_t rxHeader[sizeof(TRX_PayloadHeader)];
uint8_t rxPayload[RX_PAYLOAD_LENGTH];
uint8_t rxRssi[RSSI_SIZE_BYTES];
uint8_t rxTimestamp[TIMESTAMP_SIZE_BYTES];

volatile bool resetTrx = false;
volatile bool eraseNv = false;

// Debug
uint16_t eventRxBufFull = 0U;
uint16_t eventRxEntryDone = 0U;
uint16_t eventCmdDone = 0U;
uint16_t eventLastCmdDone = 0U;
uint16_t eventSyncFrameDetected = 0U; // .enableSFDEvent = true
uint16_t eventError = 0U;

uint8_t commandStatus_RxTimeout = 0U;
uint8_t commandStatus_GracefulStopTimeout = 0U;
uint8_t commandStatus_GracefulStopApi = 0U;
uint8_t commandStatus_GracefulStopScheduling = 0U;
uint8_t commandStatus_HardStopApi = 0U;
uint8_t commandStatus_HardStopScheduling = 0U;

uint32_t packetReceived = 0U;
uint16_t rxTimeout = 0U;
uint16_t iterations = 0U;

uint8_t mdrByte;
TRX_RadioCommand_Modem modem;
uint8_t rate = 0U;
uint16_t optionMask;

SemaphoreP_Handle configSemaphore;
SemaphoreP_Handle rxSemaphore;
SemaphoreP_Handle initSemaphore;

static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);
static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg); // Used for both loading config and loading the PA TABLE
static void rxCallback(TRX_Host_Handle handle, uintptr_t pCmdOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);
static void sfdCallback(uint_least8_t index);
void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg);
static uint8_t getMdrHeaderIdx(TRX_WisunMdrMappingTable *map);
static int16_t getMdrHeaderChannelPlanId(TRX_WisunMdrMappingTable *map, uint8_t mdrHeaderIdx);
static void setupCmdMdrRxHdr(TRX_Request_CommandStore *pCmdMdrRxHeader, int32_t mdrHeaderRxFrequency, int16_t baseIdx);
static TRX_Host_Handle setupTrxForMdrRx(TRX_Request_CommandStore *pCmdMdrRxHeader);

//------------------------------------------------------------------------------
void *mainThread(void *arg0)
{
    //--------------------------------------------------------------------------
    // Configure the LED pins and turn it off
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

    SemaphoreP_Params semParamsRx;
    SemaphoreP_Params_init(&semParamsRx);
    rxSemaphore = SemaphoreP_create(0, &semParamsRx);

    SemaphoreP_Params semParamsInit;
    SemaphoreP_Params_init(&semParamsInit);
    initSemaphore = SemaphoreP_create(0, &semParamsInit);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Init the RF driver and load the configuration
    TRX_Request_CommandStore cmdMdrRxHeader;
    TRX_Host_Handle rf_handle = setupTrxForMdrRx(&cmdMdrRxHeader);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Infinite loop for receiving
    while(true)
    {
        if(resetTrx)
        {
            resetTrx = false;
            // An error occurred that requires the TRX to be reset
            TRX_Host_close(rf_handle);

            // Setup the TRX again
            rf_handle = setupTrxForMdrRx(&cmdMdrRxHeader);
        }
        else if(eraseNv)
        {
            eraseNv = false;
            (void)TRX_Host_eraseNv(rf_handle, NULL);
        }

        /** Submit the command chain to the radio (MDR Header Tx is the first command) */
        TRX_Host_Status status = TRX_Host_submitCmd(rf_handle, CMD_RX_MDR_PACKET_SLOT);
        while(TRX_Host_Success != status);

        SemaphoreP_pend(rxSemaphore, SemaphoreP_WAIT_FOREVER);

        iterations++;
    }
    //--------------------------------------------------------------------------
}

//------------------------------------------------------------------------------
static TRX_Host_Handle setupTrxForMdrRx(TRX_Request_CommandStore *pCmdMdrRxHeader)
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

    // Set up the device configuration (DIOs, clock, power mode, RF mode)
    TRX_DeviceConfigData deviceConfig = {
        .dioConfig = {
            .dio0 = DIO0_UNCHANGED,
            .dio2 = DIO2_HIGH_PA,
            .dio3 = DIO3_LOW_PA_AND_LNA,
            .dio4 = DIO4_UNCHANGED,
            .dio5 = DIO5_UNCHANGED,
            .dio6 = DIO6_SFD,
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
    SemaphoreP_pend(initSemaphore, SemaphoreP_WAIT_FOREVER);

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
    // SFD pin setup
    GPIO_setConfig(CONFIG_GPIO_SFD, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);

    // Install Sfd callback
    GPIO_setCallback(CONFIG_GPIO_SFD, sfdCallback);

    // Enable interrupts
    GPIO_enableInt(CONFIG_GPIO_SFD);
    //--------------------------------------------------------------------------

    // Get a pointer to the mapping table (part of LRF_mainRegConfig_wisun)
    uint32_t *pData = (uint32_t *)RF_CONFIG_PTR;
    TRX_WisunMdrMappingTable *map = (TRX_WisunMdrMappingTable *)&(pData[15]);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // CONFIG for MDR HEADER
    uint8_t mdrHeaderIdx = getMdrHeaderIdx(map);
    int16_t mdrHeaderChannelPlanId = getMdrHeaderChannelPlanId(map, mdrHeaderIdx);
    int32_t mdrHeaderRxFrequency = get_wisun_frequency_from_channel_JP(mdrHeaderChannelPlanId, MDR_HEADER_CHANNEL);
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
    int16_t baseIdx = get_wisun_delta_table_baseindex_JP(mdrHeaderChannelPlanId, MDR_HEADER_CHANNEL);
    setupCmdMdrRxHdr(pCmdMdrRxHeader, mdrHeaderRxFrequency, baseIdx);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // REGISTER rx stream with host driver
    status = TRX_Host_registerRxStream(rf_handle, MDR_PACKET_STREAM_ID, rxBuffer,
                                       sizeof(rxBuffer), rxCallback,
                                       TRX_EventFinalStreamStoreReceived);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // STORE cmds on TRX
    uint64_t subscribedCmdEvents = TRX_EventCmdStatus | TRX_EventFinalCmdStatus |
                                   TRX_EventCmdStoreComplete | TRX_EventCmdSubmitComplete |
                                   TRX_EventLastStatusError;
    status = TRX_Host_storeCmds(rf_handle, pCmdMdrRxHeader, 1U, NULL, rxCallback, subscribedCmdEvents);
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

static int16_t getMdrHeaderChannelPlanId(TRX_WisunMdrMappingTable *map, uint8_t mdrHeaderIdx)
{
    uint8_t mdrHeaderChannelPlanIdIdx = map->mappingTable[mdrHeaderIdx].channelPlanIdIdx;
    int16_t mdrHeaderChannelPlanId = get_wisun_channelPlanId_from_channelPlanIdIdx_JP(mdrHeaderChannelPlanIdIdx);
    return(mdrHeaderChannelPlanId);
}

static void setupCmdMdrRxHdr(TRX_Request_CommandStore *pCmdMdrRxHeader, int32_t mdrHeaderRxFrequency, int16_t baseIdx)
{
    //set it all to zeros
    memset(pCmdMdrRxHeader, 0, sizeof(TRX_Request_CommandStore));

    //--------------------------------------------------------------------------
    // Create the RX command
    pCmdMdrRxHeader->slot = CMD_RX_MDR_PACKET_SLOT;
    pCmdMdrRxHeader->cmd_id = TRX_RadioCommand_Receive;
    pCmdMdrRxHeader->enable_on_true = false;
    pCmdMdrRxHeader->enable_on_false = false;
    pCmdMdrRxHeader->enable_on_compare = false;
    pCmdMdrRxHeader->trigger = Command_Trigger_Immediate;
    pCmdMdrRxHeader->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdMdrRxHeader->allow_delay = false;
    pCmdMdrRxHeader->params.rx.phy0.config_id = RF_CONFIG_ID;
    pCmdMdrRxHeader->params.rx.phy0.option_mask = MDR_HEADER_OPTION_MASK;
    pCmdMdrRxHeader->params.rx.phy1.config_id = DELTA_TABLE_CONFIG_ID;
    pCmdMdrRxHeader->params.rx.phy1.option_mask = (uint16_t)baseIdx;
    pCmdMdrRxHeader->params.rx.phy2.config_id = 0U;
    pCmdMdrRxHeader->params.rx.phy2.option_mask = 0U;
    pCmdMdrRxHeader->params.rx.stream_id = MDR_PACKET_STREAM_ID;
    pCmdMdrRxHeader->params.rx.frequency = mdrHeaderRxFrequency;
    pCmdMdrRxHeader->params.rx.modem = MDR_HEADER_MODEM;
    pCmdMdrRxHeader->params.rx.enable_mdr = true;
    pCmdMdrRxHeader->params.rx.stream_early = true; // When this is enabled, RX packets will start
                                                    // being streamed over SPI during the reception
                                                    // of the data rather than after the packet has
                                                    // been fully received.
    pCmdMdrRxHeader->params.rx.timeout = 0U;
    pCmdMdrRxHeader->params.rx.repeat = REPEAT_MODE;
    pCmdMdrRxHeader->params.rx.search_strategy = TRX_Rx_SearchStrategy_Sync;
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
    SemaphoreP_post(rxSemaphore);
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
    // RequestStatus requestStatus = pReqLastStatus->status;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void sfdCallback(uint_least8_t index)
{
    eventSyncFrameDetected++; // Debug
    GPIO_toggle(CONFIG_GPIO_RLED);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* If this loops, something went wrong when a TRX_Request_DeviceConfigStore
     * was sent to the TRX */
    while(TRX_EventLastStatusError & events);

    if(TRX_EventDeviceConfigStoreComplete & events)
    {
        SemaphoreP_post(initSemaphore);
    }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void rxCallback(TRX_Host_Handle handle, uintptr_t pCmdOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg)
{
    if (TRX_EventLastStatusError & events)
    {
        // If status needs to be checked:
        // TRX_Request_LastStatus *pReqLastStatus = (TRX_Request_LastStatus *)request;
        // TRX_CommandStatus cmdStatus = pReqLastStatus->status;
        eventError++;
        SemaphoreP_post(rxSemaphore);
    }

    //--------------------------------------------------------------------------
    // Get a pointer to the mapping table (part of LRF_mainRegConfig_wisun)
    uint32_t *pData = (uint32_t *)RF_CONFIG_PTR;
    TRX_WisunMdrMappingTable *map = (TRX_WisunMdrMappingTable *)&(pData[3]);
    //--------------------------------------------------------------------------

    if(TRX_EventFinalStreamStoreReceived & events)
    {
        if (events & TRX_EventStreamStoreOverflow)
        {
            // The RX buffer wasn't big enough
            eventRxBufFull++; // Debug
        }
        else
        {
            // +----------+---------+----------+----------------+
            // | hdr (4B) | payload | rssi(1B) | timestamp (4B) |
            // +----------+---------+----------+----------------+
            if((uintptr_t)NULL != pCmdOrData)
            {
                // Read the packet, obtaining the header information, the payload,
                // the RSSI and the timestamp and copying them into buffers for
                // easy access and reading.
                TRX_PayloadHeader *header   = (TRX_PayloadHeader *)pCmdOrData;
                uint8_t *startOfPayload     = (uint8_t *)header + sizeof(TRX_PayloadHeader);
                uint8_t *endOfPayload       = (uint8_t *)header + header->length + sizeof(TRX_PayloadHeader);
                int8_t *rssi                = (int8_t *)endOfPayload;
                uint32_t *timestamp         = (uint32_t *)(endOfPayload + sizeof(int8_t));

                memcpy(rxHeader,    header,         sizeof(TRX_PayloadHeader));
                memcpy(rxPayload,   startOfPayload, header->length);
                memcpy(rxRssi,      rssi,           RSSI_SIZE_BYTES);
                memcpy(rxTimestamp, timestamp,      TIMESTAMP_SIZE_BYTES);

                packetReceived++;
                GPIO_toggle(CONFIG_GPIO_GLED);
            }

            rate = 0;

            if(((TRX_PayloadHeader_Common *)((TRX_PayloadHeader *)pCmdOrData))->modulation == TRX_PayloadHeader_Modulation_FSK)
            {
                mdrByte = (uint8_t)(((TRX_PayloadHeader_SunFSK *)((TRX_PayloadHeader *)pCmdOrData))->newPhyId);
                modem = TRX_RadioCommand_Modem_FSK;
            }
            else if(((TRX_PayloadHeader_Common *)((TRX_PayloadHeader *)pCmdOrData))->modulation == TRX_PayloadHeader_Modulation_OFDM)
            {
                mdrByte = (uint8_t)(((TRX_PayloadHeader_SunOFDM *)((TRX_PayloadHeader *)pCmdOrData))->newPhyId);
                modem = TRX_RadioCommand_Modem_OFDM;
                rate = (uint8_t)(((TRX_PayloadHeader_SunOFDM *)((TRX_PayloadHeader *)pCmdOrData))->rate);

                // Undo OR of rate into mdrByte
                mdrByte = mdrByte ^ rate;
            }

            if(modem == TRX_RadioCommand_Modem_FSK)
            {
                // Get optionMask to determine if it is FSK2B or FSK4B
                if(mdrByte != 0)
                {
                    uint8_t i;
                    for(i = 0; i < map->numEntries; i++)
                    {
                        if(mdrByte == map->mappingTable[i].mdrByte)
                        {
                            optionMask = map->mappingTable[i].optionMask;
                            // optionMask = 0: FSK2B
                            // optionMask = 1: FSK4B
                            break;
                        }
                    }
                }
            }
            else if(modem == TRX_RadioCommand_Modem_OFDM)
            {
                // Get optionMask to determine if it is OFDMo2, OFMDo3 or OFDMo4
                if(mdrByte != 0)
                {
                    uint8_t i;
                    for(i = 0; i < map->numEntries; i++)
                    {
                        if(mdrByte == map->mappingTable[i].mdrByte)
                        {
                            optionMask = map->mappingTable[i].optionMask;
                            // optionMask = 0: OFDMo2, rate = x: MCSx
                            // optionMask = 2: OFDMo3, rate = x: MCSx
                            // optionMask = 4: OFDMo4, rate = x: MCSx
                            break;
                        }
                    }
                }
            }
        }
    }

    if (TRX_EventFinalCmdStatus & events)
    {
        SemaphoreP_post(rxSemaphore);
        eventLastCmdDone++; // Debug

        // Debugging
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        TRX_CommandStatus cmdStatus = pCmdStatusRequest->status;
        switch(cmdStatus)
        {
            case TRX_CommandStatus_GracefulStopTimeout:
                // Command ended gracefully because the set timeout expired
                commandStatus_GracefulStopTimeout++; // Debug
                break;

            case TRX_CommandStatus_GracefulStopApi:
                // Command ended because graceful stop command was sent
                commandStatus_GracefulStopApi++; // Debug
                break;

            case TRX_CommandStatus_GracefulStopScheduling:
                // Command ended gracefully because another command was scheduled
                commandStatus_GracefulStopScheduling++; // Debug
                break;

            case TRX_CommandStatus_HardStopApi:
                // Command ended because hard stop command was sent
                commandStatus_HardStopApi++; // Debug
                break;

            case TRX_CommandStatus_HardStopScheduling:
                // Command ended hard because another command was scheduled
                commandStatus_HardStopScheduling++; // Debug
                break;

            default:
                  break;
        }
    }

    if (TRX_EventCmdStatus & events)
    {
        // If status needs to be checked:
        // TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        // TRX_CommandStatus cmdStatus = pCmdStatusRequest->status;
        eventCmdDone++; // Debug
    }
}
//------------------------------------------------------------------------------
