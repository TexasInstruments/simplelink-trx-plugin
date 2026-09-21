/******************************************************************************
 Group: CMCU LPRF
 Target Device: cc13xx_cc26xx

 ******************************************************************************
 
 Copyright (c) 2026, Texas Instruments Incorporated
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

/***** Includes *****/

/* Standard C Libraries */
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* TI Drivers */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

/* Board Header files */
#include "ti_drivers_config.h"

/* TRX RF Header files */
#include <ti/trx/TRX.h>
#include <ti/trx/host/trx_host.h>
#include <ti/trx/host/hal/platform.h>

/***** Defines *****/

/* Maximum payload in a receive operation */
#define RX_PAYLOAD_LENGTH   (76U)

/* The data received will contain modem mask (1B), timestamp (4B) and RSSI (1B) information. */
#define MODEM_MASK_SIZE_BYTES   (1U)
#define RSSI_SIZE_BYTES         (1U)
#define TIMESTAMP_SIZE_BYTES    (4U)

/* Total size of data that can be received including MODEM MASK + HEADER + PAYLOAD + RSSI + TIMESTAMP */
#define RX_PACKET_LENGTH    (MODEM_MASK_SIZE_BYTES + sizeof(TRX_PayloadHeader) + RX_PAYLOAD_LENGTH + RSSI_SIZE_BYTES + TIMESTAMP_SIZE_BYTES)

/* Slot for the RX cmd. Can be anything in the range 0, TRX_MAX_NUM_COMMANDS excluding TRX_MAX_NUM_COMMANDS */
#define RX_CMD_SLOT      (0U)

/* ID of the packet data payload on the TRX. Can be anything in the range 0, TRX_MAX_STREAMS inclusive */
#define RX_STREAM_ID     (0U)

/* ID of the PHY configuration on the TRX. Can be anything in the range 1, TRX_MAX_CONFIGURATIONS inclusive */
#define RF_CONFIG_ID     (1U)

/* Reference of the Front End (FE) configuration on the TRX. Can be any 32-bit value */
#define FE_CONFIG_REFERENCE (0xFEFEFEFE)

/* Reference of the PHY configuration on the TRX. Can be any 32-bit value */
#define RF_CONFIG_REFERENCE (0xF000000D)

/***** Prototypes *****/

static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);

static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg);

static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg);

static void receiveCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);

/***** Variable declarations *****/

/* Semaphore signaling configurations were submitted to the TRX */
SemaphoreP_Handle configurationSem;

/* Semaphore signaling a complete command store to the TRX */
SemaphoreP_Handle cmdStoreCompleteSem;

/* LRF register configurations */
#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.h"
#define RF_CONFIG_SIZE (LRF_mainRegConfig_dualrxSunOqpsk2gfsk38p4kbps_byteCount)
#define RF_CONFIG_PTR  ((uint8_t *)LRF_mainRegConfig_dualrxSunOqpsk2gfsk38p4kbps)
#define FE_CONFIG_SIZE (LRF_frontendRegConfig_dualrxSunOqpsk2gfsk38p4kbps_byteCount)
#define FE_CONFIG_PTR  ((uint8_t *)LRF_frontendRegConfig_dualrxSunOqpsk2gfsk38p4kbps)


/* Handle to the RF driver object */
TRX_Host_Handle trxHostHandle;

/* Global Receive command */
TRX_Request_CommandStore cmdDualRx;

/* Receive buffer */
uint8_t rxBuffer[RX_PACKET_LENGTH] = {0};

/* RX packet counter */
uint32_t cntRxPackets = 0;

/******* Callbacks *******/

static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* If this loops, a typical application should post a semaphore and
     * call TRX_Host_eraseNv() */
    while(events & TRX_EventNvCorrupt);

    /* If this loops, a typical application should post a semaphore and
     * call TRX_Host_close(), then re-initialize the TRX */
    while(events & TRX_EventTransportError);
}

static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* Handle status error event - occurs when configuration operation fails */
    if(TRX_EventLastStatusError & events)
    {
        /* An error occurred during configuration store/erase/persist operation.
         * This should not happen in normal operation. Possible causes:
         * - Invalid configuration data
         * - Communication error with TRX
         * - TRX internal error
         *
         * In a typical application, you would handle this by logging the error
         * and potentially retrying the operation or notifying the user.
         */
        while(1);
    }

    if(TRX_EventNvEraseComplete & events)
    {
        SemaphoreP_post(configurationSem);
    }

    if(TRX_EventConfigStoreComplete & events)
    {
        SemaphoreP_post(configurationSem);
    }

    if(TRX_EventConfigPersistComplete & events)
    {
        SemaphoreP_post(configurationSem);
    }
}
static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* Handle status error event - occurs when RF mode set operation fails */
    if(TRX_EventLastStatusError & events)
    {
        /* An error occurred when setting the RF mode.
         * This should not happen in normal operation. Possible causes:
         * - Invalid RF mode parameter
         * - TRX not ready for mode change
         *
         * In a typical application, you would handle this by logging the error
         * and potentially retrying the operation.
         */
        while(1);
    }

    if(TRX_EventDeviceConfigStoreComplete & events)
    {
        SemaphoreP_post(configurationSem);
    }
}

static void receiveCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* Handle command status event - occurs when RX command finishes */
    if(TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        TRX_CommandStatus cmdStatus = pCmdStatusRequest->status;

        /* Check if the command finished successfully or with an error */
        if(TRX_CommandStatus_Finished != cmdStatus)
        {
            /* RX command exited unexpectedly. Possible reasons:
             * - TRX_CommandStatus_GracefulStopTimeout: RX timeout occurred
             * - TRX_CommandStatus_GracefulStopApi: Stopped via API call
             * - TRX_CommandStatus_HardStopApi: Hard stopped via API
             * - TRX_CommandStatus_RxErr: RX error occurred
             * - TRX_CommandStatus_Error_RxBufferCorruption: Buffer corruption
             * - TRX_CommandStatus_Error_RxFifo: FIFO error
             *
             * In a typical application, you would handle these errors appropriately,
             * such as restarting the RX operation or logging the error.
             */
            while(1);
        }
    }

    /* Handle command store complete event - occurs when command is stored on TRX */
    if(TRX_EventCmdStoreComplete & events)
    {
        /* Signal that storing the command is complete */
        SemaphoreP_post(cmdStoreCompleteSem);
    }

    /* Handle buffer overflow event - occurs when received data exceeds buffer size */
    if(TRX_EventStreamStoreOverflow & events)
    {
        /* RX buffer wasn't big enough to hold the received packet.
         * This should not happen in normal operation.
         *
         * To fix: Increase RX_PACKET_LENGTH to accommodate larger packets.
         * In a typical application, you would log this error and potentially
         * discard the packet or resize the buffer dynamically.
         */

        while(1);
    }

    /* Handle data received event - occurs when a packet is successfully received */
    if(TRX_EventFinalStreamStoreReceived & events)
    {
        /* SUN Packet data structure:
         * +-----------------+----------+---------+----------+----------------+
         * | modem mask (1B) | hdr (4B) | payload | rssi(1B) | timestamp (4B) |
         * +-----------------+----------+---------+----------+----------------+
         */
        /* Proprietary FSK Packet data structure:
         * +-----------------+----------+---------+----------+----------------+
         * | modem mask (1B) | Len (1B) | payload | rssi(1B) | timestamp (4B) |
         * +-----------------+----------+---------+----------+----------------+
         */
        if((uintptr_t)NULL != pCmdStoreOrData)
        {
            /* Parse the received packet to extract modem mask, header, payload, RSSI, and timestamp.
             * The packet data is located at pCmdStoreOrData:
             * - Modem Mask: First byte (TRX_RadioCommand_Modem, indicates which modem received the packet)
             * - Header:
             *     - SUN: Next 4 bytes (TRX_PayloadHeader)
             *     - Proprietary FSK: 1 byte (length field)
             * - Payload: Next N bytes (length specified in header)
             * - RSSI: 1 byte after payload (signed int8_t value in dBm)
             * - Timestamp: 4 bytes after RSSI (uint32_t value)
             *
             * Example of how to access the data for a SUN packet:
             *   uint8_t *modemMask = (uint8_t *)pCmdStoreOrData;
             *   TRX_PayloadHeader *header = (TRX_PayloadHeader *)(modemMask + MODEM_MASK_SIZE_BYTES);
             *   uint8_t *payload = (uint8_t *)header + sizeof(TRX_PayloadHeader);
             *   int8_t *rssi = (int8_t *)(payload + header->length);
             *   uint32_t *timestamp = (uint32_t *)(rssi + RSSI_SIZE_BYTES);
             */

            /* The modem mask is the first byte of the received packet */
            uint8_t modemMask = ((uint8_t *)pCmdStoreOrData)[0];

            /* Toggle LED based on which modem received the packet */
            if (modemMask & TRX_RadioCommand_Modem_OQPSK)
            {
                GPIO_toggle(CONFIG_GPIO_GLED);
            }
            else if (modemMask & TRX_RadioCommand_Modem_FSK)
            {
                GPIO_toggle(CONFIG_GPIO_RLED);
            }
            else
            {
                while(1);
            }
            cntRxPackets++;
        }
    }
}

/***** Function definitions *****/

void *mainThread(void *arg0)
{
    /* Configure the LED pin and turn it off */
    GPIO_setConfig(CONFIG_GPIO_RLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_RLED, CONFIG_GPIO_LED_OFF);

    /* Initialize the configuration semaphore */
    SemaphoreP_Params semParamsConfig;
    SemaphoreP_Params_init(&semParamsConfig);
    configurationSem = SemaphoreP_create(0, &semParamsConfig);

    /* Initialize the command store complete semaphore */
    SemaphoreP_Params semParamsCmdStore;
    SemaphoreP_Params_init(&semParamsCmdStore);
    cmdStoreCompleteSem = SemaphoreP_create(0, &semParamsCmdStore);

    /* Init the RF driver */
    TRX_Host_Params params = {
        .bitRate = PLATFORM_DEFAULT_SPI_BITRATE,
        .generalCb = generalCallback,
        .arg = (uintptr_t)NULL
    };
    // Reset TRX by toggling the reset pin
    GPIO_setConfig(TRX_RESET, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH);
    GPIO_write(TRX_RESET, 0);
    usleep(100); // keep reset low for 100us (empirically found)
    GPIO_write(TRX_RESET, 1);
    usleep(100); // wait for 100us after releasing reset

    trxHostHandle = TRX_Host_open(&params);

    TRX_Host_Status status;

    status = TRX_Host_eraseNv(trxHostHandle, configCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

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
        .rfMode = TRX_RfMode_PropFSKPeakAGC,
    };
    status = TRX_Host_storeDeviceConfig(trxHostHandle, deviceConfig, storeDeviceConfigCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    /* Load the front end configuration */
    status = TRX_Host_storeConfig(trxHostHandle, TRX_CONFIG_ID_FRONTEND, FE_CONFIG_PTR, FE_CONFIG_SIZE,
                                  FE_CONFIG_REFERENCE, true, configCallback, TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);
#ifdef TRX_CC1407
    status = TRX_Host_persistConfig(trxHostHandle, TRX_CONFIG_ID_FRONTEND);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);
#endif

    /* Load the phy configuration */
    status = TRX_Host_storeConfig(trxHostHandle, RF_CONFIG_ID,
                                  RF_CONFIG_PTR, RF_CONFIG_SIZE,
                                  RF_CONFIG_REFERENCE, false, configCallback,
                                  TRX_EventConfigStoreComplete|TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    status = TRX_Host_persistConfig(trxHostHandle, RF_CONFIG_ID);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    /* Create a Dual Rx command */
    cmdDualRx.slot = RX_CMD_SLOT;
    cmdDualRx.cmd_id = TRX_RadioCommand_DualReceive;
    cmdDualRx.enable_on_true = false;
    cmdDualRx.enable_on_false = false;
    cmdDualRx.enable_on_compare = false;
    cmdDualRx.trigger = Command_Trigger_Immediate;
    cmdDualRx.conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    cmdDualRx.allow_delay = false;
    cmdDualRx.params.dualRx.phy0.config_id = RF_CONFIG_ID;
    cmdDualRx.params.dualRx.stream_id = RX_STREAM_ID;
    cmdDualRx.params.dualRx.frequency0 = 868000000;
    cmdDualRx.params.dualRx.frequency1 = 869000000;
    cmdDualRx.params.dualRx.modem = TRX_RadioCommand_Modem_FSK | TRX_RadioCommand_Modem_OQPSK;
    cmdDualRx.params.dualRx.enable_mdr = false;
    cmdDualRx.params.dualRx.stream_early = true;
    cmdDualRx.params.dualRx.timeout = 0U;
    cmdDualRx.params.dualRx.repeat = true;
    cmdDualRx.params.dualRx.search_strategy = TRX_Rx_SearchStrategy_Sync;

    /* Register the RX buffer for received data */
    status = TRX_Host_registerRxStream(trxHostHandle, RX_STREAM_ID, rxBuffer,
                                       sizeof(rxBuffer), receiveCallback,
                                       TRX_EventFinalStreamStoreReceived);
    while(TRX_Host_Success != status);

    /* Store the RX command on the TRX and issue it to the radio */
    status = TRX_Host_storeCmds(trxHostHandle, &cmdDualRx, 1U, &cmdDualRx, receiveCallback,
                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(cmdStoreCompleteSem, SemaphoreP_WAIT_FOREVER);

    /* RX will repeat forever */
    while (1){};
}