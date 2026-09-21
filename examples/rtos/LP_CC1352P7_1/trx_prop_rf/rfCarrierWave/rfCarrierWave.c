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
#include <unistd.h>

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

// Slot for the TX cmd. Can be anything in the range 0, TRX_MAX_NUM_COMMANDS excluding TRX_MAX_NUM_COMMANDS
#define TX_CMD_SLOT      (0U)

// ID of the PHY configuration on the TRX. Can be anything in the range 1, TRX_MAX_CONFIGURATIONS inclusive
#define RF_CONFIG_ID     (1U)

// Reference of the Front End (FE) configuration on the TRX. Can be any 32-bit value
#define FE_CONFIG_REFERENCE (0xFEFEFEFE)

// Reference of the PHY configuration on the TRX. Can be any 32-bit value
#define RF_CONFIG_REFERENCE (0xF000000D)

/***** Prototypes *****/

static void generalCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                            TRX_Request *request, uint64_t events, uintptr_t arg);

static void configCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                                        TRX_Request *request, uint64_t events, uintptr_t arg);

static void transmitTestCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                                 TRX_Request *request, uint64_t events, uintptr_t arg);

static void storeDeviceConfigCallback(TRX_Host_Handle handle, uintptr_t pConfigData,
                                 TRX_Request *request, uint64_t events, uintptr_t arg);

/***** Variable declarations *****/

/* Semaphore signaling configurations were submitted to the TRX */
SemaphoreP_Handle configurationSem;

/* Semaphore signaling the device configuration was stored on the TRX */
SemaphoreP_Handle storeDeviceConfigCompleteSem;

// LRF register configurations
#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_wisun.h"
#define RF_CONFIG_SIZE (LRF_mainRegConfig_wisun_byteCount)
#define RF_CONFIG_PTR  ((uint8_t *)LRF_mainRegConfig_wisun)
#define FE_CONFIG_SIZE (LRF_frontendRegConfig_wisun_byteCount)
#define FE_CONFIG_PTR  ((uint8_t *)LRF_frontendRegConfig_wisun)


/* Handle to the RF driver object */
TRX_Host_Handle trxHostHandle;

/* Global Transmit command */
TRX_Request_CommandStore cmdTxTest;

/***** Function definitions *****/

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
    /* If this loops, something went wrong when a TRX_Request_ConfigStore was
     * sent to the TRX */
    while(TRX_EventLastStatusError & events);

    if(TRX_EventNvEraseComplete & events)
    {
        SemaphoreP_post(configurationSem);
    }

    if(TRX_EventConfigStoreComplete & events)
    {
        // Would typically post a semaphore being pended on elsewhere
        SemaphoreP_post(configurationSem);
    }

    if(TRX_EventConfigPersistComplete & events)
    {
        SemaphoreP_post(configurationSem);
    }
}

static void transmitTestCallback(TRX_Host_Handle handle, uintptr_t pCmdStore,
                                 TRX_Request *request, uint64_t events, uintptr_t arg)
{
    /* If this loops, something went wrong when the cmd was being stored or
     * submitted on the TRX */
    while(TRX_EventLastStatusError & events);

    if(TRX_EventCmdStatus & events)
    {
        /* Something went wrong check pCmdStatusRequest->status for the reason */
        TRX_Request_CommandStatus *pCmdStatusRequest = (TRX_Request_CommandStatus *)request;
        (void)pCmdStatusRequest;
    }
}

void *mainThread(void *arg0)
{
    /* Initialize the configuration semaphore */
    SemaphoreP_Params semParams;
    SemaphoreP_Params_init(&semParams);
    configurationSem = SemaphoreP_create(0,&semParams);

    /* Initialize the store device config complete semaphore */
    SemaphoreP_Params semParamsStoreDeviceConfig;
    SemaphoreP_Params_init(&semParamsStoreDeviceConfig);
    storeDeviceConfigCompleteSem = SemaphoreP_create(0, &semParamsStoreDeviceConfig);

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

    /* Store the device configuration (DIOs, clock, power mode, RF mode) */
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
    SemaphoreP_pend(storeDeviceConfigCompleteSem, SemaphoreP_WAIT_FOREVER);

    //erase NV
    status = TRX_Host_eraseNv(trxHostHandle, configCallback);
    while(TRX_Host_Success != status);
    SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);


    /* Load the front end configuration */
    status = TRX_Host_storeConfig(trxHostHandle, TRX_CONFIG_ID_FRONTEND, FE_CONFIG_PTR, FE_CONFIG_SIZE,
                                  FE_CONFIG_REFERENCE, true, configCallback, TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    (void)SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);    
#ifdef TRX_CC1407
    status = TRX_Host_persistConfig(trxHostHandle, TRX_CONFIG_ID_FRONTEND);
    while(TRX_Host_Success != status);
    (void)SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);
#endif

    /* Load the phy configuration */
    status = TRX_Host_storeConfig(trxHostHandle, RF_CONFIG_ID,
                                  RF_CONFIG_PTR, RF_CONFIG_SIZE,
                                  RF_CONFIG_REFERENCE, false, configCallback,
                                  TRX_EventConfigStoreComplete | TRX_EventConfigPersistComplete);
    while(TRX_Host_Success != status);
    (void)SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    status = TRX_Host_persistConfig(trxHostHandle, RF_CONFIG_ID);
    while(TRX_Host_Success != status);
    (void)SemaphoreP_pend(configurationSem, SemaphoreP_WAIT_FOREVER);

    /* Create a Tx Test command */
    cmdTxTest.slot = TX_CMD_SLOT;
    cmdTxTest.cmd_id = TRX_RadioCommand_TransmitTest;
    cmdTxTest.enable_on_true = false;
    cmdTxTest.enable_on_false = false;
    cmdTxTest.enable_on_compare = false;
    cmdTxTest.trigger = Command_Trigger_Immediate;
    cmdTxTest.conflict_policy = TRX_ConflictPolicy_AlwaysInterrupt;
    cmdTxTest.allow_delay = false;
    cmdTxTest.params.txTest.phy0.config_id = RF_CONFIG_ID;
    cmdTxTest.params.txTest.phy0.option_mask = TRX_PHY_FEATURE_FSK_MODE_2B_WISUN;
    cmdTxTest.params.tx.pa = TRX_TxPa_High; //see also TRX_TxPa in TRX.h
    cmdTxTest.params.txTest.frequency = 920600;
    cmdTxTest.params.txTest.modem = TRX_RadioCommand_Modem_FSK;
    //cmdTxTest.params.txTest.power.rawValue = TRX_MAX_POWER;
    cmdTxTest.params.txTest.power.dBm = 21; //21 dBm, 
    cmdTxTest.params.txTest.power.fraction = 1; // +0.5 dBm. Total power = 21.5dBm
    cmdTxTest.params.txTest.mode = TRX_TxTestMode_Unmodulated; // Unmodulated carrier wave at center frequency
    cmdTxTest.params.txTest.pattern = 0U; // 1-byte pattern modulated during transmission. Only used if mode = TRX_TxTestMode_Modulated

    /* Store the TX Test command to the TRX and submit it to the radio */
    status = TRX_Host_storeCmds(trxHostHandle, &cmdTxTest, 1U, &cmdTxTest,
                                transmitTestCallback, TRX_EventCmdStatus);
    while(TRX_Host_Success != status);

    /* TX Test command transmits forever */
    while(1);
}
