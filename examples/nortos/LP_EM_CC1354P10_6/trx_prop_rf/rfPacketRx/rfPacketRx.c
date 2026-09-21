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

// LRF register configurations
#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_wisun.h"

/***** Defines *****/

/* Maximum payload in a receive operation */
#define RX_PAYLOAD_LENGTH   (76U)

/* The data received will contain both timestamp (4B) and RSSI (1B) information. */
#define RSSI_SIZE_BYTES         (1U)
#define TIMESTAMP_SIZE_BYTES    (4U)

/* Total size of data that can be received including HEADER + PAYLOAD + RSSI + TIMESTAMP */
#define RX_PACKET_LENGTH    (sizeof(TRX_PayloadHeader) + RX_PAYLOAD_LENGTH + RSSI_SIZE_BYTES + TIMESTAMP_SIZE_BYTES)

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

/* Size of the PHY configuration structure (found in rcl_settings_wisun.h) */
#define RF_CONFIG_SIZE (LRF_mainRegConfig_wisun_byteCount)

/* Pointer to the PHY configuration structure (found in rcl_settings_wisun.h) */
#define RF_CONFIG_PTR  ((uint8_t *)LRF_mainRegConfig_wisun)

/* Size of the Frontend Configuration structure (found in rcl_settings_wisun.h) */
#define FE_CONFIG_SIZE (LRF_frontendRegConfig_wisun_byteCount)
/* Pointer to Frontend Configuration structure (found in rcl_settings_wisun.h) */
#define FE_CONFIG_PTR  ((uint8_t *)LRF_frontendRegConfig_wisun)

/***** Prototypes *****/
static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);

static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg);

static void rxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                             TRX_Request *request, uint64_t events, uintptr_t arg);

static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg);

/***** Variable declarations *****/

/* Semaphore signaling configurations were submitted to the TRX */
SemaphoreP_Handle configurationSem;

/* Semaphore signaling RF mode set was completed on the TRX */
SemaphoreP_Handle setRfModeCompleteSem;

/* Semaphore signaling a completed command store on the TRX */
SemaphoreP_Handle cmdStoreCompleteSem;

/* Handle to the RF driver object */
TRX_Host_Handle trxHostHandle;

/* Global RX command */
TRX_Request_CommandStore cmdRx;

/* Receive buffer */
uint8_t rxBuffer[RX_PACKET_LENGTH] = {0};

/* RX packet counter */
uint32_t cntRxPackets = 0;

/******* Callbacks *******/

/**
 * @brief General callback for handling asynchronous TRX events
 *
 * This callback is registered during TRX_Host_open() and is invoked for general/error
 * events that don't map to a specific user-initiated command.
 *
 * @note The infinite loops below are placeholders for error recovery logic.
 *       The error handling recommendations are provided in the comments.
 */
static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /*
     * Non-volatile memory on the TRX is corrupted.
     * Recovery: Erase NV (TRX_Host_eraseNv()) and reconfigure the TRX from scratch.
     */
    if(events & TRX_EventNvCorrupt)
    {
        while(1);
    }

    /*
     * This is a critical error indicating the SPI link is broken.
     * Recovery: Close the driver (TRX_Host_close), reset the TRX hardware (by toggling its reset pin),
     * then re-open the driver (TRX_Host_open) and reconfigure the TRX from scratch.
     */
    if(events & TRX_EventTransportError)
    {
        while(1);
    }
}

/**
 * @brief Callback for handling all TRX configuration steps before reception
 *
 * This callback is registered when submitting configuration commands to the TRX.
 * It is re-used for NV erase (TRX_Host_eraseNv), configuration store (TRX_Host_storeConfig)
 * and configuration persist (TRX_Host_persistConfig)
 *
 * @note The infinite loops below are placeholders for error recovery logic.
 *       The error handling recommendations are provided in the comments.
 */
static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* Handle status error event - occurs when configuration operation fails */
    if(TRX_EventLastStatusError & events)
    {
        /* An error occurred during configuration store/erase/persist operation.
         * This should not happen in normal operation. Some possible causes are listed below.
         */
        TRX_Request_LastStatus *pLastStatus = (TRX_Request_LastStatus *)request;

        if (pLastStatus->status == TRX_STATUS_INVALID_PARAM)
        {
            /* Recovery:
             * For store, check that the configuration data is valid.
             * For persist, make sure the configuration is stored before persisting.
             */
            while(1);
        }
        else if (pLastStatus->status == TRX_STATUS_INVALID_STATE)
        {
            /* There was no available slot to store/persist the configuration.
             * Recovery:
             * For store, you can persist existing configurations to free up space.
             * For persist, you can erase NV to free up space.
             */
            while(1);
        }
    }

    if(TRX_EventNvEraseComplete & events)
    {
        /* From calling TRX_Host_eraseNv */
        SemaphoreP_post(configurationSem);
    }

    if(TRX_EventConfigStoreComplete & events)
    {
        /* From calling TRX_Host_storeConfig */
        SemaphoreP_post(configurationSem);
    }

    if(TRX_EventConfigPersistComplete & events)
    {
        /* From calling TRX_Host_persistConfig */
        SemaphoreP_post(configurationSem);
    }
}
/**
 * @brief Callback for handling RF mode switch (TRX_Host_utilSetRfMode) completion on the TRX.
 *        By default, the RF mode is set to TRX_RfMode_SUN
 *
 * This callback is registered when submitting a TRX_Request_UtilRfMode request to the TRX using TRX_Host_utilSetRfMode.
 *
 * @note The infinite loops below are placeholders for error recovery logic.
 *       The error handling recommendations are provided in the comments.
 */
static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* Handle status error event - occurs when RF mode set operation fails */
    if(TRX_EventLastStatusError & events)
    {
        /* An error occurred when setting the RF mode. */
        TRX_Request_LastStatus *pLastStatus = (TRX_Request_LastStatus *)request;

        if (pLastStatus->status == TRX_STATUS_INVALID_PARAM)
        {
            /* Invalid RF mode parameter was provided.
             * Recovery: Check that TRX_RfMode value is valid (SUN, PropFSK etc.)
             */
            while(1);
        }
        else if (pLastStatus->status == TRX_STATUS_INVALID_STATE)
        {
            /* TRX is not ready for mode change - radio command is running.
             * Recovery: Stop the running command, then retry the RF mode change.
             */
            while(1);
        }
    }

    if(TRX_EventDeviceConfigStoreComplete & events)
    {
        SemaphoreP_post(setRfModeCompleteSem);
    }
}

/**
 * @brief Callback for handling various events generated due to the RX command
 *
 * This callback is registered when storing the command (TRX_Host_storeCmds) or when registering the RX stream.
 *
 * @note The infinite loops below are placeholders for error recovery logic.
 *       The error handling recommendations are provided in the comments.
 */
static void rxCallback(TRX_Host_Handle handle, uintptr_t pCmdStoreOrData,
                             TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* Handle status error event - occurs when an operation fails */
    if(TRX_EventLastStatusError & events)
    {
        /* An error occurred during command or stream operation. */
        TRX_Request_LastStatus *pLastStatus = (TRX_Request_LastStatus *)request;

        if (pLastStatus->status == TRX_STATUS_INVALID_PARAM)
        {
           /* Causes for the error:
            *   Invalid slot provided. The slot might be out of range.
            *   Invalid RX parameters.
            * Recovery: Check the slot and command parameters being used are valid.
            */
            while(1);
        }
        else if (pLastStatus->status == TRX_STATUS_INVALID_STATE)
        {
            /* Causes for the error:
             * Another command is running in the slot.
             * Recovery: Stop the running command or use another slot.
             */
            while(1);
        }
    }

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

    /* Handle command store complete event - occurs when command is stored on TRX (TRX_Host_storeCmds)*/
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
        /* Packet data structure:
         * +----------+---------+----------+----------------+
         * | hdr (4B) | payload | rssi(1B) | timestamp (4B) |
         * +----------+---------+----------+----------------+
         */
        if((uintptr_t)NULL != pCmdStoreOrData)
        {
            /* Parse the received packet to extract header, payload, RSSI, and timestamp.
             * The packet data is located at pCmdStoreOrData:
             * - Header: First 4 bytes (TRX_PayloadHeader)
             * - Payload: Next N bytes (length specified in header)
             * - RSSI: 1 byte after payload (signed int8_t value in dBm)
             * - Timestamp: 4 bytes after RSSI (uint32_t value)
             *
             * Example of how to access the data:
             *   TRX_PayloadHeader *header = (TRX_PayloadHeader *)pCmdStoreOrData;
             *   uint8_t *payload = (uint8_t *)header + sizeof(TRX_PayloadHeader);
             *   int8_t *rssi = (int8_t *)(payload + header->length);
             *   uint32_t *timestamp = (uint32_t *)(rssi + RSSI_SIZE_BYTES);
             */

            /* Toggle LED to indicate packet received */
            GPIO_toggle(CONFIG_GPIO_RLED);
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

    /* Initialize the rf mode set complete semaphore */
    SemaphoreP_Params semParamsSetRfMode;
    SemaphoreP_Params_init(&semParamsSetRfMode);
    setRfModeCompleteSem = SemaphoreP_create(0, &semParamsSetRfMode);

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
    status = TRX_Host_storeDeviceConfig(trxHostHandle, deviceConfig, storeDeviceConfigCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(setRfModeCompleteSem, SemaphoreP_WAIT_FOREVER);

    status = TRX_Host_eraseNv(trxHostHandle, configCallback);
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

    /* Load the PHY configuration */
    status = TRX_Host_storeConfig(trxHostHandle,
                                  RF_CONFIG_ID,
                                  RF_CONFIG_PTR,
                                  RF_CONFIG_SIZE,
                                  RF_CONFIG_REFERENCE,
                                  false,
                                  configCallback,
                                  TRX_EventConfigStoreComplete|TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    status = TRX_Host_persistConfig(trxHostHandle, RF_CONFIG_ID);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    /* Create an RX command */
    cmdRx.slot = RX_CMD_SLOT;
    cmdRx.cmd_id = TRX_RadioCommand_Receive;
    cmdRx.enable_on_true = false;
    cmdRx.enable_on_false = false;
    cmdRx.enable_on_compare = false;
    cmdRx.trigger = Command_Trigger_Immediate;
    cmdRx.conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    cmdRx.allow_delay = false;
    cmdRx.params.rx.phy0.config_id = RF_CONFIG_ID;
    cmdRx.params.rx.phy0.option_mask = TRX_PHY_FEATURE_FSK_MODE_2B_WISUN;
    cmdRx.params.rx.stream_id = RX_STREAM_ID;
    cmdRx.params.rx.frequency = 920600;
    cmdRx.params.rx.modem = TRX_RadioCommand_Modem_FSK;
    cmdRx.params.rx.enable_mdr = false;
    cmdRx.params.rx.stream_early = true;
    cmdRx.params.rx.timeout = 0U;
    cmdRx.params.rx.repeat = true;
    cmdRx.params.rx.search_strategy = TRX_Rx_SearchStrategy_Sync;

    /* Register the RX buffer for received data */
    status = TRX_Host_registerRxStream(trxHostHandle,
                                       RX_STREAM_ID,
                                       rxBuffer,
                                       sizeof(rxBuffer),
                                       rxCallback,
                                       TRX_EventFinalStreamStoreReceived);
    while(TRX_Host_Success != status);

    /* Store the RX command to the TRX */
    status = TRX_Host_storeCmds(trxHostHandle,
                                &cmdRx,
                                1U,
                                &cmdRx,
                                rxCallback,
                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(cmdStoreCompleteSem, SemaphoreP_WAIT_FOREVER);

    /* RX will repeat forever */
    while (1){};
}