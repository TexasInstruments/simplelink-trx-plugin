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
#include <stdlib.h>

/* TI Drivers */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

/* Board Header files */
#include "ti_drivers_config.h"

/* TRX Host Header files */
#include <ti/trx/TRX.h>
#include <ti/trx/host/trx_host.h>
#include <ti/trx/host/hal/platform.h>

/* LRF register configurations */
#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.h"

/* Length of the payload to transmit */
#define PAYLOAD_LENGTH (20U)

/* Total length of the HEADER + PAYLOAD to transmit (FSK payload header is 1 Byte long) */
#define PACKET_LENGTH_OQPSK (sizeof(TRX_PayloadHeader) + PAYLOAD_LENGTH)
#define PACKET_LENGTH_FSK   (1 + PAYLOAD_LENGTH)

/* Set packet interval to 2s */
#define PACKET_INTERVAL     2

/* Slot for the TX cmds. Can be anything in the range 0, TRX_MAX_NUM_COMMANDS excluding TRX_MAX_NUM_COMMANDS */
#define TX_CMD_SLOT_OQPSK   (0U)
#define TX_CMD_SLOT_FSK     (1U)

/* ID of the packet data payload on the TRX. Can be anything in the range 0, TRX_MAX_STREAMS inclusive */
#define TX_STREAM_ID     (0U)

/* ID of the PHY configuration on the TRX. Can be anything in the range 1, TRX_MAX_CONFIGURATIONS inclusive */
#define RF_CONFIG_ID     (1U)

/* Reference of the Front End (FE) configuration on the TRX. Can be any 32-bit value */
#define FE_CONFIG_REFERENCE (0xFEFEFEFE)

/* Reference of the PHY configuration on the TRX. Can be any 32-bit value */
#define RF_CONFIG_REFERENCE (0xF000000D)

/* Size of the PHY configuration structure (found in rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.h) */
#define RF_CONFIG_SIZE (LRF_mainRegConfig_dualrxSunOqpsk2gfsk38p4kbps_byteCount)
#define FE_CONFIG_SIZE (LRF_frontendRegConfig_dualrxSunOqpsk2gfsk38p4kbps_byteCount)

/* Pointer to the PHY configuration structure (found in rcl_settings_dualrx_sun_oqpsk_2gfsk38p4kbps.h) */
#define RF_CONFIG_PTR  ((uint8_t *)LRF_mainRegConfig_dualrxSunOqpsk2gfsk38p4kbps)
#define FE_CONFIG_PTR  ((uint8_t *)LRF_frontendRegConfig_dualrxSunOqpsk2gfsk38p4kbps)

/***** Prototypes *****/
static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);

static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                           TRX_Request *request, uint64_t events, uintptr_t arg);

static void txCallback(TRX_Host_Handle handle, uintptr_t pCmdStore,
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

/* Semaphore signaling a completed stream store on the TRX */
SemaphoreP_Handle streamStoreCompleteSem;

/* Semaphore signaling TX command is done on the TRX and the status is available to read */
SemaphoreP_Handle txCmdStatusAvailableSem;




/* Handle to the RF driver object */
TRX_Host_Handle trxHostHandle;

/* Global TX command */
TRX_Request_CommandStore cmdTx;

/* Transmit packet buffer */
uint8_t txPacket[PACKET_LENGTH_OQPSK];

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
 * @brief Callback for handling all TRX configuration steps before transmission
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
 * @brief Callback for handling various events generated due to the TX command
 *
 * This callback is registered when storing the command (TRX_Host_storeCmds) or submitting the command (TRX_Host_submitCmd) to the TRX.
 *
 * @note The infinite loops below are placeholders for error recovery logic.
 *       The error handling recommendations are provided in the comments.
 */
static void txCallback(TRX_Host_Handle handle, uintptr_t pCmdStore,
                             TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* Handle status error event - occurs when an operation fails */
    if(TRX_EventLastStatusError & events)
    {
        /* An error occurred during command or stream store operation. */
        TRX_Request_LastStatus *pLastStatus = (TRX_Request_LastStatus *)request;

        if (pLastStatus->status == TRX_STATUS_INVALID_PARAM)
        {
           /* Causes for the error:
            *   Invalid slot provided. The slot might be out of range.
            *   Using TRX_TxPa_Low for an OFDM phy.
            *   Invalid Tx power setting.
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

    /* Handle command status event - occurs when TX command finishes */
    if(TRX_EventCmdStatus & events)
    {
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        TRX_CommandStatus cmdStatus = pCmdStatusRequest->status;

        /* Check if the command finished successfully or with an error */
        if(TRX_CommandStatus_Finished != cmdStatus)
        {
            /* TX command exited unexpectedly. Possible reasons:
             * - TRX_CommandStatus_GracefulStopApi: Stopped via API call
             * - TRX_CommandStatus_HardStopApi: Hard stopped via API
             * - TRX_CommandStatus_TxErr: TX error occurred
             * - TRX_CommandStatus_Error_TxFifo: FIFO error
             *
             * In a typical application, you would handle these errors appropriately,
             * such as retrying the transmission or logging the error.
             */
            while(1);
        }

        /* TX completed successfully - signal completion */
        SemaphoreP_post(txCmdStatusAvailableSem);
        if (pCmdStatusRequest->slot == TX_CMD_SLOT_OQPSK)
        {
            GPIO_toggle(CONFIG_GPIO_GLED);
        }
        else if (pCmdStatusRequest->slot == TX_CMD_SLOT_FSK)
        {
            GPIO_toggle(CONFIG_GPIO_RLED);
        }

    }

    /* Handle command store complete event - occurs when command is stored on TRX (TRX_Host_storeCmds)*/
    if(TRX_EventCmdStoreComplete & events)
    {
        /* Signal that storing the command is complete */
        SemaphoreP_post(cmdStoreCompleteSem);
    }

    /* Handle stream store complete event - occurs when packet data is stored on TRX (TRX_Host_storeStream) */
    if(TRX_EventStreamStoreComplete & events)
    {
        /* Signal that the stream (packet data) store is complete */
        SemaphoreP_post(streamStoreCompleteSem);
    }
}

/***** Function definitions *****/

void *mainThread(void *arg0)
{

    /* Configure the LED pin and turn it off */
    GPIO_setConfig(CONFIG_GPIO_GLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_GLED, CONFIG_GPIO_LED_OFF);

    /* Initialize the configuration semaphore */
    SemaphoreP_Params semParamsConfig;
    SemaphoreP_Params_init(&semParamsConfig);
    configurationSem = SemaphoreP_create(0, &semParamsConfig);

    /* Initialize the command store complete semaphore */
    SemaphoreP_Params semParamsCmdStore;
    SemaphoreP_Params_init(&semParamsCmdStore);
    cmdStoreCompleteSem = SemaphoreP_create(0, &semParamsCmdStore);

    /* Initialize the stream store complete semaphore */
    SemaphoreP_Params semParamsStreamStore;
    SemaphoreP_Params_init(&semParamsStreamStore);
    streamStoreCompleteSem = SemaphoreP_create(0, &semParamsStreamStore);

    /* Initialize the TX command status available semaphore */
    SemaphoreP_Params semParamstxCmdStatusAvailable;
    SemaphoreP_Params_init(&semParamstxCmdStatusAvailable);
    txCmdStatusAvailableSem = SemaphoreP_create(0, &semParamstxCmdStatusAvailable);

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
        .rfMode = TRX_RfMode_PropFSKPeakAGC,
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

    /* Create the OPQSK TX command */
    cmdTx.slot = TX_CMD_SLOT_OQPSK;
    cmdTx.cmd_id = TRX_RadioCommand_Transmit;
    cmdTx.enable_on_true = false;
    cmdTx.enable_on_false = false;
    cmdTx.enable_on_compare = false;
    cmdTx.trigger = Command_Trigger_Immediate;
    cmdTx.conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    cmdTx.allow_delay = false;
    cmdTx.params.tx.phy0.config_id = RF_CONFIG_ID;
    cmdTx.params.tx.pa = TRX_TxPa_High;
    cmdTx.params.tx.stream_id = TX_STREAM_ID;
    cmdTx.params.tx.frequency = 868000;
    cmdTx.params.tx.modem = TRX_RadioCommand_Modem_OQPSK;
    // cmdTx.params.tx.power.rawValue = TRX_MAX_POWER;
    cmdTx.params.tx.power.dBm = 21; // 21 dBm
    cmdTx.params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm

    /* Store the TX command to the TRX */
    status = TRX_Host_storeCmds(trxHostHandle,
                                &cmdTx,
                                1U,
                                NULL,
                                txCallback,
                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(cmdStoreCompleteSem, SemaphoreP_WAIT_FOREVER);

    /* Create the FSK TX command */
    /* Only modify the delta between the two TX commands */
    cmdTx.slot = TX_CMD_SLOT_FSK;
    cmdTx.params.tx.stream_id = TX_STREAM_ID;
    cmdTx.params.tx.frequency = 869000;
    cmdTx.params.tx.modem = TRX_RadioCommand_Modem_FSK;

    /* Store the TX command to the TRX */
    status = TRX_Host_storeCmds(trxHostHandle,
                                &cmdTx,
                                1U,
                                NULL,
                                txCallback,
                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(cmdStoreCompleteSem, SemaphoreP_WAIT_FOREVER);

    /* Send an OQPSK packet on even iterations and an FSK packet on odd iterations */
    int iteration = 0;
    while (1)
    {
        uint8_t cmdSlot = iteration % 2 == 0 ? TX_CMD_SLOT_OQPSK : TX_CMD_SLOT_FSK;
        uint8_t packetLength = iteration % 2 == 0 ? PACKET_LENGTH_OQPSK : PACKET_LENGTH_FSK;

        /* The transmitted packet consists of PHY header + payload.
         * Update the PHY header based on the selected modulation.
         * The payload is the same for both modulations.
         */
        if (cmdSlot == TX_CMD_SLOT_OQPSK)
        {
            /* Clear the packet header and fill out the required fields */
            *(TRX_PayloadHeader_SUNOQPSK *)txPacket = (TRX_PayloadHeader_SUNOQPSK){0};
            ((TRX_PayloadHeader_SUNOQPSK *)txPacket)->length = PAYLOAD_LENGTH; // This length excludes the header
            ((TRX_PayloadHeader_SUNOQPSK *)txPacket)->rate_mode = TRX_PayloadHeader_SunOQPSK_RateMode3_50kbps;
            ((TRX_PayloadHeader_SUNOQPSK *)txPacket)->modulation = TRX_PayloadHeader_Modulation_OQPSK;

            /* Payload starts after 4 bytes of header */
            memset(&txPacket[sizeof(TRX_PayloadHeader)], 0xAA, PAYLOAD_LENGTH);
        }
        else
        {
            /* Update the PHY header */
            txPacket[0] = PAYLOAD_LENGTH;

            /* Payload starts after 1 byte of header */
            memset(&txPacket[1], 0xAA, PAYLOAD_LENGTH);
        }

        /** Load the stream of data to transmit */
        status = TRX_Host_storeStream(trxHostHandle,
                                      TX_STREAM_ID,
                                      txPacket,
                                      packetLength,
                                      Stream_Retention_Flush_Streaming,
                                      txCallback,
                                      TRX_EventStreamStoreComplete);
        while(TRX_Host_Success != status);
        SemaphoreP_pend(streamStoreCompleteSem, SemaphoreP_WAIT_FOREVER);

        /* Submit the TX command to the TRX */
        status = TRX_Host_submitCmd(trxHostHandle, cmdSlot);
        while(TRX_Host_Success != status);

        /* Wait for TX to complete and status to be available */
        SemaphoreP_pend(txCmdStatusAvailableSem, SemaphoreP_WAIT_FOREVER);

        /* Wait for packet interval */
        sleep(PACKET_INTERVAL);
        iteration = (iteration + 1) % 2;
    }
}