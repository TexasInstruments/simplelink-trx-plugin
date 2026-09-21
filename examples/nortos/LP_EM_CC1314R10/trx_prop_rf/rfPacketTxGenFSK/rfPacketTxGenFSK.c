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

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

// LRF register configurations
#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_2gfsk50kbps.h"

/***** Defines *****/
/* Set to 1 to use the extended radio command (TRX_RadioCommand_ExtTransmit)
 * instead of the regular TRX_RadioCommand_Transmit. The extended command takes
 * frequency in Hz (vs kHz) and adds a fixed_length field. 
 */
#define USE_EXTENDED_COMMANDS (0)

/* Length of the payload to transmit */
#define TX_PAYLOAD_LENGTH (20U)

/* Total length of the HEADER + PAYLOAD to transmit : Out of Box GenFSK PHY uses 1 byte header to carry length info */
#define TX_PACKET_LENGTH (1 + TX_PAYLOAD_LENGTH)

/* Transmit frequency. Regular commands use kHz; extended commands use Hz. */
#define RF_FREQUENCY_KHZ (868000U)
#define RF_FREQUENCY_HZ  (RF_FREQUENCY_KHZ * 1000U)

/* Set packet interval to 500000 us or 500 ms */
#define PACKET_INTERVAL     500000

/* Slot for the TX cmd. Can be anything in the range 0, TRX_MAX_NUM_COMMANDS excluding TRX_MAX_NUM_COMMANDS */
#define TX_CMD_SLOT      (0U)

/* ID of the packet data payload on the TRX. Can be anything in the range 0, TRX_MAX_STREAMS inclusive */
#define TX_STREAM_ID     (0U)

/* ID of the PHY configuration on the TRX. Can be anything in the range 1, TRX_MAX_CONFIGURATIONS inclusive */
#define RF_CONFIG_ID     (1U)

/* Reference of the Front End (FE) configuration on the TRX. Can be any 32-bit value */
#define FE_CONFIG_REFERENCE (0xFEFEFEFE)

/* Reference of the PHY configuration on the TRX. Can be any 32-bit value */
#define RF_CONFIG_REFERENCE (0xF000000D)

/* Size of the PHY configuration structure (found in rcl_settings_2gfsk50kbps.h) */
#define RF_CONFIG_SIZE (LRF_mainRegConfig_2gfsk50kbps_byteCount)

/* Pointer to the PHY configuration structure (found in rcl_settings_2gfsk50kbps.h) */
#define RF_CONFIG_PTR  ((uint8_t *)LRF_mainRegConfig_2gfsk50kbps)

/* Size of Frontend Configuration structure (found in rcl_settings_2gfsk50kbps.h) */
#define FE_CONFIG_SIZE (LRF_frontendRegConfig_2gfsk50kbps_byteCount)

/* Pointer to Frontend Configuration structure (found in rcl_settings_2gfsk50kbps.h) */
#define FE_CONFIG_PTR  ((uint8_t *)LRF_frontendRegConfig_2gfsk50kbps)

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

/* Transmit packet buffer: Out of Box GenFSK PHY uses 1 byte header for length */
uint8_t txPacket[TX_PACKET_LENGTH];

static uint16_t seqNumber;
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
 * It is re-used for NV erase (TRX_Host_eraseNv) and configuration store (TRX_Host_storeConfig)
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
        /* An error occurred during configuration store/erase operation.
         * This should not happen in normal operation. Some possible causes are listed below.
         */
        TRX_Request_LastStatus *pLastStatus = (TRX_Request_LastStatus *)request;

        if (pLastStatus->status == TRX_STATUS_INVALID_PARAM)
        {
            /* Recovery:
             * For store, check that the configuration data is valid.
             */
            while(1);
        }
        else if (pLastStatus->status == TRX_STATUS_INVALID_STATE)
        {
            /* There was no available slot to store the configuration.
             * Recovery:
             * For store, you can persist existing configurations to free up space.
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
        // Post to a semaphore being pended on in main
        SemaphoreP_post(configurationSem);        
    }
}

/**
 * @brief Callback for handling RF mode switch (TRX_Host_utilSetRfMode) completion on the TRX.
 *        For GenFSK, the RF mode is set to TRX_RfMode_PropFSK
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

        /* TX completed successfully - signal completion and update statistics */
        SemaphoreP_post(txCmdStatusAvailableSem);
        GPIO_toggle(CONFIG_GPIO_GLED);
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
    // Disable standby on host if you want to enable standby on the TRX.
    (void)Power_disablePolicy(); 
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
        .rfMode = TRX_RfMode_PropFSK,
    };
    status = TRX_Host_storeDeviceConfig(trxHostHandle, deviceConfig, storeDeviceConfigCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(setRfModeCompleteSem, SemaphoreP_WAIT_FOREVER);

    /* Load the front end configuration */
    status = TRX_Host_storeConfig(trxHostHandle, 
                                  TRX_CONFIG_ID_FRONTEND, 
                                  FE_CONFIG_PTR, 
                                  FE_CONFIG_SIZE,
                                  FE_CONFIG_REFERENCE, 
                                  true, 
                                  configCallback, 
                                  TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    (void)SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);
#ifdef TRX_CC1407
    status = TRX_Host_persistConfig(trxHostHandle, TRX_CONFIG_ID_FRONTEND);
    while(TRX_Host_Success != status);
    (void)SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);
#endif

    /* Load the PHY configuration */
    status = TRX_Host_storeConfig(trxHostHandle,
                                  RF_CONFIG_ID,
                                  RF_CONFIG_PTR,
                                  RF_CONFIG_SIZE,
                                  RF_CONFIG_REFERENCE,
                                  false,
                                  configCallback,
                                  TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    status = TRX_Host_persistConfig(trxHostHandle, RF_CONFIG_ID);
    while(TRX_Host_Success != status);
    (void)SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    /* Create an extended Tx command
     * Extended Transmit: frequency is in Hz and a fixed packet length is set. 
     */
#if USE_EXTENDED_COMMANDS
    cmdTx.slot = TX_CMD_SLOT;
    cmdTx.enable_on_true = false;
    cmdTx.enable_on_false = false;
    cmdTx.enable_on_compare = false;
    cmdTx.trigger = Command_Trigger_Immediate;
    cmdTx.conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    cmdTx.allow_delay = false;
    cmdTx.cmd_id = TRX_RadioCommand_ExtTransmit;
    cmdTx.params.extTx.phy0.config_id = RF_CONFIG_ID;
    cmdTx.params.extTx.phy0.option_mask = TRX_PHY_FEATURE_WHITENING_DISABLED_2GFSK50KBPS;
    cmdTx.params.extTx.pa = TRX_TxPa_High; //see also TRX_TxPa in TRX.h
    cmdTx.params.extTx.stream_id = TX_STREAM_ID;
    cmdTx.params.extTx.frequency = RF_FREQUENCY_HZ;
    cmdTx.params.extTx.modem = TRX_RadioCommand_Modem_FSK;
    // cmdTx.params.extTx.power.rawValue = TRX_MAX_POWER;
    cmdTx.params.extTx.power.dBm = 22; // 22dBm
    cmdTx.params.extTx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
    cmdTx.params.extTx.fixed_length = TX_PACKET_LENGTH; // bytes incl header, excl CRC
#else
    cmdTx.slot = TX_CMD_SLOT;
    cmdTx.cmd_id = TRX_RadioCommand_Transmit;
    cmdTx.enable_on_true = false;
    cmdTx.enable_on_false = false;
    cmdTx.enable_on_compare = false;
    cmdTx.trigger = Command_Trigger_Immediate;
    cmdTx.conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    cmdTx.allow_delay = false;
    cmdTx.params.tx.phy0.config_id = RF_CONFIG_ID;
    cmdTx.params.tx.phy0.option_mask = TRX_PHY_FEATURE_WHITENING_DISABLED_2GFSK50KBPS;
    cmdTx.params.tx.pa = TRX_TxPa_High; //see also TRX_TxPa in TRX.h
    cmdTx.params.tx.stream_id = TX_STREAM_ID;
    cmdTx.params.tx.frequency = RF_FREQUENCY_KHZ;
    cmdTx.params.tx.modem = TRX_RadioCommand_Modem_FSK;
    // cmdTx.params.tx.power.rawValue = TRX_MAX_POWER;
    cmdTx.params.tx.power.dBm = 21; // 21 dBm
    cmdTx.params.tx.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
    #endif

    /* Store the TX command to the TRX */
    status = TRX_Host_storeCmds(trxHostHandle,
                                &cmdTx,
                                1U,
                                NULL,
                                txCallback,
                                TRX_EventCmdStatus|TRX_EventCmdStoreComplete|TRX_EventCmdSubmitComplete);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(cmdStoreCompleteSem, SemaphoreP_WAIT_FOREVER);

    /* Update the PHY header for GenFSK: 1 byte header contains the payload length */
    txPacket[0] = TX_PAYLOAD_LENGTH;

    while (1)
    {
        /* Create packet with incrementing sequence number and random payload
         * Payload starts after 1 byte of header */
        txPacket[1] = (uint8_t)(seqNumber >> 8);
        txPacket[2] = (uint8_t)(seqNumber++);
        uint8_t i;
        for (i = 3; i < TX_PACKET_LENGTH; i++)
        {
            txPacket[i] = rand();
        }
        /** Load the stream of data to transmit */
        status = TRX_Host_storeStream(trxHostHandle,
                                      TX_STREAM_ID,
                                      txPacket,
                                      sizeof(txPacket),
                                      Stream_Retention_Flush_Streaming,
                                      txCallback,
                                      TRX_EventStreamStoreComplete);
        while(TRX_Host_Success != status);
        SemaphoreP_pend(streamStoreCompleteSem, SemaphoreP_WAIT_FOREVER);

        /* Submit the TX command to the TRX */
        status = TRX_Host_submitCmd(trxHostHandle, TX_CMD_SLOT);
        while(TRX_Host_Success != status);

        /* Wait for TX to complete and status to be available */
        SemaphoreP_pend(txCmdStatusAvailableSem, SemaphoreP_WAIT_FOREVER);

        /* Wait for packet interval */
        usleep(PACKET_INTERVAL);
    }
}