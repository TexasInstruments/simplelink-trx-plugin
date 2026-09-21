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

/* Include Files*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>

/* TI Drivers */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

/* Board Header files */
#include "ti_drivers_config.h"

/* Diagnostics specific files */
#include "TestMode.h"
#include "radio/radio.h"
#include "radio/radio_pvt.h"
#include "PhySettings_trx.h"
#include "PhySettings_trx_mdr.h"
#include "radio_trx_mdr.h"

/* TRX RF Header files */
#include <ti/trx/TRX.h>
#include <ti/trx/host/trx_host.h>
#include <ti/trx/host/hal/platform.h>

#ifdef TRX_CC1407
#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_wisun.h"
#define FE_CONFIG_SIZE (LRF_frontendRegConfig_wisun_byteCount)
#define FE_CONFIG_PTR  ((uint8_t *)LRF_frontendRegConfig_wisun)
#endif

/* POSIX */
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>

/* Defines */
/* Max length byte the radio will accept. */
#define RADIO_MAX_LENGTH        (2043U)

/* The data received will contain both timestamp (4B) and RSSI (1B) information. */
#define RSSI_SIZE_BYTES (1U)
#define TIMESTAMP_SIZE_BYTES (4U)

/* Maximum payload in a receive operation */
#define PAYLOAD_LENGTH  (RADIO_MAX_LENGTH + RSSI_SIZE_BYTES + TIMESTAMP_SIZE_BYTES + sizeof(TRX_PayloadHeader_Common))

/* OOB GENFSK PHYS use only 1 byte header for length and not 4 bytes like other PHYs*/
#define TRX_PAYLOADHEADER_LEN_GENFSK (1U)

/* Sequence number that is placed at the head of each Tx packet. */
#define SZ_SEQUENCE_NUM_BYTES (2U)

/* Size of the moving average window. */
#define MOVING_AVG_WINDOW_SIZE (16U)

/* Size of the timestamp window. */
#define TIMESTAMP_WINDOW_SIZE (16U)

/* PER Tx packet transmission interval. */
#define RADIO_PER_TX_INTERVAL_US (100000U)

/* Default frequency. */
#define DEFAULT_FREQ_KHZ    (915000)

/* Maximum Possible Channel. */
#define MAX_CHANNEL    (1000)

/* Default TX Power dBm. */
#define DEFAULT_TX_POWER_DBM    (TRX_MAX_POWER)

/* Invalid RSSI Value. */
#define INVALID_RSSI            (SCHAR_MAX)

#define NUM_RF_CMDS (5U) // Cannot exceed TRX_MAX_NUM_COMMANDS

#define TX_STREAM_ID                 (0U)
#define MDR_TX_STREAM_ID             (1U)
#define RX_STREAM_ID                 (1U)
#define RF_CONFIG_ID                 (1U)
#define MDR_FREQ_DELTA_CONFIG_ID     (2U)

#define BIT_SHIFT_BY_ONE (1)

typedef struct TimestampLog_t
{
    uint32_t log[TIMESTAMP_WINDOW_SIZE];
    int idx;
    int size;
} TimestampLog;

/* Structure defining the radio statistics */
typedef struct RadioStatistics_t
{
    int8_t currRssi;
    int8_t minRssi;
    int8_t maxRssi;
    int32_t avgRssi; // Accumulated then divided by num packets
    char radioVersion[RADIO_VERSION_LENGTH];
    TimestampLog tsLog;
} RadioStatistics;

/* Structure containing an instance of each radio command. */
typedef struct RadioCommands_t
{
    // Total number of cmds here must match NUM_CMDS
    union {
        TRX_Request_CommandStore tx;
        TRX_Request_CommandStore txTest;
    };
    union {
        TRX_Request_CommandStore rx;
        TRX_Request_CommandStore rxTest;
    };
    TRX_Request_CommandStore mdrTx;
    TRX_Request_CommandStore cs;
    TRX_Request_CommandStore mdrCs;

    /** Handle to regular and test Rx commands. */
    uint8_t rxCmdSlot;
    /** Handle to regular and test Tx commands. */
    uint8_t txCmdSlot;
} RadioCommands;

#define Radio_Event_None                 (0)
#define Radio_Event_Tx_Done              (1 << 0)
#define Radio_Event_Rx_Done              (1 << 1)
#define Radio_Event_Rx_Stop              (1 << 2)
#define Radio_Event_RxTest_Done          (1 << 3)
#define Radio_Event_TxTest_Done          (1 << 4)
#define Radio_Event_GetFirmware_Version  (1 << 5)
#define Radio_Event_Cmd_Submitted        (1 << 6)
#define Radio_Event_Config_Submitted     (1 << 7)
#define Radio_Event_GetRssi              (1 << 8)
#define Radio_Event_Error                (1 << 9)
#define Radio_Event_Mdr_Done             (1 << 10)
#define Radio_Event_NV_Erase_Done        (1 << 11)
#define Radio_Event_RFMode_complete      (1 << 12)
#define Radio_Event_Config_Persisted     (1 << 13)

typedef uint32_t Radio_Event;

/* Structure defining the OS primitives used by this radio. */
typedef struct RadioOsPrimitives_t
{
    sem_t radioEvent;
} RadioOsPrimitives;

/* Structure defining the counters tracked by the radio. */
typedef struct RadioCounters_t
{
    uint32_t packetTx;
    uint32_t packetRx;
    uint32_t packetRxNok;
    uint32_t rxSync;
    uint32_t packetRxStopped;
    uint32_t rxTestDone;
    uint32_t txTestDone;
    uint32_t csBusy;
    uint32_t mdrCsBusy;
} RadioCounters;

/* Structure defining all operational aspects of the radio. */
typedef struct Radio_Trx_Object_t
{
    bool isInit;
    bool mdrEnabled;
    PhySettings_Trx_region region;
    uint32_t frequency;
    int8_t txPower;
    uint32_t packetsToTx;
    bool useTxInterval; // If true, set an interval between PER Tx
    uint8_t currentPhyIndex;
    uint8_t currentPhyIndex2;

    RadioStatistics statistics;

    RadioCommands commands;

    Radio_Event evt;

    RadioOsPrimitives osPrimitives;

    RadioCounters counters;
    uint8_t phyReceived;
} Radio_Trx_Object;

/* Function Prototypes */
static void txCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);
static void rxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);
static void rxTestCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);
static void mdrTxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);
static void mdrCsTxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);
static void csTxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg);

static void extractRssiAndTimestamp(uint8_t *payload, int8_t *rssi, uint32_t *timestamp);
static void logRssi(Radio_Trx_Object *radioObject, int8_t rssi);
static void logTimestamp(Radio_Trx_Object *radioObject, uint32_t timestamps);

static void TimestampLog_init(TimestampLog *tsLog);
static void TimestampLog_log(TimestampLog *tsLog, uint32_t ts);

static void configureDefaultRxCmd(Radio_Trx_Object *handle);
static void configureDefaultTxCmd(Radio_Trx_Object *handle);
static void configureDefaultMdrCmds(Radio_Trx_Object *handle);
static void configureDefaultTxTestCmd(Radio_Trx_Object *handle);
static void configureDefaultRxTestCmd(Radio_Trx_Object *handle);
static void resetRadioTrxObject(Radio_Trx_Object *handle);
static void initOsPrimitives(Radio_Trx_Object *handle);
static int composePayload(Radio_Trx_Object *handle, uint8_t *payload, uint32_t dataLength, uint16_t sequenceNumber);
static bool issueTxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb);
static bool issueMdrTxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb);
static bool issueMdrCsTxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb);
static void resetStatistics(Radio_Trx_Object *handle);
static bool issueRxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb);
static bool setChannelPlan(Radio_Trx_Object *handle);
static bool issueRxTestCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb);
static void setMdrFrequencies(uint32_t freq, uint32_t mdrFreq);
static void signalEvent(Radio_Trx_Object *handle, Radio_Event evt);
static Radio_Event waitOnEvent(Radio_Trx_Object *handle);
static void clearPendingEvents(Radio_Trx_Object *handle);

static void clockNanoSleep(int32_t uSec);

/* Globals */

/* Handle to the RF driver object */
TRX_Host_Handle rf_handle;

/* Receive/Transmit payload */
uint8_t txPayload[PAYLOAD_LENGTH] = {0};
uint8_t rxPayload[PAYLOAD_LENGTH] = {0};

uint8_t mdrPayload[sizeof(TRX_PayloadHeader)] = {
                     0, 0, 0, 0, // Reserved for common header
};

/* Global radio object.
 * This is single instance (singleton) that can be directly accessed
 * only in the command callbacks and interface API; all other functions
 * are passed a handle to this object.
 */
Radio_Trx_Object radioObject = {
    .isInit = false,
    .frequency = RADIO_ERROR_VALUE,
    .mdrEnabled = false,
    .region = PhySettings_Trx_Region_JP
};

static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    while(1);
}

static void txCallback(TRX_Host_Handle handle, uintptr_t pCmdStore,
                       TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        if(TRX_CommandStatus_Finished == pCmdStatusRequest->status)
        {
            event |= Radio_Event_Tx_Done;
            GPIO_toggle(CONFIG_GPIO_GLED);
            pRadioObject->counters.packetTx++;
        }
        else
        {
            event |= Radio_Event_Error;
        }
    }

    if(TRX_EventCmdSubmitComplete & events)
    {
        event |= Radio_Event_Cmd_Submitted;
    }

    signalEvent(pRadioObject, event);
}

static void rxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                       TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        TRX_Request_CommandStatus_Params_Rx *pRxParams = (TRX_Request_CommandStatus_Params_Rx *)&(pCmdStatusRequest->params);

        if(TRX_CommandStatus_Finished != pCmdStatusRequest->status)
        {
            // TRX_CommandStatus_GracefulStop || TRX_CommandStatus_HardStop || Failure status
            event |= Radio_Event_Rx_Stop;
            pRadioObject->counters.packetRxStopped++;
            pRadioObject->counters.rxSync = pRxParams->num_sync;
            if(pRadioObject->mdrEnabled)
            {
                pRadioObject->counters.packetRxNok = ((pRxParams->num_header_ok)>>(BIT_SHIFT_BY_ONE)) - pRxParams->num_crc_ok;

            }
            else
            {
                pRadioObject->counters.packetRxNok = pRxParams->num_header_ok - pRxParams->num_crc_ok;
            }
        }
    }

    if(TRX_EventFinalStreamStoreReceived & events)
    {
        if((TRX_EventStreamStoreOverflow & events) == 0)
        {
            event |= Radio_Event_Rx_Done;

            uint8_t *data = (uint8_t *)pCmdStoreOrData;
            if(NULL != data)
            {
                int8_t rssi;
                uint32_t timestamp;
                extractRssiAndTimestamp(data, &rssi, &timestamp);

                if(pRadioObject->mdrEnabled)
                {
                    pRadioObject->phyReceived = extractPhyIndex(pRadioObject->currentPhyIndex, pRadioObject->region, data);
                }
                else
                {
                    pRadioObject->phyReceived = pRadioObject->currentPhyIndex;
                }


                logRssi(pRadioObject, rssi);
                logTimestamp(pRadioObject, timestamp);

                pRadioObject->counters.packetRx++;
                GPIO_toggle(CONFIG_GPIO_RLED);
            }
        }// else RX buffer overflow, ignore the packet
    }

    if(TRX_EventCmdSubmitComplete & events)
    {
        event |= Radio_Event_Cmd_Submitted;
    }

    signalEvent(pRadioObject, event);
}


static void rxTestCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventCmdStatus & events)
    {
        pRadioObject->counters.rxTestDone++;
        event |= Radio_Event_RxTest_Done | Radio_Event_Rx_Stop;
    }

    if(TRX_EventCmdSubmitComplete & events)
    {
        event |= Radio_Event_Cmd_Submitted;
    }
    signalEvent(pRadioObject, event);
}

static void txTestCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventCmdStatus & events)
    {
        pRadioObject->counters.txTestDone++;
        event |= Radio_Event_TxTest_Done;
    }

    if(TRX_EventCmdSubmitComplete & events)
    {
        event |= Radio_Event_Cmd_Submitted;
    }
    signalEvent(pRadioObject, event);
}

static void mdrTxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                          TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStore *pCmdStore = (TRX_Request_CommandStore *)pCmdStoreOrData;
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;

        if(TRX_Request_CommandStatus_ChainAction_None == pCmdStatusRequest->chain_action)
        {
            // Last cmd in the chain
            if(&(pRadioObject->commands.mdrTx) == pCmdStore)
            {
                if(TRX_CommandStatus_Finished == pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Mdr_Done;
                    pRadioObject->counters.packetTx++;
                    GPIO_toggle(CONFIG_GPIO_GLED);
                }
                else
                {
                    event |= Radio_Event_Error;
                }
            }
            else
            {
                // 1st tx failed
                event |= Radio_Event_Error;
            }
        } // else wait for the end of the chain
    }

    if(TRX_EventCmdSubmitComplete & events)
    {
        event |= Radio_Event_Cmd_Submitted;
    }
    signalEvent(pRadioObject, event);
}

static void mdrCsTxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStore *pCmdStore = (TRX_Request_CommandStore *)pCmdStoreOrData;
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        if(TRX_Request_CommandStatus_ChainAction_None == pCmdStatusRequest->chain_action)
        {
            // Last cmd in the chain
            if(&(pRadioObject->commands.mdrTx) == pCmdStore)
            {
                if(TRX_CommandStatus_Finished == pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Mdr_Done;
                    pRadioObject->counters.packetTx++;
                    GPIO_toggle(CONFIG_GPIO_GLED);
                }
                else
                {
                    event |= Radio_Event_Error;
                }
            }
            else if(&(pRadioObject->commands.mdrCs) == pCmdStore)
            {
                if(TRX_CommandStatus_ChannelBusy == pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Mdr_Done;
                    pRadioObject->counters.mdrCsBusy++;
                }
                else if(TRX_CommandStatus_ChannelIdle != pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Error;
                }
            }
            else if(&(pRadioObject->commands.cs) == pCmdStore)
            {
                if(TRX_CommandStatus_ChannelBusy == pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Mdr_Done;
                    pRadioObject->counters.csBusy++;
                }
                else if(TRX_CommandStatus_ChannelIdle != pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Error;
                }
            }
            else
            {
                // 1st tx failed
                event |= Radio_Event_Error;
            }
        } // else wait for the end of the chain
    }

    if(TRX_EventCmdSubmitComplete & events)
    {
        event |= Radio_Event_Cmd_Submitted;
    }
    signalEvent(pRadioObject, event);
}

static void csTxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                         TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStore *pCmdStore = (TRX_Request_CommandStore *)pCmdStoreOrData;
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        if(TRX_Request_CommandStatus_ChainAction_None == pCmdStatusRequest->chain_action)
        {
            // Last cmd in the chain
            if(&(pRadioObject->commands.tx) == pCmdStore)
            {
                if(TRX_CommandStatus_Finished == pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Tx_Done;
                    pRadioObject->counters.packetTx++;
                    GPIO_toggle(CONFIG_GPIO_GLED);
                }
                else
                {
                    event |= Radio_Event_Error;
                }
            }
            else if(&(pRadioObject->commands.cs) == pCmdStore)
            {
                if(TRX_CommandStatus_ChannelBusy == pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Tx_Done;
                    pRadioObject->counters.csBusy++;
                }
                else if(TRX_CommandStatus_ChannelIdle != pCmdStatusRequest->status)
                {
                    event |= Radio_Event_Error;
                }
            }
            else
            {
                // 1st tx failed
                event |= Radio_Event_Error;
            }
        } // else wait for the end of the chain
    }

    if(TRX_EventCmdSubmitComplete & events)
    {
        event |= Radio_Event_Cmd_Submitted;
    }
    signalEvent(pRadioObject, event);
}

static void configSubmittedCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                                    TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;

    if(TRX_EventNvEraseComplete & events)
    {
        event |= Radio_Event_NV_Erase_Done;
    }

    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventConfigStoreComplete & events)
    {
        event |= Radio_Event_Config_Submitted;
    }

    if(TRX_EventConfigPersistComplete & events)
    {
        event |= Radio_Event_Config_Persisted;
    }

    signalEvent(pRadioObject, event);
}

static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* If this loops, something went wrong when a TRX_Host_storeDeviceConfig was
     * sent to the TRX */
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;

    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventDeviceConfigStoreComplete & events)
    {
        event |= Radio_Event_RFMode_complete;
    }

    signalEvent(pRadioObject, event);
}

static void getRssiCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventGetRSSIComplete & events)
    {
        TRX_Request_LastStatus *pReqLastStatus = (TRX_Request_LastStatus *)request;
        int8_t rssi = (int8_t)pReqLastStatus->params[0];
        logRssi(pRadioObject, rssi);
        event |= Radio_Event_GetRssi;
    }
    signalEvent(pRadioObject, event);
}

static void getRadioVersionCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                                    TRX_Request *request, uint64_t events, uintptr_t arg)
{
    Radio_Trx_Object *pRadioObject = (Radio_Trx_Object *)arg;

    Radio_Event event = Radio_Event_None;
    if(TRX_EventLastStatusError & events)
    {
        event |= Radio_Event_Error;
    }

    if(TRX_EventGetVersionComplete & events)
    {
        TRX_Request_LastStatus *pReqLastStatus = (TRX_Request_LastStatus *)request;
        TRX_Request_LastStatus_Param_Version *versionParams = (TRX_Request_LastStatus_Param_Version *)pReqLastStatus->params;
        // Store firmware version in format major.minor.patch.build
        sprintf(pRadioObject->statistics.radioVersion, "%d.%d.%d.%d", versionParams->major, versionParams->minor, \
                versionParams->patch, versionParams->build);
        event |= Radio_Event_GetFirmware_Version;
    }
    signalEvent(pRadioObject, event);
}

static void extractRssiAndTimestamp(uint8_t *payload, int8_t *rssi, uint32_t *timestamp)
{

    if(((radioObject.currentPhyIndex >= PHY_SETTINGS_GENFSK_PHY_INDEX_START) && (radioObject.currentPhyIndex <= PHY_SETTINGS_GENFSK_PHY_INDEX_END)) || //Gen FSK
       ((radioObject.currentPhyIndex >= PHY_SETTINGS_XLR_PHY_INDEX_START) && (radioObject.currentPhyIndex <= PHY_SETTINGS_XLR_PHY_INDEX_END)))         //XLR
    {
        // +-----------------+---------+----------+-----------------------+
        // |        hdr/length (1B) | payload | rssi(1B) | timestamp (4B) |
        // +-----------------+---------+----------+-----------------------+
        uint8_t *header = payload;
        uint8_t *endOfPayload = header + TRX_PAYLOADHEADER_LEN_GENFSK + (*header);
        *rssi = *(int8_t *)endOfPayload;
        *timestamp = *(uint32_t *)(endOfPayload + sizeof(int8_t));
    }
    else
    {
        // +-----------------+---------+----------+----------------+
        // |        hdr (4B) | payload | rssi(1B) | timestamp (4B) |
        // +-----------------+---------+----------+----------------+
        TRX_PayloadHeader *header = (TRX_PayloadHeader *)(payload);
        uint8_t *endOfPayload = (uint8_t *)header + header->length + sizeof(TRX_PayloadHeader);
        *rssi = *(int8_t *)endOfPayload;
        *timestamp = *(uint32_t *)(endOfPayload + sizeof(int8_t));
    }
}

static void logRssi(Radio_Trx_Object *handle, int8_t rssi)
{
    RadioStatistics *stats = &handle->statistics;
    if(RADIO_ERROR_VALUE == rssi)
    {
        // RADIO_ERROR_VALUE is meant for errors from the radio but
        // if we're logging an RSSI it's valid. Artificially increment by one
        rssi += 1;
    }

    stats->currRssi = rssi;
    if (rssi > stats->maxRssi)
    {
        stats->maxRssi = rssi;
    }
    else if (rssi < stats->minRssi)
    {
        stats->minRssi = rssi;
    }
    else
    {
        // Do nothing
    }

    stats->avgRssi += (int32_t)rssi;
}

static void logTimestamp(Radio_Trx_Object *handle, uint32_t timestamps)
{
    TimestampLog_log(&handle->statistics.tsLog, timestamps);
}

static void TimestampLog_init(TimestampLog *tsLog)
{
    memset(tsLog, 0, sizeof(TimestampLog));
    tsLog->size = sizeof(tsLog->log) / sizeof(uint32_t);
}

static void TimestampLog_log(TimestampLog *tsLog, uint32_t ts)
{
    tsLog->log[tsLog->idx] = ts;
    tsLog->idx = (tsLog->idx + 1) % tsLog->size;
}

static uint8_t getCmdSlot(TRX_Request_CommandStore *pCmdArray, uint8_t numCmds, TRX_Request_CommandStore *pCmd)
{
    uint8_t index = 0;
    while (index < numCmds && &(pCmdArray[index]) != pCmd ) ++index;

    return ( index == numCmds ? TRX_RESERVED_ID : index );
}


static uint16_t getrfConfigSize(uint32_t *rfConfig)
{
    uint16_t size = (((uint16_t)(rfConfig[0] & 0x0FFF)) * sizeof(uint32_t)) + sizeof(uint32_t);
    return(size);
}

void Radio_Trx_init(void)
{
    /* Initialization parameters */
    TRX_Host_Params params = {
        .bitRate = PLATFORM_DEFAULT_SPI_BITRATE,
        .generalCb = generalCallback, // TODO
        .arg = (uintptr_t)&radioObject
    };

    TRX_Host_Status status;
    Radio_Event event = Radio_Event_None;

    // Reset TRX by toggling the reset pin
    GPIO_setConfig(TRX_RESET, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH);
    GPIO_write(TRX_RESET, 0);
    usleep(100); // keep reset low for 100us (empirically found)
    GPIO_write(TRX_RESET, 1);
    usleep(100); // wait for 100us after releasing reset

    rf_handle = TRX_Host_open(&params);
    resetRadioTrxObject(&radioObject);
    clearPendingEvents(&radioObject);

    // Erase NV for a clean start
    status = TRX_Host_eraseNv(rf_handle, configSubmittedCallback);
    while(TRX_Host_Success != status);
    while(!(event & Radio_Event_NV_Erase_Done) && !(event & Radio_Event_Error))
    {
        event = waitOnEvent(&radioObject);
    }

    configureDefaultRxCmd(&radioObject);
    configureDefaultTxCmd(&radioObject);
    configureDefaultMdrCmds(&radioObject);
    configureDefaultTxTestCmd(&radioObject);
    configureDefaultRxTestCmd(&radioObject);

    // Load the front end configuration
    status = TRX_Host_storeConfig(rf_handle, TRX_CONFIG_ID_FRONTEND, FE_CONFIG_PTR, FE_CONFIG_SIZE,
                                  0xDEADBEEF, true, configSubmittedCallback, TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);

    while(!(event & Radio_Event_Config_Submitted) && !(event & Radio_Event_Error))
    {
        event = waitOnEvent(&radioObject);
    }
#ifdef TRX_CC1407
    status = TRX_Host_persistConfig(rf_handle, TRX_CONFIG_ID_FRONTEND);
    while(TRX_Host_Success != status);

    while(!(event & Radio_Event_Config_Persisted) && !(event & Radio_Event_Error))
    {
        event = waitOnEvent(&radioObject);
    }
#endif

    if(event & Radio_Event_Error)
    {
        radioObject.isInit = false;
    }
    else
    {
        radioObject.isInit = true;
        GPIO_write(CONFIG_GPIO_RLED, 1);
        GPIO_write(CONFIG_GPIO_GLED, 1);
    }
}

static void resetRadioTrxObject(Radio_Trx_Object *handle)
{
    memset(handle, 0, sizeof(Radio_Trx_Object));

    handle->frequency = DEFAULT_FREQ_KHZ;
    handle->txPower = DEFAULT_TX_POWER_DBM;
    handle->useTxInterval = false;
    handle->mdrEnabled = false;
    handle->currentPhyIndex = RADIO_NO_PHY;
    handle->currentPhyIndex2 = RADIO_NO_PHY;

    resetStatistics(handle);

    initOsPrimitives(handle);
}

static void resetStatistics(Radio_Trx_Object *handle)
{
    handle->statistics = (RadioStatistics){
        .currRssi = 0,
        .minRssi = SCHAR_MAX,
        .maxRssi = SCHAR_MIN,
        .avgRssi = 0,
    };


    TimestampLog_init(&handle->statistics.tsLog);
}

static void initOsPrimitives(Radio_Trx_Object *handle)
{
    int retc = sem_init(&handle->osPrimitives.radioEvent, 0U, 0U);
    while(0 != retc);
}

static void configureDefaultRxCmd(Radio_Trx_Object *handle)
{
    TRX_Request_CommandStore *pCmdRx = &(handle->commands.rx);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCmdRx);

    memset((uint8_t *)pCmdRx, 0, sizeof(TRX_Request_CommandStore));

    pCmdRx->slot = slot;
    pCmdRx->cmd_id = TRX_RadioCommand_Receive;
    pCmdRx->enable_on_true = false;
    pCmdRx->enable_on_false = false;
    pCmdRx->enable_on_compare = false;
    pCmdRx->trigger = Command_Trigger_Immediate;
    pCmdRx->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdRx->allow_delay = false;
    pCmdRx->params.rx.phy0.config_id = 0U;
    pCmdRx->params.rx.phy0.option_mask = 0U;
    pCmdRx->params.rx.phy1.config_id = 0U;
    pCmdRx->params.rx.phy1.option_mask = 0U;
    pCmdRx->params.rx.phy2.config_id = 0U;
    pCmdRx->params.rx.phy2.option_mask = 0U;
    pCmdRx->params.rx.stream_id = 0U;
    pCmdRx->params.rx.frequency = handle->frequency;
    pCmdRx->params.rx.modem = TRX_RadioCommand_Modem_FSK;
    pCmdRx->params.rx.enable_mdr = false;
    pCmdRx->params.rx.stream_early = true;
    pCmdRx->params.rx.timeout = 0U;
    pCmdRx->params.rx.repeat = true;
    pCmdRx->params.rx.search_strategy = TRX_Rx_SearchStrategy_Sync;
}

static void configureDefaultTxCmd(Radio_Trx_Object *handle)
{
    TRX_Request_CommandStore *pCmdTx = &(handle->commands.tx);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCmdTx);

    memset((uint8_t *)pCmdTx, 0, sizeof(TRX_Request_CommandStore));

    pCmdTx->slot = slot;
    pCmdTx->cmd_id = TRX_RadioCommand_Transmit;
    pCmdTx->enable_on_true = false;
    pCmdTx->enable_on_false = false;
    pCmdTx->enable_on_compare = false;
    pCmdTx->trigger = Command_Trigger_Immediate;
    pCmdTx->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdTx->allow_delay = false;
    pCmdTx->params.tx.phy0.config_id = 0U;
    pCmdTx->params.tx.phy0.option_mask = 0U;
    pCmdTx->params.tx.phy1.config_id = 0U;
    pCmdTx->params.tx.phy1.option_mask = 0U;
    pCmdTx->params.tx.phy2.config_id = 0U;
    pCmdTx->params.tx.phy2.option_mask = 0U;
    pCmdTx->params.tx.pa = TRX_TxPa_High;
    pCmdTx->params.tx.stream_id = 0U;
    pCmdTx->params.tx.frequency = handle->frequency;
    pCmdTx->params.tx.modem = TRX_RadioCommand_Modem_FSK;

    if((handle->txPower == TRX_MAX_POWER) || (handle->txPower == TRX_MIN_POWER))
    {
        pCmdTx->params.tx.power.rawValue = handle->txPower;
    }
    else
    {
        pCmdTx->params.tx.power.dBm = handle->txPower;
        pCmdTx->params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
    }
}

static void configureDefaultMdrCmds(Radio_Trx_Object *handle)
{
    // MDR TX Cmd
    TRX_Request_CommandStore *pMdrTx = &(handle->commands.mdrTx);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pMdrTx);

    memset((uint8_t *)pMdrTx, 0, sizeof(TRX_Request_CommandStore));

    pMdrTx->slot = slot;
    pMdrTx->cmd_id = TRX_RadioCommand_Transmit;
    pMdrTx->enable_on_true = false;
    pMdrTx->enable_on_false = false;
    pMdrTx->enable_on_compare = false;
    pMdrTx->trigger = Command_Trigger_Immediate;
    pMdrTx->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pMdrTx->allow_delay = false;
    pMdrTx->params.tx.phy0.config_id = 0U;
    pMdrTx->params.tx.phy0.option_mask = 0U;
    pMdrTx->params.tx.phy1.config_id = 0U;
    pMdrTx->params.tx.phy1.option_mask = 0U;
    pMdrTx->params.tx.phy2.config_id = 0U;
    pMdrTx->params.tx.phy2.option_mask = 0U;
    pMdrTx->params.tx.pa = TRX_TxPa_High;
    pMdrTx->params.tx.stream_id = 0U;
    pMdrTx->params.tx.frequency = handle->frequency;
    pMdrTx->params.tx.modem = TRX_RadioCommand_Modem_FSK;

    if((handle->txPower == TRX_MAX_POWER) || (handle->txPower == TRX_MIN_POWER))
    {
        pMdrTx->params.tx.power.rawValue = handle->txPower;
    }
    else
    {
        pMdrTx->params.tx.power.dBm = handle->txPower;
        pMdrTx->params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
    }

    // CS Cmd
    TRX_Request_CommandStore *pCs = &(handle->commands.cs);
    slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCs);

    memset((uint8_t *)pCs, 0, sizeof(TRX_Request_CommandStore));

    pCs->slot = slot;
    pCs->cmd_id = TRX_RadioCommand_CarrierSense;
    pCs->enable_on_true = false;
    pCs->enable_on_false = false;
    pCs->enable_on_compare = false;
    pCs->trigger = Command_Trigger_Immediate;
    pCs->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCs->allow_delay = false;
    pCs->params.cs.phy0.config_id = 0U;
    pCs->params.cs.phy0.option_mask = 0U;
    pCs->params.cs.phy1.config_id = 0U;
    pCs->params.cs.phy1.option_mask = 0U;
    pCs->params.cs.phy2.config_id = 0U;
    pCs->params.cs.phy2.option_mask = 0U;
    pCs->params.cs.mode = TRX_CarrierSense_Mode_Energy;
    pCs->params.cs.rssi_override = -87;
    pCs->params.cs.rssi_window = 5;
    pCs->params.cs.rssi_count = 5;
    pCs->params.cs.exit_condition = TRX_CarrierSense_ExitCondition_WaitForBusy;
    pCs->params.cs.timeout = 0U;

    // MDR CS Cmd
    TRX_Request_CommandStore *pMdrCs = &(handle->commands.mdrCs);
    slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pMdrCs);

    memset((uint8_t *)pMdrCs, 0, sizeof(TRX_Request_CommandStore));

    pMdrCs->slot = slot;
    pMdrCs->cmd_id = TRX_RadioCommand_CarrierSense;
    pMdrCs->enable_on_true = false;
    pMdrCs->enable_on_false = false;
    pMdrCs->enable_on_compare = false;
    pMdrCs->trigger = Command_Trigger_Immediate;
    pMdrCs->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pMdrCs->allow_delay = false;
    pMdrCs->params.cs.phy0.config_id = 0U;
    pMdrCs->params.cs.phy0.option_mask = 0U;
    pMdrCs->params.cs.phy1.config_id = 0U;
    pMdrCs->params.cs.phy1.option_mask = 0U;
    pMdrCs->params.cs.phy2.config_id = 0U;
    pMdrCs->params.cs.phy2.option_mask = 0U;
    pMdrCs->params.cs.mode = TRX_CarrierSense_Mode_Energy;
    pMdrCs->params.cs.rssi_override = -87;
    pMdrCs->params.cs.rssi_window = 5;
    pMdrCs->params.cs.rssi_count = 5;
    pMdrCs->params.cs.exit_condition = TRX_CarrierSense_ExitCondition_WaitForBusy;
    pMdrCs->params.cs.timeout = 0U;
}

static void configureDefaultTxTestCmd(Radio_Trx_Object *handle)
{
    TRX_Request_CommandStore *pTxTest = &(handle->commands.txTest);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pTxTest);

    memset((uint8_t *)pTxTest, 0, sizeof(TRX_Request_CommandStore));

    pTxTest->slot = slot;
    pTxTest->cmd_id = TRX_RadioCommand_TransmitTest;
    pTxTest->enable_on_true = false;
    pTxTest->enable_on_false = false;
    pTxTest->enable_on_compare = false;
    pTxTest->trigger = Command_Trigger_Immediate;
    pTxTest->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pTxTest->allow_delay = false;
    pTxTest->params.txTest.phy0.config_id = 0U;
    pTxTest->params.txTest.phy0.option_mask = 0U;
    pTxTest->params.txTest.phy1.config_id = 0U;
    pTxTest->params.txTest.phy1.option_mask = 0U;
    pTxTest->params.txTest.phy2.config_id = 0U;
    pTxTest->params.txTest.phy2.option_mask = 0U;
    pTxTest->params.txTest.pa = TRX_TxPa_Low;
    pTxTest->params.txTest.stream_id = 0U;
    pTxTest->params.txTest.frequency = handle->frequency;
    pTxTest->params.txTest.modem = TRX_RadioCommand_Modem_FSK;
    pTxTest->params.txTest.mode = TRX_TxTestMode_Unmodulated;

    if((handle->txPower == TRX_MAX_POWER) || (handle->txPower == TRX_MIN_POWER))
    {
        pTxTest->params.txTest.power.rawValue = handle->txPower;
    }
    else
    {
        pTxTest->params.txTest.power.dBm = handle->txPower;
        pTxTest->params.txTest.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
    }
}


static void configureDefaultRxTestCmd(Radio_Trx_Object *handle)
{
    TRX_Request_CommandStore *pRxTest = &(handle->commands.rxTest);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pRxTest);

    memset((uint8_t *)pRxTest, 0, sizeof(TRX_Request_CommandStore));

    pRxTest->slot = slot;
    pRxTest->cmd_id = TRX_RadioCommand_ReceiveTest;
    pRxTest->enable_on_true = false;
    pRxTest->enable_on_false = false;
    pRxTest->enable_on_compare = false;
    pRxTest->trigger = Command_Trigger_Immediate;
    pRxTest->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pRxTest->allow_delay = false;
    pRxTest->params.rxTest.phy0.config_id = 0U;
    pRxTest->params.rxTest.phy0.option_mask = 0U;
    pRxTest->params.rxTest.phy1.config_id = 0U;
    pRxTest->params.rxTest.phy1.option_mask = 0U;
    pRxTest->params.rxTest.phy2.config_id = 0U;
    pRxTest->params.rxTest.phy2.option_mask = 0U;
    pRxTest->params.rxTest.stream_id = 0U;
    pRxTest->params.rxTest.frequency = handle->frequency;
    pRxTest->params.rxTest.modem = TRX_RadioCommand_Modem_FSK;
    pRxTest->params.rxTest.enable_mdr = false;
    pRxTest->params.rxTest.stream_early = true;
    pRxTest->params.rxTest.timeout = 0U;
    pRxTest->params.rxTest.repeat = true;
    pRxTest->params.rxTest.search_strategy = TRX_Rx_SearchStrategy_Sync;
    pRxTest->params.rxTest.test_nosync = true;
    pRxTest->params.rxTest.test_discard_packets  = true;
}

void Radio_Trx_cancelRx(void)
{
    clearPendingEvents(&radioObject);
    TRX_Host_Status status = TRX_Host_stopCmd(rf_handle, radioObject.commands.rxCmdSlot, TRX_Command_StopType_Graceful);
    if(TRX_Host_Success == status)
    {
        // Need to wait for the RX callback is issued so the counters are correct
        Radio_Event event = Radio_Event_None;
        while(!(event & Radio_Event_Rx_Stop) && !(event & Radio_Event_Error))
        {
            event = waitOnEvent(&radioObject);
        }
    }
}

void Radio_Trx_cancelTx(void)
{
    // Only a hard stop cancels a TX test command
    (void)TRX_Host_stopCmd(rf_handle, radioObject.commands.txCmdSlot, TRX_Command_StopType_Hard);
}

bool Radio_Trx_setupPhy(uint8_t phyIndex, uint8_t phyIndex2)
{
    // Disable standby on host if you want to enable standby on the TRX. Known issue, will be fixed in next release.
    (void)Power_disablePolicy();
    uint8_t *prevRfConfig = NULL;
    const uint32_t *prevDeltaConfig = NULL;
    TRX_Host_Status status = TRX_Host_Success;
    bool radioStatus = true;
    bool mdr = radioObject.mdrEnabled;
    PhySettings_Trx_region region = radioObject.region;
    uint8_t RfModeSetting = TRX_RfMode_SUN;

    if(!radioObject.isInit)
    {
        Radio_Trx_init();
        radioObject.mdrEnabled = mdr;
        radioObject.region = region;
    }
    else
    {
        if(!radioObject.mdrEnabled)
        {
            if(RADIO_NO_PHY != radioObject.currentPhyIndex)
            {
                // Replace the previous configuration with the newly requested one
                prevRfConfig = PhySettings_Trx_supportedPhys[radioObject.currentPhyIndex].rfConfig;
            }
        }
        else
        {
            if(RADIO_NO_PHY != radioObject.currentPhyIndex)
            {
                prevRfConfig = PhySettings_Trx_supportedMdrPhys.rfConfig;
                prevDeltaConfig = getDeltaConfig(radioObject.region, radioObject.currentPhyIndex);
            }
        }
    }

    if(!mdr)
    {
        // Only update the configuration if it's changes
        if(prevRfConfig != (uint8_t *)PhySettings_Trx_supportedPhys[phyIndex].rfConfig)
        {
            // Load configuration to the TRX
            clearPendingEvents(&radioObject);

            uint16_t rfConfigSize = getrfConfigSize((uint32_t *)PhySettings_Trx_supportedPhys[phyIndex].rfConfig);

            // Either only 1 phy or 2 MDR phys
            status = TRX_Host_storeConfig(rf_handle, RF_CONFIG_ID,
                                          PhySettings_Trx_supportedPhys[phyIndex].rfConfig,
                                          rfConfigSize, 0xDEADBEEF, true,
                                          configSubmittedCallback,
                                          TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
            if(TRX_Host_Success == status)
            {
                Radio_Event event = Radio_Event_None;
                while(!(event & Radio_Event_Config_Submitted) && !(event & Radio_Event_Error))
                {
                    event = waitOnEvent(&radioObject);
                }

                status = TRX_Host_persistConfig(rf_handle, RF_CONFIG_ID);
                while(TRX_Host_Success != status);
                while(!(event & Radio_Event_Config_Persisted) && !(event & Radio_Event_Error))
                {
                    event = waitOnEvent(&radioObject);
                }
            }
            else
            {
                radioStatus = false;
            }

        }

        radioObject.currentPhyIndex = phyIndex;
        // Set 2nd phy to no-phy
        radioObject.currentPhyIndex2 = RADIO_NO_PHY;

        //update modem image
        // ADDING A NEW PHY: this selects the radio RF mode for the PHY. A new
        // SUN FSK/OFDM/OQPSK PHY falls into the default branch and needs no
        // change. Only add/adjust a branch here if the PHY needs a non-SUN RF
        // mode - keep it consistent with the range macros in PhySettings_trx.h.
        if (phyIndex == PhySettings_Trx_PHY_15) // for Narrow Band GFSK PHY, we need a different peak AGC RF Mode
        {
            RfModeSetting = TRX_RfMode_PropFSKPeakAGC;
        }
        else if((phyIndex >= PHY_SETTINGS_GENFSK_PHY_INDEX_START) && (phyIndex <= PHY_SETTINGS_GENFSK_PHY_INDEX_END)) //Gen FSK
        {
            RfModeSetting = TRX_RfMode_PropFSK;
        }
        else if ((phyIndex >= PHY_SETTINGS_XLR_PHY_INDEX_START) && (phyIndex <= PHY_SETTINGS_XLR_PHY_INDEX_END)) //XLR
        {
            RfModeSetting = TRX_RfMode_XLR;
        }
        else //rest of PHYs all use TRX_RfMode_SUN
        {
             RfModeSetting = TRX_RfMode_SUN;
        }

        TRX_DeviceConfigData deviceConfig = {
            .dioConfig = {
                .dio0 = DIO0_UNCHANGED,
                .dio2 = DIO2_HIGH_PA,
                .dio3 = DIO3_LOW_PA_AND_LNA,
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
            .rfMode = (TRX_RfMode)RfModeSetting,
        };
        status = TRX_Host_storeDeviceConfig(rf_handle, deviceConfig, storeDeviceConfigCallback);

        if(TRX_Host_Success == status)
        {
            Radio_Event event = Radio_Event_None;
            while(!(event & Radio_Event_RFMode_complete) && !(event & Radio_Event_Error))
            {
                event = waitOnEvent(&radioObject);
            }
        }
        else
        {
            radioStatus = false;
        }

        // update radio commands
        radioObject.commands.tx.params.tx.modem = PhySettings_Trx_supportedPhys[phyIndex].modem;
        radioObject.commands.txTest.params.txTest.modem = PhySettings_Trx_supportedPhys[phyIndex].modem;
        radioObject.commands.rx.params.rx.modem = PhySettings_Trx_supportedPhys[phyIndex].modem;
        radioObject.commands.rxTest.params.txTest.modem = PhySettings_Trx_supportedPhys[phyIndex].modem;

        radioObject.commands.tx.params.tx.phy0.config_id = RF_CONFIG_ID;
        radioObject.commands.txTest.params.txTest.phy0.config_id = RF_CONFIG_ID;
        radioObject.commands.rx.params.rx.phy0.config_id= RF_CONFIG_ID;
        radioObject.commands.rxTest.params.rxTest.phy0.config_id = RF_CONFIG_ID;

        radioObject.commands.tx.params.tx.phy0.option_mask = PhySettings_Trx_supportedPhys[phyIndex].optionMask;
        radioObject.commands.txTest.params.txTest.phy0.option_mask = PhySettings_Trx_supportedPhys[phyIndex].optionMask;
        radioObject.commands.rx.params.rx.phy0.option_mask = PhySettings_Trx_supportedPhys[phyIndex].optionMask;
        radioObject.commands.rxTest.params.rxTest.phy0.option_mask = PhySettings_Trx_supportedPhys[phyIndex].optionMask;
    }
    else
    {
        bool isSecondPhy = (RADIO_NO_PHY != phyIndex2);
        if(prevRfConfig != ((uint8_t *)PhySettings_Trx_supportedMdrPhys.rfConfig))
        {
            // Load configuration to the TRX
            clearPendingEvents(&radioObject);

            uint16_t rfConfigSize = getrfConfigSize((uint32_t *)PhySettings_Trx_supportedMdrPhys.rfConfig);

            // Either only 1 phy or 2 MDR phys
            status = TRX_Host_storeConfig(rf_handle, RF_CONFIG_ID,
                                          PhySettings_Trx_supportedMdrPhys.rfConfig,
                                          rfConfigSize, 0xDEADBEEF, true,
                                          configSubmittedCallback,
                                          TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);

            if(TRX_Host_Success == status)
            {
                Radio_Event event = Radio_Event_None;
                while(!(event & Radio_Event_Config_Submitted) && !(event & Radio_Event_Error))
                {
                    event = waitOnEvent(&radioObject);
                }

                status = TRX_Host_persistConfig(rf_handle, RF_CONFIG_ID);
                while(TRX_Host_Success != status);
                while(!(event & Radio_Event_Config_Persisted) && !(event & Radio_Event_Error))
                {
                    event = waitOnEvent(&radioObject);
                }
            }
            else
            {
                radioStatus = false;
            }
        }
        if(PhySettings_Trx_supportedMdrPhys.phys[phyIndex].loadDeltaTable)
        {
            const uint32_t *newDeltaConfig = NULL;
            newDeltaConfig = getDeltaConfig(radioObject.region, phyIndex);
            if(prevDeltaConfig != newDeltaConfig)
            {
                uint16_t deltaTableLength = (((uint16_t)(newDeltaConfig[0] & 0x0FFF)) * sizeof(uint32_t)) + sizeof(uint32_t);

                status = TRX_Host_storeConfig(rf_handle, MDR_FREQ_DELTA_CONFIG_ID,
                                              (uint8_t *)newDeltaConfig,
                                              deltaTableLength, 0xCAFEB0BA, true,
                                              configSubmittedCallback,
                                              TRX_EventConfigStoreComplete);
                if(TRX_Host_Success == status)
                {
                    Radio_Event event = Radio_Event_None;
                    while(!(event & Radio_Event_Config_Submitted) && !(event & Radio_Event_Error))
                    {
                        event = waitOnEvent(&radioObject);
                    }
                }
                else
                {
                    radioStatus = false;
                }
            }
        }

        radioObject.currentPhyIndex = phyIndex;

        radioObject.commands.tx.params.tx.modem = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].modem;
        radioObject.commands.txTest.params.txTest.modem = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].modem;
        radioObject.commands.rx.params.rx.modem = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].modem;
        radioObject.commands.rxTest.params.txTest.modem = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].modem;
        radioObject.commands.cs.params.cs.modem = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].modem;

        radioObject.commands.tx.params.tx.phy0.config_id = RF_CONFIG_ID;
        radioObject.commands.txTest.params.txTest.phy0.config_id = RF_CONFIG_ID;
        radioObject.commands.rx.params.rx.phy0.config_id= RF_CONFIG_ID;
        radioObject.commands.rxTest.params.rxTest.phy0.config_id = RF_CONFIG_ID;
        radioObject.commands.cs.params.cs.phy0.config_id = RF_CONFIG_ID;

        radioObject.commands.tx.params.tx.phy0.option_mask = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].optionMask;
        radioObject.commands.txTest.params.txTest.phy0.option_mask = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].optionMask;
        radioObject.commands.rx.params.rx.phy0.option_mask = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].optionMask;
        radioObject.commands.rxTest.params.rxTest.phy0.option_mask = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].optionMask;
        radioObject.commands.cs.params.cs.phy0.option_mask = PhySettings_Trx_supportedMdrPhys.phys[phyIndex].optionMask;

        if(isSecondPhy && (phyIndex2 != radioObject.currentPhyIndex2))
        {
            // setup mdrTx command
            radioObject.currentPhyIndex2 = phyIndex2;
            radioObject.commands.mdrTx.params.tx.modem = PhySettings_Trx_supportedMdrPhys.phys[phyIndex2].modem;
            radioObject.commands.mdrTx.params.tx.phy0.config_id = RF_CONFIG_ID;
            radioObject.commands.mdrTx.params.tx.phy0.option_mask = PhySettings_Trx_supportedMdrPhys.phys[phyIndex2].optionMask;

            // setup mdrCs command
            radioObject.commands.mdrCs.params.cs.modem = PhySettings_Trx_supportedMdrPhys.phys[phyIndex2].modem;
            radioObject.commands.mdrCs.params.cs.phy0.config_id = RF_CONFIG_ID;
            radioObject.commands.mdrCs.params.cs.phy0.option_mask = PhySettings_Trx_supportedMdrPhys.phys[phyIndex2].optionMask;

        }
    }
    return radioStatus;
}

bool Radio_Trx_enableMdr(uint8_t region)
{
    if(region > 0)
    {
        radioObject.mdrEnabled = true;
        radioObject.region = region;

        uint32_t *pData = (uint32_t *)(PhySettings_Trx_supportedMdrPhys.rfConfig);
        TRX_WisunMdrMappingTable *wisun_map = (TRX_WisunMdrMappingTable *)&(pData[12]);
        PhySettings_Trx_supportedMdrPhys.mappingTable = wisun_map;
    }
    else
    {
        radioObject.mdrEnabled = false;
        radioObject.region = PhySettings_Trx_Region_Disabled;
    }
    return true;
}

bool Radio_Trx_packetTx(uint16_t numPkts, uint32_t *pktLen)
{
    bool retval = true;

    radioObject.counters.packetTx = 0;
    radioObject.packetsToTx = numPkts;

    static uint16_t sequenceNumber = 0;
    uint8_t *ptrTxPayload = &txPayload[0];
    int totPktLength;

    TRX_Request_CommandStore *pCmdTx = &(radioObject.commands.tx);
    pCmdTx->cmd_id = TRX_RadioCommand_Transmit;
    pCmdTx->enable_on_true = false;
    pCmdTx->enable_on_false = false;
    pCmdTx->enable_on_compare = false;
    pCmdTx->trigger = Command_Trigger_Immediate;
    pCmdTx->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdTx->allow_delay = false;
    pCmdTx->params.tx.stream_id = TX_STREAM_ID;

    /** Set pa type based on modem type */
    if(TRX_RadioCommand_Modem_FSK == pCmdTx->params.tx.modem)
    {
        pCmdTx->params.tx.pa = TRX_TxPa_High;
    }
    else if(TRX_RadioCommand_Modem_OFDM == pCmdTx->params.tx.modem)
    {
        pCmdTx->params.tx.pa = TRX_TxPa_High;
    }

        while(0U != radioObject.packetsToTx)
        {
            totPktLength = composePayload(&radioObject, txPayload, *pktLen, sequenceNumber);
            if(0 > totPktLength)
            {
                retval = false;
                goto endPacketTx;
            }

            /** Load the TX command */
            TRX_Host_Status status = TRX_Host_storeCmds(rf_handle, pCmdTx, 1U, NULL, txCallback,
                                                    TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
            if(TRX_Host_Success != status)
            {
                retval = false;
                goto endPacketTx;
            }

            if(((radioObject.currentPhyIndex >= PHY_SETTINGS_GENFSK_PHY_INDEX_START) && (radioObject.currentPhyIndex <= PHY_SETTINGS_GENFSK_PHY_INDEX_END)) ||
               ((radioObject.currentPhyIndex >= PHY_SETTINGS_XLR_PHY_INDEX_START) && (radioObject.currentPhyIndex <= PHY_SETTINGS_XLR_PHY_INDEX_END)))
            {
                //adjust the length and the txpayload ptr
                ptrTxPayload = &txPayload[sizeof(TRX_PayloadHeader_Common)-1];
                totPktLength = ((*pktLen)&0xFF) + TRX_PAYLOADHEADER_LEN_GENFSK; //OOB Gen FSK PHYs have only 1 byte length field
            }

            /** Load the stream of data to transmit */
            status = TRX_Host_storeStream(rf_handle, TX_STREAM_ID, ptrTxPayload, totPktLength,
                                          Stream_Retention_Flush_Streaming, txCallback,
                                          TRX_EventLastStatusError);
            if(TRX_Host_Success != status)
            {
                retval = false;
                goto endPacketTx;
            }

            // Issue TX
            clearPendingEvents(&radioObject);
            retval = issueTxCmd(&radioObject, txCallback);
            if(radioObject.useTxInterval)
            {
                clockNanoSleep(RADIO_PER_TX_INTERVAL_US);
            }

            Radio_Event event = Radio_Event_None;
            while(!(Radio_Event_Error & event) && !(Radio_Event_Tx_Done & event))
            {
                event = waitOnEvent(&radioObject);
            }

            if(Radio_Event_Error & event)
            {
                // There was a failure to TX a packet
                retval = false;
                break;
            }
            else
            {
                radioObject.packetsToTx -= 1;
                sequenceNumber++;
            }
        }
        // Reset sequence number after all packets have been transmitted
        sequenceNumber=0;
endPacketTx:
    return (retval);
}

static int composePayload(Radio_Trx_Object *handle, uint8_t *payload, uint32_t dataLength, uint16_t sequenceNumber)
{
    (void)handle;
    int totalLength;

    int i = 0;
    for (; i < sizeof(TRX_PayloadHeader); i++)
    {
        payload[i] = 0;
    }

    payload[i++] = (sequenceNumber >> 8) & 0x00FF;
    payload[i++] = sequenceNumber & 0x00FF;

    totalLength = i + dataLength - sizeof(sequenceNumber);

    /** Update the PHY header **/
    // ADDING A NEW PHY: the over-the-air payload header is chosen by modem type
    // (and, within FSK/OFDM, by the PHY-index ranges from PhySettings_trx.h).
    // A new PHY that reuses an existing modem + header format works here with no
    // change. Add a branch only if the PHY uses a modem or header layout not
    // already handled below; an unsupported modem returns totalLength = -1.
    if(TRX_RadioCommand_Modem_FSK == handle->commands.tx.params.tx.modem)
    {
        if(((handle->currentPhyIndex >= PHY_SETTINGS_GENFSK_PHY_INDEX_START) && (handle->currentPhyIndex <= PHY_SETTINGS_GENFSK_PHY_INDEX_END)) || //Gen FSK
           ((handle->currentPhyIndex >= PHY_SETTINGS_XLR_PHY_INDEX_START) && (handle->currentPhyIndex <= PHY_SETTINGS_XLR_PHY_INDEX_END)))         //XLR
        {
            payload[sizeof(TRX_PayloadHeader)-1] = (dataLength & 0xFF); //OOB Gen FSK PHYs use 1 byte for length field: use last byte
        }
        else //SUN FSK
        {
            ((TRX_PayloadHeader_SunFSK *)payload)->length = dataLength; // This length excludes the header
            ((TRX_PayloadHeader_SunFSK *)payload)->modulation = TRX_PayloadHeader_Modulation_FSK; // 0U = FSK
            ((TRX_PayloadHeader_SunFSK *)payload)->mode_switch = 0U; // 0U = No mode switch
            if(true == handle->mdrEnabled)
            {
                ((TRX_PayloadHeader_SunFSK *)payload)->fcs_mode = PhySettings_Trx_supportedMdrPhys.phys[handle->currentPhyIndex].header.fsk.fcs_mode;
                ((TRX_PayloadHeader_SunFSK *)payload)->whitening = PhySettings_Trx_supportedMdrPhys.phys[handle->currentPhyIndex].header.fsk.whitening;
            }
            else
            {
                ((TRX_PayloadHeader_SunFSK *)payload)->fcs_mode = PhySettings_Trx_supportedPhys[handle->currentPhyIndex].header.fsk.fcs_mode;
                ((TRX_PayloadHeader_SunFSK *)payload)->whitening = PhySettings_Trx_supportedPhys[handle->currentPhyIndex].header.fsk.whitening;
            }
        } //sun fsk or gen fsk check
    }
    else if(TRX_RadioCommand_Modem_OQPSK == handle->commands.tx.params.tx.modem)
    {
        ((TRX_PayloadHeader_SUNOQPSK *)payload)->length = dataLength; // This length excludes the header
        ((TRX_PayloadHeader_SUNOQPSK *)payload)->rate_mode = TRX_PayloadHeader_SunOQPSK_RateMode3_50kbps;
        ((TRX_PayloadHeader_SUNOQPSK *)payload)->modulation = TRX_PayloadHeader_Modulation_OQPSK;
    }
    else if(TRX_RadioCommand_Modem_OFDM == handle->commands.tx.params.tx.modem)
    {
        if((handle->currentPhyIndex >= PHY_SETTINGS_OFDMLR_PHY_INDEX_START) && (handle->currentPhyIndex <= PHY_SETTINGS_OFDMLR_PHY_INDEX_END)) // OFDM-LR
        {
            ((TRX_PayloadHeader_OFDMLR *)payload)->length = dataLength; // This length excludes the header
            ((TRX_PayloadHeader_OFDMLR *)payload)->rate = PhySettings_Trx_supportedPhys[handle->currentPhyIndex].header.ofdm.rate; // DSSS
            ((TRX_PayloadHeader_OFDMLR *)payload)->modulation = TRX_PayloadHeader_Modulation_OFDM;
        }
        else // SUN OFDM
        {
            ((TRX_PayloadHeader_SunOFDM *)payload)->length = dataLength; // This length excludes the header
            ((TRX_PayloadHeader_SunOFDM *)payload)->modulation = TRX_PayloadHeader_Modulation_OFDM;
            ((TRX_PayloadHeader_SunOFDM *)payload)->newPhyId = 0U;

            if(true == handle->mdrEnabled)
            {
                ((TRX_PayloadHeader_SunOFDM *)payload)->rate = PhySettings_Trx_supportedMdrPhys.phys[handle->currentPhyIndex].header.ofdm.rate; // MCS
                ((TRX_PayloadHeader_SunOFDM *)payload)->scrambler = PhySettings_Trx_supportedMdrPhys.phys[handle->currentPhyIndex].header.ofdm.scrambler;
            }
            else
            {
                ((TRX_PayloadHeader_SunOFDM *)payload)->rate = PhySettings_Trx_supportedPhys[handle->currentPhyIndex].header.ofdm.rate; // MCS
                ((TRX_PayloadHeader_SunOFDM *)payload)->scrambler = PhySettings_Trx_supportedPhys[handle->currentPhyIndex].header.ofdm.scrambler;
            }
        }
    }
    else
    {
        // Unsupported modem
        totalLength = -1;
    }

    for (; i < totalLength; i++)
    {
        payload[i] = rand();
    }

    return (totalLength);
}

bool Radio_Trx_packetMdrTx(uint16_t numPkts, uint32_t *pktLen)
{
    bool retval = false;

    if(radioObject.mdrEnabled && (RADIO_NO_PHY != radioObject.currentPhyIndex2))
    {
        radioObject.counters.packetTx = 0;
        radioObject.packetsToTx = numPkts;

        // Set up MDR packet header and payload
        int totPpduLength = composeMdrPpdu(&(radioObject.commands.tx), &(radioObject.commands.mdrTx), mdrPayload, radioObject.region, radioObject.currentPhyIndex, radioObject.currentPhyIndex2);
        if(0 <= totPpduLength)
        {
            int totPktLength = composeMdrPayload(&(radioObject.commands.mdrTx), *pktLen, txPayload, radioObject.region, radioObject.currentPhyIndex2);
            if(0 <= totPktLength)
            {
                TRX_Request_CommandStore *pCmdTx = &(radioObject.commands.tx);
                TRX_Request_CommandStore *pCmdMdrTx = &(radioObject.commands.mdrTx);
                uint8_t mdrTxSlot = getCmdSlot((TRX_Request_CommandStore *)&(radioObject.commands), NUM_RF_CMDS, pCmdMdrTx);

                // Set the delta frequency for the TX command:
                int32_t tempFreq = pCmdMdrTx->params.tx.frequency;
                int32_t deltaFreq = (int32_t)(pCmdMdrTx->params.tx.frequency - pCmdTx->params.tx.frequency);
                pCmdMdrTx->params.tx.frequency = deltaFreq;

                pCmdTx->cmd_id = TRX_RadioCommand_Transmit;
                pCmdTx->slot_on_true = mdrTxSlot;
                pCmdTx->enable_on_true = true;
                pCmdTx->enable_on_false = false;
                pCmdTx->enable_on_compare = false;
                pCmdTx->trigger = Command_Trigger_Immediate;
                pCmdTx->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
                pCmdTx->allow_delay = false;
                pCmdTx->params.tx.stream_id = TX_STREAM_ID;

                pCmdMdrTx->cmd_id = TRX_RadioCommand_Transmit;
                pCmdMdrTx->enable_on_true = false;
                pCmdMdrTx->enable_on_false = false;
                pCmdMdrTx->enable_on_compare = false;
                pCmdMdrTx->chain_trigger = Command_Trigger_Time_Relative_Previous_End;
                pCmdMdrTx->chain_trig_param = WISUN_MDR_TX_SETTLING_DELAY;
                pCmdMdrTx->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
                pCmdMdrTx->allow_delay = false;
                pCmdMdrTx->params.tx.stream_id = MDR_TX_STREAM_ID;

                /** Set pa type based on modem type */
                if(TRX_RadioCommand_Modem_FSK == pCmdMdrTx->params.tx.modem)
                {
                    pCmdMdrTx->params.tx.pa = TRX_TxPa_High;
                }
                else if(TRX_RadioCommand_Modem_OFDM == pCmdMdrTx->params.tx.modem)
                {
                    pCmdMdrTx->params.tx.pa = TRX_TxPa_High;
                }

                retval = true;
                while(0U != radioObject.packetsToTx)
                {
                    /** Load the 1st TX command to transmit */
                    TRX_Host_Status status = TRX_Host_storeCmds(rf_handle, pCmdTx, 1U, NULL, mdrTxCallback,
                                                            TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
                    if(TRX_Host_Success != status)
                    {
                        retval = false;
                        goto endPacketMdrTx;
                    }

                    /** Load the 2nd TX command to transmit */
                    status = TRX_Host_storeCmds(rf_handle, pCmdMdrTx, 1U, NULL, mdrTxCallback,
                                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
                    if(TRX_Host_Success != status)
                    {
                        retval = false;
                        goto endPacketMdrTx;
                    }

                    /** Load the stream of data to transmit */
                    status = TRX_Host_storeStream(rf_handle, TX_STREAM_ID, mdrPayload, totPpduLength,
                                                Stream_Retention_Flush_Streaming, mdrTxCallback,
                                                TRX_EventLastStatusError);
                    if(TRX_Host_Success != status)
                    {
                        retval = false;
                        goto endPacketMdrTx;
                    }

                    status = TRX_Host_storeStream(rf_handle, MDR_TX_STREAM_ID, txPayload, totPktLength,
                                                Stream_Retention_Flush_Streaming, mdrTxCallback,
                                                TRX_EventLastStatusError);
                    if(TRX_Host_Success != status)
                    {
                        retval = false;
                        goto endPacketMdrTx;
                    }

                    // Issue TX
                    clearPendingEvents(&radioObject);
                    retval = issueMdrTxCmd(&radioObject, mdrTxCallback);
                    if(radioObject.useTxInterval)
                    {
                        clockNanoSleep(RADIO_PER_TX_INTERVAL_US);
                    }

                    Radio_Event event = Radio_Event_None;
                    while(!(Radio_Event_Error & event) && !(Radio_Event_Mdr_Done & event))
                    {
                        event = waitOnEvent(&radioObject);
                    }

                    if(Radio_Event_Error & event)
                    {
                        // There was a failure to TX a packet
                        retval = false;
                        break;
                    }
                    else
                    {
                        radioObject.packetsToTx -= 1;
                    }
                }
                pCmdMdrTx->params.tx.frequency = tempFreq;
            }
        }
    }

endPacketMdrTx:
    return (retval);
}

bool Radio_Trx_packetMdrCsTx(uint16_t numPkts, uint32_t *pktLen)
{
    bool retval = false;

    if(radioObject.mdrEnabled && (RADIO_NO_PHY != radioObject.currentPhyIndex2))
    {
        radioObject.counters.packetTx = 0;
        radioObject.counters.csBusy = 0;
        radioObject.counters.mdrCsBusy = 0;
        radioObject.packetsToTx = numPkts;

        // Set up MDR packet header and payload
        int totPpduLength = composeMdrPpdu(&(radioObject.commands.tx), &(radioObject.commands.mdrTx), mdrPayload, radioObject.region, radioObject.currentPhyIndex, radioObject.currentPhyIndex2);
        if(0 <= totPpduLength)
        {
            int totPktLength = composeMdrPayload(&(radioObject.commands.mdrTx), *pktLen, txPayload, radioObject.region, radioObject.currentPhyIndex2);
            if(0 <= totPktLength)
            {
                TRX_Request_CommandStore *pCmdCs = &(radioObject.commands.cs);
                TRX_Request_CommandStore *pCmdTx = &(radioObject.commands.tx);
                TRX_Request_CommandStore *pCmdMdrCs = &(radioObject.commands.mdrCs);
                TRX_Request_CommandStore *pCmdMdrTx = &(radioObject.commands.mdrTx);
                uint8_t mdrTxSlot = getCmdSlot((TRX_Request_CommandStore *)&(radioObject.commands), NUM_RF_CMDS, pCmdMdrTx);
                uint8_t mdrCsSlot = getCmdSlot((TRX_Request_CommandStore *)&(radioObject.commands), NUM_RF_CMDS, pCmdMdrCs);
                uint8_t txSlot = getCmdSlot((TRX_Request_CommandStore *)&(radioObject.commands), NUM_RF_CMDS, pCmdTx);

                composeMdrCs(pCmdCs, radioObject.region, radioObject.currentPhyIndex);
                composeMdrCs(pCmdMdrCs, radioObject.region, radioObject.currentPhyIndex2);

                pCmdCs->cmd_id = TRX_RadioCommand_CarrierSense;
                pCmdCs->enable_on_true = false;
                pCmdCs->slot_on_false = txSlot;
                pCmdCs->enable_on_false = true;
                pCmdCs->enable_on_compare = false;
                pCmdCs->trigger = Command_Trigger_Immediate;
                pCmdCs->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
                pCmdCs->allow_delay = false;

                pCmdTx->cmd_id = TRX_RadioCommand_Transmit;
                pCmdTx->slot_on_true = mdrCsSlot;
                pCmdTx->enable_on_true = true;
                pCmdTx->enable_on_false = false;
                pCmdTx->enable_on_compare = false;
                pCmdTx->chain_trigger = Command_Trigger_Immediate;
                pCmdTx->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
                pCmdTx->allow_delay = false;
                pCmdTx->params.tx.stream_id = TX_STREAM_ID;

                /** Set pa type based on modem type */
                if(TRX_RadioCommand_Modem_FSK == pCmdTx->params.tx.modem)
                {
                    pCmdTx->params.tx.pa = TRX_TxPa_High;
                }
                else if(TRX_RadioCommand_Modem_OFDM == pCmdTx->params.tx.modem)
                {
                    pCmdTx->params.tx.pa = TRX_TxPa_High;
                }


                pCmdMdrCs->cmd_id = TRX_RadioCommand_CarrierSense;
                pCmdMdrCs->enable_on_true = false;
                pCmdMdrCs->slot_on_false = mdrTxSlot;
                pCmdMdrCs->enable_on_false = true;
                pCmdMdrCs->enable_on_compare = false;
                pCmdMdrCs->chain_trigger = Command_Trigger_Immediate;
                pCmdMdrCs->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
                pCmdMdrCs->allow_delay = false;

                pCmdMdrTx->cmd_id = TRX_RadioCommand_Transmit;
                pCmdMdrTx->enable_on_true = false;
                pCmdMdrTx->enable_on_false = false;
                pCmdMdrTx->enable_on_compare = false;
                pCmdMdrTx->chain_trigger = Command_Trigger_Immediate;
                pCmdMdrTx->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
                pCmdMdrTx->allow_delay = false;
                pCmdMdrTx->params.tx.stream_id = MDR_TX_STREAM_ID;

                /** Set pa type based on modem type */
                if(TRX_RadioCommand_Modem_FSK == pCmdMdrTx->params.tx.modem)
                {
                    pCmdMdrTx->params.tx.pa = TRX_TxPa_High;
                }
                else if(TRX_RadioCommand_Modem_OFDM == pCmdMdrTx->params.tx.modem)
                {
                    pCmdMdrTx->params.tx.pa = TRX_TxPa_High;
                }

                // Set Delta Frequency
                int32_t tempFreq = pCmdMdrTx->params.tx.frequency;
                int32_t deltaFreq = (int32_t)(pCmdMdrTx->params.tx.frequency - pCmdTx->params.tx.frequency);
                pCmdTx->params.tx.frequency = 0U;
                pCmdMdrTx->params.tx.frequency = deltaFreq;
                pCmdMdrCs->params.cs.frequency = deltaFreq;

                retval = true;
                while(0U != radioObject.packetsToTx)
                {

                    TRX_Host_Status status = TRX_Host_storeCmds(rf_handle, pCmdTx, NUM_RF_CMDS, NULL, mdrCsTxCallback,
                                                            TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
                    if(TRX_Host_Success != status)
                    {
                        retval = false;
                        goto endPacketMdrCsTx;
                    }

                    /** Load the stream of data to transmit */
                    status = TRX_Host_storeStream(rf_handle, TX_STREAM_ID, mdrPayload, totPpduLength,
                                                Stream_Retention_Flush_Streaming, mdrCsTxCallback,
                                                TRX_EventLastStatusError);
                    if(TRX_Host_Success != status)
                    {
                        retval = false;
                        goto endPacketMdrCsTx;
                    }

                    status = TRX_Host_storeStream(rf_handle, MDR_TX_STREAM_ID, txPayload, totPktLength,
                                                Stream_Retention_Flush_Streaming, mdrCsTxCallback,
                                                TRX_EventLastStatusError);
                    if(TRX_Host_Success != status)
                    {
                        retval = false;
                        goto endPacketMdrCsTx;
                    }

                    // Issue TX
                    clearPendingEvents(&radioObject);
                    retval = issueMdrCsTxCmd(&radioObject, mdrCsTxCallback);
                    if(radioObject.useTxInterval)
                    {
                        clockNanoSleep(RADIO_PER_TX_INTERVAL_US);
                    }

                    Radio_Event event = Radio_Event_None;
                    while(!(Radio_Event_Error & event) && !(Radio_Event_Mdr_Done & event))
                    {
                        event = waitOnEvent(&radioObject);
                    }

                    if(Radio_Event_Error & event)
                    {
                        // There was a failure to TX a packet
                        retval = false;
                        break;
                    }
                    else
                    {
                        radioObject.packetsToTx -= 1;
                    }
                }
                TestMode_perTxPrintMsg(radioObject.counters.packetTx);
                pCmdMdrTx->params.tx.frequency = tempFreq;
                pCmdMdrCs->params.cs.frequency = tempFreq;
                pCmdTx->params.tx.frequency = tempFreq;
            }
        }
    }

endPacketMdrCsTx:
    return (retval);
}

bool Radio_Trx_packetCsTx(uint16_t numPkts, uint32_t *pktLen)
{
    bool retval = false;

    if(radioObject.mdrEnabled)
    {
        radioObject.counters.packetTx = 0;
        radioObject.counters.csBusy = 0;
        radioObject.packetsToTx = numPkts;

        // Set up packet header and payload

        int totPktLength = composeMdrPayload(&(radioObject.commands.tx), *pktLen, txPayload, radioObject.region, radioObject.currentPhyIndex);
        if(0 <= totPktLength)
        {
            TRX_Request_CommandStore *pCmdCs = &(radioObject.commands.cs);
            TRX_Request_CommandStore *pCmdTx = &(radioObject.commands.tx);
            uint8_t txSlot = getCmdSlot((TRX_Request_CommandStore *)&(radioObject.commands), NUM_RF_CMDS, pCmdTx);

            composeMdrCs(pCmdCs, radioObject.region, radioObject.currentPhyIndex);

            pCmdCs->cmd_id = TRX_RadioCommand_CarrierSense;
            pCmdCs->enable_on_true = false;
            pCmdCs->slot_on_false = txSlot;
            pCmdCs->enable_on_false = true;
            pCmdCs->enable_on_compare = false;
            pCmdCs->trigger = Command_Trigger_Immediate;
            pCmdCs->conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
            pCmdCs->allow_delay = false;

            pCmdTx->cmd_id = TRX_RadioCommand_Transmit;
            pCmdTx->enable_on_true = false;
            pCmdTx->enable_on_false = false;
            pCmdTx->enable_on_compare = false;
            pCmdTx->chain_trigger = Command_Trigger_Immediate;
            pCmdTx->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
            pCmdTx->allow_delay = false;
            pCmdTx->params.tx.stream_id = TX_STREAM_ID;

            int32_t tempFreq = pCmdTx->params.tx.frequency;
            pCmdTx->params.tx.frequency = 0U;

            /** Set pa type based on modem type */
            if(TRX_RadioCommand_Modem_FSK == pCmdTx->params.tx.modem)
            {
                pCmdTx->params.tx.pa = TRX_TxPa_High;
            }
            else if(TRX_RadioCommand_Modem_OFDM == pCmdTx->params.tx.modem)
            {
                pCmdTx->params.tx.pa = TRX_TxPa_High;
            }

            retval = true;
            while(0U != radioObject.packetsToTx)
            {

                TRX_Host_Status status = TRX_Host_storeCmds(rf_handle, pCmdTx, 1U, NULL, csTxCallback,
                                                        TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
                if(TRX_Host_Success != status)
                {
                    retval = false;
                    goto endPacketCsTx;
                }

                status = TRX_Host_storeCmds(rf_handle, pCmdCs, 1U, NULL, csTxCallback,
                                            TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
                if(TRX_Host_Success != status)
                {
                    retval = false;
                    goto endPacketCsTx;
                }

                /** Load the stream of data to transmit */
                status = TRX_Host_storeStream(rf_handle, TX_STREAM_ID, txPayload, totPktLength,
                                            Stream_Retention_Flush_Streaming, csTxCallback,
                                            TRX_EventLastStatusError);
                if(TRX_Host_Success != status)
                {
                    retval = false;
                    goto endPacketCsTx;
                }

                // Issue TX
                clearPendingEvents(&radioObject);
                retval = issueMdrCsTxCmd(&radioObject, csTxCallback);
                if(radioObject.useTxInterval)
                {
                    clockNanoSleep(RADIO_PER_TX_INTERVAL_US);
                }

                Radio_Event event = Radio_Event_None;
                while(!(Radio_Event_Error & event) && !(Radio_Event_Tx_Done & event))
                {
                    event = waitOnEvent(&radioObject);
                }

                if(Radio_Event_Error & event)
                {
                    // There was a failure to TX a packet
                    retval = false;
                    break;
                }
                else
                {
                    radioObject.packetsToTx -= 1;
                }
            }
            TestMode_perTxPrintMsg(radioObject.counters.packetTx);
            pCmdTx->params.tx.frequency = tempFreq;
        }

    }
endPacketCsTx:
    return (retval);
}


static bool issueMdrTxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb)
{
    (void)cb;
    bool retval = true;
    TRX_Request_CommandStore *pCmdTx = &(handle->commands.tx);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCmdTx);

    TRX_Host_Status status = TRX_Host_submitCmd(rf_handle, slot);
    if (TRX_Host_Success != status)
    {
        retval = false;
    }

    handle->commands.txCmdSlot = slot;

    // Print the packet number (starting at 1)
    TestMode_perTxPrintMsg(handle->counters.packetTx + 1);

    return (retval);
}

static bool issueMdrCsTxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb)
{
    (void)cb;
    bool retval = true;
    TRX_Request_CommandStore *pCmdCs = &(handle->commands.cs);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCmdCs);

    TRX_Host_Status status = TRX_Host_submitCmd(rf_handle, slot);
    if (TRX_Host_Success != status)
    {
        retval = false;
    }

    handle->commands.txCmdSlot = slot;

    // Print the packet number (starting at 1)
    TestMode_perTxPrintMsg(handle->counters.packetTx);

    return (retval);
}

static bool issueTxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb)
{
    (void)cb;
    bool retval = true;
    TRX_Request_CommandStore *pCmdTx = &(handle->commands.tx);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCmdTx);

    TRX_Host_Status status = TRX_Host_submitCmd(rf_handle, slot);
    if (TRX_Host_Success != status)
    {
        retval = false;
    }

    handle->commands.txCmdSlot = slot;

    // Print the packet number (starting at 1)
    TestMode_perTxPrintMsg(handle->counters.packetTx + 1);

    return (retval);
}

bool Radio_Trx_packetRx(uint16_t pktLen)
{
    bool retval = true;

    resetStatistics(&radioObject);
    radioObject.counters.packetRx = 0;
    radioObject.counters.packetRxNok = 0;
    radioObject.counters.rxSync = 0;
    radioObject.counters.packetRxStopped = 0;

    TRX_Request_CommandStore *pCmdRx = &(radioObject.commands.rx);
    pCmdRx->cmd_id = TRX_RadioCommand_Receive;
    pCmdRx->enable_on_true = false;
    pCmdRx->enable_on_false = false;
    pCmdRx->enable_on_compare = false;
    pCmdRx->chain_trigger = Command_Trigger_Immediate;
    pCmdRx->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdRx->allow_delay = false;
    pCmdRx->params.rx.stream_id = RX_STREAM_ID;
    pCmdRx->params.rx.phy1.config_id = 0U;
    pCmdRx->params.rx.phy1.option_mask = 0U;
    pCmdRx->params.rx.enable_mdr = false;
    pCmdRx->params.rx.stream_early = true;
    pCmdRx->params.rx.timeout = 0U;
    pCmdRx->params.rx.repeat = true;
    pCmdRx->params.rx.search_strategy = TRX_Rx_SearchStrategy_Sync;

    TRX_Host_Status status = TRX_Host_registerRxStream(rf_handle, RX_STREAM_ID, rxPayload,
                                                       sizeof(rxPayload), rxCallback,
                                                       TRX_EventFinalStreamStoreReceived);
    if(TRX_Host_Success != status)
    {
        retval = false;
        goto endPacketRx;
    }

    status = TRX_Host_storeCmds(rf_handle, pCmdRx, 1U, NULL, rxCallback,
                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
    if(TRX_Host_Success != status)
    {
        retval = false;
        goto endPacketRx;
    }

    // Issue the RX command once, here. It repeats by default
    retval = issueRxCmd(&radioObject, rxCallback);

endPacketRx:
    return (retval);
}

bool Radio_Trx_packetMdrRx(uint16_t pktLen)
{
    bool retval = false;
    if(radioObject.mdrEnabled)
    {
        resetStatistics(&radioObject);
        radioObject.counters.packetRx = 0;
        radioObject.counters.packetRxNok = 0;
        radioObject.counters.rxSync = 0;
        radioObject.counters.packetRxStopped = 0;

        TRX_Request_CommandStore *pCmdRx = &(radioObject.commands.rx);
        pCmdRx->cmd_id = TRX_RadioCommand_Receive;
        pCmdRx->enable_on_true = false;
        pCmdRx->enable_on_false = false;
        pCmdRx->enable_on_compare = false;
        pCmdRx->chain_trigger = Command_Trigger_Immediate;
        pCmdRx->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
        pCmdRx->allow_delay = false;
        pCmdRx->params.rx.stream_id = RX_STREAM_ID;
        pCmdRx->params.rx.phy1.config_id = 0U;
        pCmdRx->params.rx.phy1.option_mask = 0U;
        pCmdRx->params.rx.enable_mdr = true;
        pCmdRx->params.rx.stream_early = true;
        pCmdRx->params.rx.timeout = 0U;
        pCmdRx->params.rx.repeat = true;
        pCmdRx->params.rx.search_strategy = TRX_Rx_SearchStrategy_Sync;

        // Set the delta frequency channel plan
        retval = setChannelPlan(&radioObject);
        if(retval)
        {
            TRX_Host_Status status = TRX_Host_registerRxStream(rf_handle, RX_STREAM_ID, rxPayload,
                                                               sizeof(rxPayload), rxCallback,
                                                               TRX_EventFinalStreamStoreReceived);
            if(TRX_Host_Success != status)
            {
                retval = false;
                goto endPacketMdrRx;
            }

            status = TRX_Host_storeCmds(rf_handle, pCmdRx, 1U, NULL, rxCallback,
                                        TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
            if(TRX_Host_Success != status)
            {
                retval = false;
                goto endPacketMdrRx;
            }

            // Issue the RX command once, here. It repeats by default
            retval = issueRxCmd(&radioObject, rxCallback);
        }
    }

endPacketMdrRx:
    return (retval);
}

static bool issueRxCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb)
{
    (void)cb;
    bool retval = true;
    TRX_Request_CommandStore *pCmdRx = &(handle->commands.rx);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCmdRx);

    TRX_Host_Status status = TRX_Host_submitCmd(rf_handle, slot);
    if (TRX_Host_Success != status)
    {
        retval = false;
    }

    handle->commands.rxCmdSlot = slot;

    return (retval);
}

static bool setChannelPlan(Radio_Trx_Object *handle)
{
    bool retval = false;
    TRX_Request_CommandStore *pCmdRx = &(handle->commands.rx);
    pCmdRx->params.rx.phy1.config_id = MDR_FREQ_DELTA_CONFIG_ID;
    pCmdRx->params.rx.phy1.option_mask = 0U;

    // set the base index into optionMask1
    int16_t baseIdx = getDeltaBaseIndex(handle->region, handle->currentPhyIndex, pCmdRx->params.rx.frequency);
    if(baseIdx >= 0)
    {
        pCmdRx->params.rx.phy1.option_mask = (uint16_t)baseIdx;
        retval = true;
    }

    return (retval);
}

bool Radio_Trx_contRx(void)
{
    bool retval = true;

    resetStatistics(&radioObject);
    radioObject.counters.packetRx = 0;
    radioObject.counters.packetRxNok = 0;
    radioObject.counters.rxSync = 0;
    radioObject.counters.packetRxStopped = 0;

    TRX_Request_CommandStore *pCmdRxTest = &(radioObject.commands.rxTest);
    pCmdRxTest->cmd_id = TRX_RadioCommand_ReceiveTest;
    pCmdRxTest->enable_on_true = false;
    pCmdRxTest->enable_on_false = false;
    pCmdRxTest->enable_on_compare = false;
    pCmdRxTest->chain_trigger = Command_Trigger_Immediate;
    pCmdRxTest->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdRxTest->allow_delay = false;
    pCmdRxTest->params.rxTest.stream_id = RX_STREAM_ID;
    pCmdRxTest->params.rxTest.enable_mdr = false;
    pCmdRxTest->params.rxTest.stream_early = true;
    pCmdRxTest->params.rxTest.timeout = 0U;
    pCmdRxTest->params.rxTest.repeat = true;
    pCmdRxTest->params.rxTest.search_strategy = TRX_Rx_SearchStrategy_Sync;
    pCmdRxTest->params.rxTest.test_nosync = true;
    pCmdRxTest->params.rxTest.test_discard_packets  = true;

    TRX_Host_Status status = TRX_Host_registerRxStream(rf_handle, RX_STREAM_ID, rxPayload,
                                                       sizeof(rxPayload), rxTestCallback,
                                                       TRX_EventFinalStreamStoreReceived);
    if(TRX_Host_Success != status)
    {
        retval = false;
        goto endContRx;
    }

    status = TRX_Host_storeCmds(rf_handle, pCmdRxTest, 1U, NULL, rxTestCallback,
                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
    if(TRX_Host_Success != status)
    {
        retval = false;
        goto endContRx;
    }

    retval = issueRxTestCmd(&radioObject, rxTestCallback);

endContRx:
    return (retval);
}

static bool issueRxTestCmd(Radio_Trx_Object *handle, TRX_Host_Callback cb)
{
    (void)cb;
    bool retval = true;
    TRX_Request_CommandStore *pCmdRxTest = &(handle->commands.rxTest);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(handle->commands), NUM_RF_CMDS, pCmdRxTest);

    TRX_Host_Status status = TRX_Host_submitCmd(rf_handle, slot);
    if (TRX_Host_Success != status)
    {
        retval = false;
    }

    handle->commands.rxCmdSlot = slot;

    return (retval);
}

static void signalEvent(Radio_Trx_Object *handle, Radio_Event evt)
{
    uintptr_t key = HwiP_disable();
    handle->evt |= evt;
    HwiP_restore(key);
    (void)sem_post(&handle->osPrimitives.radioEvent);
}

static Radio_Event waitOnEvent(Radio_Trx_Object *handle)
{
    (void)sem_wait(&handle->osPrimitives.radioEvent);
    uintptr_t key = HwiP_disable();
    Radio_Event evt = handle->evt;
    handle->evt = Radio_Event_None;
    HwiP_restore(key);
    return (evt);
}

static void clearPendingEvents(Radio_Trx_Object *handle)
{
    /* Remove all pending semaphores */
    while(sem_trywait(&handle->osPrimitives.radioEvent) == 0);
    handle->evt = Radio_Event_None;
}

bool Radio_Trx_contTx(bool cw)
{
    TRX_Request_CommandStore *pCmdTxTest = &(radioObject.commands.txTest);
    uint8_t slot = getCmdSlot((TRX_Request_CommandStore *)&(radioObject.commands), NUM_RF_CMDS, pCmdTxTest);
    pCmdTxTest->cmd_id = TRX_RadioCommand_TransmitTest;
    pCmdTxTest->enable_on_true = false;
    pCmdTxTest->enable_on_false = false;
    pCmdTxTest->enable_on_compare = false;
    pCmdTxTest->chain_trigger = Command_Trigger_Immediate;
    pCmdTxTest->chain_conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    pCmdTxTest->allow_delay = false;
    pCmdTxTest->params.txTest.stream_id = 0U;
    pCmdTxTest->params.txTest.mode = (true == cw) ? TRX_TxTestMode_Unmodulated : TRX_TxTestMode_PN9;

    /** Set pa type based on modem type */
    if(TRX_RadioCommand_Modem_FSK == pCmdTxTest->params.tx.modem)
    {
        pCmdTxTest->params.tx.pa = TRX_TxPa_High;
    }
    else if(TRX_RadioCommand_Modem_OFDM == pCmdTxTest->params.tx.modem)
    {
        pCmdTxTest->params.tx.pa = TRX_TxPa_High;
    }

    bool retval = true;
    TRX_Host_Status status = TRX_Host_storeCmds(rf_handle, pCmdTxTest, 1U, NULL, txTestCallback,
                                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);

    if (TRX_Host_Success != status)
    {
        retval = false;
    }
    else
    {
        TRX_Host_Status status = TRX_Host_submitCmd(rf_handle, slot);
        if (TRX_Host_Success != status)
        {
            retval = false;
        }
    }

    radioObject.commands.txCmdSlot = slot;

    return(retval);
}

static void setMdrFrequencies(uint32_t freq, uint32_t mdrFreq)
{
    TRX_Request_CommandStore *pCmdCs = &(radioObject.commands.cs);
    TRX_Request_CommandStore *pCmdTx = &(radioObject.commands.tx);
    TRX_Request_CommandStore *pCmdRx = &(radioObject.commands.rx);
    TRX_Request_CommandStore *pCmdMdrCs = &(radioObject.commands.mdrCs);
    TRX_Request_CommandStore *pCmdMdrTx = &(radioObject.commands.mdrTx);
    TRX_Request_CommandStore *pCmdTxTest = &(radioObject.commands.txTest);
    TRX_Request_CommandStore *pCmdRxTest = &(radioObject.commands.rxTest);

    if(WISUN_FREQ_INVALID != freq)
    {
        uint8_t phyIndex = radioObject.currentPhyIndex;

        // Setting the first frequency based on first channel passed in
        uint32_t convFreq = getFrequencyFromChannel(radioObject.region, phyIndex, (int16_t)freq);
        if(WISUN_FREQ_INVALID != convFreq)
        {
            radioObject.frequency = convFreq;
            pCmdTx->params.tx.frequency = convFreq;
            pCmdTxTest->params.txTest.frequency = convFreq;
            pCmdRx->params.rx.frequency = convFreq;
            pCmdRxTest->params.rxTest.frequency = convFreq;
            pCmdCs->params.cs.frequency = convFreq;
        }
    }

    if(MAX_CHANNEL < mdrFreq)
    {
        if(WISUN_FREQ_INVALID != mdrFreq)
        {
            mdrFreq = mdrFreq / 1000;
            pCmdMdrTx->params.tx.frequency = mdrFreq;
            pCmdMdrCs->params.cs.frequency = mdrFreq;
        }
        else
        { // No MDR Frequency input, so set mdr frequency to default next frequency
            uint32_t mdrDeltaFreq = getNextDeltaFrequency(radioObject.region, radioObject.currentPhyIndex2, radioObject.frequency);
            pCmdMdrTx->params.tx.frequency = mdrDeltaFreq;
            pCmdMdrCs->params.cs.frequency = mdrDeltaFreq;
        }
    }
    else
    { // input is a channel, convert the channel into a frequency
        uint8_t phyIndex = radioObject.currentPhyIndex2;

        // Setting the first frequency based on first channel passed in
        uint32_t convFreq = getFrequencyFromChannel(radioObject.region, phyIndex, (int16_t)mdrFreq);
        if(WISUN_FREQ_INVALID != convFreq)
        {
            pCmdMdrTx->params.tx.frequency = convFreq;
            pCmdMdrCs->params.cs.frequency = convFreq;
        }
    }
}

void Radio_Trx_setFreq(uint32_t freq, uint32_t mdrFreq)
{
    TRX_Request_CommandStore *pCmdCs = &(radioObject.commands.cs);
    TRX_Request_CommandStore *pCmdTx = &(radioObject.commands.tx);
    TRX_Request_CommandStore *pCmdRx = &(radioObject.commands.rx);
    TRX_Request_CommandStore *pCmdTxTest = &(radioObject.commands.txTest);
    TRX_Request_CommandStore *pCmdRxTest = &(radioObject.commands.rxTest);

    if(radioObject.mdrEnabled)
    {
        // Check if frequency or channel was passed in
        if(MAX_CHANNEL < freq)
        {
            freq = freq / 1000;
            radioObject.frequency = freq;
            pCmdTx->params.tx.frequency = freq;
            pCmdTxTest->params.txTest.frequency = freq;
            pCmdRx->params.rx.frequency = freq;
            pCmdRxTest->params.rxTest.frequency = freq;
            pCmdCs->params.cs.frequency = freq;

            // Set frequency to invalid so that helper function doesnt edit freq
            freq = WISUN_FREQ_INVALID;
        }
        setMdrFrequencies(freq, mdrFreq);

    }
    else
    {
        freq = freq / 1000;
        radioObject.frequency = freq;
        pCmdTx->params.tx.frequency = freq;
        pCmdTxTest->params.txTest.frequency = freq;
        pCmdRx->params.rx.frequency = freq;
        pCmdRxTest->params.rxTest.frequency = freq;
        pCmdCs->params.cs.frequency = freq;
    }
}

void Radio_Trx_getFreq(RF_Frequency *freqs)
{
    freqs->freq = (uint32_t)RADIO_ERROR_VALUE;
    freqs->mdrFreq = 0;

    if (radioObject.isInit)
    {
        freqs->freq = radioObject.frequency * 1000;
        if(radioObject.mdrEnabled)
        {
            TRX_Request_CommandStore *pCmdMdrTx = &(radioObject.commands.mdrTx);
            freqs->mdrFreq = pCmdMdrTx->params.tx.frequency * 1000;
        }
    }
}

bool Radio_Trx_setPower(int8_t i8TxPowerDbm)
{
    radioObject.txPower = i8TxPowerDbm;
    TRX_Request_CommandStore *pCmdTx = &(radioObject.commands.tx);
    TRX_Request_CommandStore *pCmdMdrTx = &(radioObject.commands.mdrTx);
    TRX_Request_CommandStore *pCmdTxTest = &(radioObject.commands.txTest);

    if((i8TxPowerDbm == TRX_MAX_POWER) || (i8TxPowerDbm == TRX_MIN_POWER))
    {
        pCmdTx->params.tx.power.rawValue = i8TxPowerDbm;
        pCmdMdrTx->params.tx.power.rawValue = i8TxPowerDbm;
        pCmdTxTest->params.tx.power.rawValue = i8TxPowerDbm;
    }
    else
    {
        pCmdTx->params.tx.power.dBm = i8TxPowerDbm;
        pCmdMdrTx->params.tx.power.dBm = i8TxPowerDbm;
        pCmdTxTest->params.tx.power.dBm = i8TxPowerDbm;

        pCmdTx->params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
        pCmdMdrTx->params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
        pCmdTxTest->params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
    }

    return true;
}

int8_t Radio_Trx_getPower(void)
{
    return (radioObject.txPower);
}

int8_t Radio_Trx_getCurrentRssi(void)
{
    radioObject.statistics.currRssi = INVALID_RSSI;
    clearPendingEvents(&radioObject);
    TRX_Host_Status status = TRX_Host_utilGetRssi(rf_handle, getRssiCallback);

    if(TRX_Host_Success == status){
        // Wait for value to be returned by the TRX or for TRX to be in
        // an invalid state when the get RSSI command was issued.
        Radio_Event event = Radio_Event_None;
        while(!(event & Radio_Event_GetRssi) && !(event & Radio_Event_Error))
        {
            event = waitOnEvent(&radioObject);
        }
    }

    return (radioObject.statistics.currRssi);
}

int32_t Radio_Trx_getAvgRssi(void)
{
    return (radioObject.statistics.avgRssi/(int32_t)radioObject.counters.packetRx);
}

int8_t Radio_Trx_getMinRssi(void)
{
    return (radioObject.statistics.minRssi);
}

int8_t Radio_Trx_getMaxRssi(void)
{
    return (radioObject.statistics.maxRssi);
}

uint16_t Radio_Trx_getNumRxPackets(void)
{
    return (radioObject.counters.packetRx);
}

uint16_t Radio_Trx_getNumRxPacketsNok(void)
{
    return (radioObject.counters.packetRxNok);
}

uint16_t Radio_Trx_getNumRxSync(void)
{
    return (radioObject.counters.rxSync);
}

 uint8_t Radio_Trx_getRxPhyIndex(void)
 {
     return (radioObject.phyReceived);
 }

bool Radio_Trx_checkPacketLength(uint32_t *perPktLen)
{
    uint32_t pktLen = *perPktLen;
    if((pktLen > 1) && (pktLen <= RADIO_MAX_LENGTH))
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t Radio_Trx_getNumSupportedPhys(void)
{
    if(radioObject.mdrEnabled)
        return PhySettings_Trx_numSupportedMdrPhys;
    else
        return PhySettings_Trx_numSupportedPhys;
}

char *Radio_Trx_getPhyName(uint8_t phyIndex)
{
    if(!radioObject.mdrEnabled)
    {
        if (phyIndex < PhySettings_Trx_numSupportedPhys)
            return PhySettings_Trx_supportedPhys[phyIndex].PhySettings_phyName;
        else
            return "Bad Index.";
    }
    else
    {
        if (phyIndex < PhySettings_Trx_numSupportedMdrPhys)
            return PhySettings_Trx_supportedMdrPhys.phys[phyIndex].PhySettings_phyName;
        else
            return "Bad Index.";
    }
}

char *Radio_Trx_getRadioVersion(void)
{
    memset(radioObject.statistics.radioVersion, 0U, RADIO_VERSION_LENGTH);
    strcpy(radioObject.statistics.radioVersion, "Error");
    clearPendingEvents(&radioObject);

    if(!radioObject.isInit)
    {
        bool mdr = radioObject.mdrEnabled;
        PhySettings_Trx_region region = radioObject.region;
        Radio_Trx_init();
        radioObject.mdrEnabled = mdr;
        radioObject.region = region;
    }

    TRX_Host_Status status = TRX_Host_utilGetVersion(rf_handle, getRadioVersionCallback);
    if(status == TRX_Host_Success)
    {
        Radio_Event event = Radio_Event_None;
        while(!(event & Radio_Event_GetFirmware_Version) && !(event & Radio_Event_Error))
        {
            event = waitOnEvent(&radioObject);
        }

        return radioObject.statistics.radioVersion;
    }

    return RADIO_UNSUPPORTED_CMD;
}

/** Converting microseconds to nanoseconds for POSIX time module **/
#define MICRO_TO_NANO (1000)

/** Converting seconds to nanoseconds for POSIX time module **/
#define SEC_TO_NANO (1000000000)

/** Converting microseconds to nanoseconds for POSIX time module **/
#define MICRO_TO_SEC (1000000)

static void clockNanoSleep(int32_t uSec)
{
    int32_t sleepRetVal = ((uSec * MICRO_TO_NANO) % SEC_TO_NANO);
    struct timespec tsRem = {
        .tv_sec = (uint32_t)uSec / (uint32_t)MICRO_TO_SEC,
        .tv_nsec = sleepRetVal,
    };

    do
    {
        struct timespec tsReq = tsRem;
        sleepRetVal = nanosleep(&tsReq, &tsRem);
    } while ((-1 == sleepRetVal) && (errno = EINTR));
}

void Radio_Trx_registerFxns(Radio_Fxns *fxns){
    fxns->init = Radio_Trx_init;
    fxns->setupPhy = Radio_Trx_setupPhy;
    fxns->enableMdr = Radio_Trx_enableMdr;
    fxns->packetTx = Radio_Trx_packetTx;
    fxns->packetRx = Radio_Trx_packetRx;
    fxns->packetMdrTx = Radio_Trx_packetMdrTx;
    fxns->packetMdrRx = Radio_Trx_packetMdrRx;
    fxns->packetMdrCsTx = Radio_Trx_packetMdrCsTx;
    fxns->packetCsTx = Radio_Trx_packetCsTx;
    fxns->contTx = Radio_Trx_contTx;
    fxns->contRx = Radio_Trx_contRx;
    fxns->setFreq = Radio_Trx_setFreq;
    fxns->getFreq = Radio_Trx_getFreq;
    fxns->setPower = Radio_Trx_setPower;
    fxns->getPower = Radio_Trx_getPower;
    fxns->getCurrentRssi = Radio_Trx_getCurrentRssi;
    fxns->getAvgRssi = Radio_Trx_getAvgRssi;
    fxns->getMinRssi = Radio_Trx_getMinRssi;
    fxns->getMaxRssi = Radio_Trx_getMaxRssi;
    fxns->cancelRx = Radio_Trx_cancelRx;
    fxns->cancelTx = Radio_Trx_cancelTx;
    fxns->getNumRxPackets = Radio_Trx_getNumRxPackets;
    fxns->getNumRxPacketsNok = Radio_Trx_getNumRxPacketsNok;
    fxns->getNumRxSync = Radio_Trx_getNumRxSync;
     fxns->getRxPhyIndex = Radio_Trx_getRxPhyIndex;
    fxns->checkPacketLength = Radio_Trx_checkPacketLength;
    fxns->getNumSupportedPhys = Radio_Trx_getNumSupportedPhys;
    fxns->getPhyName = Radio_Trx_getPhyName;
    fxns->getRadioVersion = Radio_Trx_getRadioVersion;
}
