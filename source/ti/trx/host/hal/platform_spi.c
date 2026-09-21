/******************************************************************************
 Group: CMCU LPRF
 Target Device: cc13xx_cc26xx

 ******************************************************************************
 
 Copyright (c) 2021-2026, Texas Instruments Incorporated
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

/*!

@file platform_spi.c

@brief Platform specific SPI driver implementation

*/

/********************************** Includes **********************************/
/* Standard C Libraries */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

/* Module Header Files */
#include <ti/trx/TRX.h>
#include <ti/trx/host/hal/platform.h>
#include <ti/trx/host/hal/platform_spi.h>
#include <ti/trx/host/hal/platform_util.h>
#include <ti/trx/host/transport/spi_transport.h>

/* TI Drivers */
#include "ti_drivers_config.h"
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/timer/GPTimerCC26XX.h>
#include <ti/devices/cc13x2x7_cc26x2x7/driverlib/cpu.h>

/********************************** Defines ***********************************/

#define THIRTY_MS_IN_US (30000U)

#define PLAT_SPI_DATA_SIZE         (8U)
#define PLAT_SPI_FRAME_FORMAT      (SPI_POL0_PHA0)
#define PLAT_SPI_TRANSFER_MODE     (SPI_MODE_CALLBACK)
#define PLAT_SPI_NUM_TRANSACTIONS  (2U)
#define PLAT_SPI_TRX_READY_POLL_INTERVAL_US  (30U) 
#define PLAT_SPI_POCI_WAIT_TIMEOUT_US        (10000U)

/** CPU Frequency in Hertz */
#define CPU_FREQ_HZ     (48000000U)

/** Number of CPU ticks every microsecond */
#define CPU_TICKS_PER_US  (CPU_FREQ_HZ/1000000U)

#define PLAT_SPI_OPEN_MASK            (0b00000001)
#define PLAT_SPI_ACTIVE_MASK          (0b00000010)
#define PLAT_SPI_TRX_READY_MASK       (0b00000100)
#define PLAT_SPI_IS_OPEN(handle)      (handle->status & PLAT_SPI_OPEN_MASK)
#define PLAT_SPI_IS_ACTIVE(handle)    (handle->status & PLAT_SPI_ACTIVE_MASK)
#define PLAT_SPI_IS_TRX_READY(handle) (handle->status & PLAT_SPI_TRX_READY_MASK)
#define PLAT_SPI_SET_OPEN(handle)     (handle->status |= PLAT_SPI_OPEN_MASK)
#define PLAT_SPI_SET_NOT_OPEN(handle) (handle->status &= ~(PLAT_SPI_OPEN_MASK))
#define PLAT_SPI_SET_ACTIVE(handle)   (handle->status |= PLAT_SPI_ACTIVE_MASK)
#define PLAT_SPI_SET_INACTIVE(handle) (handle->status &= ~(PLAT_SPI_ACTIVE_MASK))
#define PLAT_SPI_SET_TRX_READY(handle) (handle->status |= PLAT_SPI_TRX_READY_MASK)
#define PLAT_SPI_SET_TRX_NOT_READY(handle) (handle->status &= ~(PLAT_SPI_TRX_READY_MASK))

const uint8_t EMPTY_ARRAY[(TRX_SPIBLOCK_SIZE + sizeof(SPI_Transport_Container) - sizeof(TRX_SpiBlock))] = {0x00};

/********************************** Structs ***********************************/

/**
 *  @brief Object global to the SPI platform module used to organize important
 *  information regarding operation.
 */
typedef struct Plat_SPI_Object_t
{
    /** Bitmask tracking the status of the platform spi module including
     * MSB to LSB TRX_READY|ACTIVE|OPEN
     */
    volatile uint8_t status;
    /** Handle returned from call to SPI_open() */
    SPI_Handle spiHandle;
    /** Handle returned from call to GPTimerCC26XX_open() */
    GPTimerCC26XX_Handle timerHandle;
    /** Callback executed on completion of user request to transmit/receive data */
    platSpiTransactionCompleteCallback pTransactionCompleteCb;
    /** Callback executed when the TRX INT line is asserted */
    platSpiIntAssertedCallback pIntAssertedCb;
    /** Callback executed when the TRX POCI line is pulled low after CS is deasserted */
    platSpiTrxReadyCallback pTrxReadyCb;
    /** TI SPI driver transaction objects */
    SPI_Transaction spiTransaction[PLAT_SPI_NUM_TRANSACTIONS];
    /** Tracks the transmitBufLen provided to Plat_SPI_startTransaction() */
    uint16_t transmitLen;
    /** Tracks the receiveBufLen provided to Plat_SPI_startTransaction() */
    uint16_t receiveLen;
    /** Pointer to the last SPI_Transaction being used for a given operation */
    SPI_Transaction *pLastSPITransaction;
    /** Optional argument passed to each callback from this module */
    uintptr_t arg;
} Plat_SPI_Object;

/********************************* Prototypes *********************************/

/**
 * @brief Callback executed by the TI SPIDriver to indicate status changes
 * to the transactions submitted via SPI_transfer()
 *
 * This function will only take action when the last SPI transaction triggered
 * execution. It verifies the status of all transactions then executes the
 * user callback with the appropriate status.
 *
 * @note Calling context: HWI
 *
 * @param[in] handle SPI_Handle passed to SPI_transfer(). Unused.
 * @param[in] pTransaction  Pointer to the SPI_Transaction passed to
 *                          SPI_transfer()
 */
static void spiDriverCallback(SPI_Handle handle, SPI_Transaction *pTransaction);

/**
 * @brief Re-apply the internal pull-up on the POCI pin without disturbing the
 * SPI RX mux.
 *
 * Why: the SDK's SPI controller driver calls initIO() on every transaction
 * start (via enableSPIAndMux) and on standby wake, which rewrites the POCI
 * IOC register with GPIO_CFG_INPUT (pull-none), clobbering any pull setting
 * we or SysConfig applied. Without this the line floats between transactions
 * when the TRX tristates POCI.
 */
static inline void restorePociPullUp(void)
{
    uint32_t pociMux = GPIO_getMux(CONFIG_SPI_POCI);
    (void)GPIO_setConfigAndMux(CONFIG_SPI_POCI, GPIO_CFG_IN_PU, pociMux);
}

/**
 * @brief Callback executed by the TI GPTIMER driver
 *
 * @note Calling context: HWI
 *
 * @param[in] handle Handle to the GPTIMER instance
 * @param[in] interruptMask Event that triggered interrupt
 */
static void trxReadyCb(GPTimerCC26XX_Handle handle,
                       GPTimerCC26XX_IntMask interruptMask);

/**
 * @brief Callback executed by the TI GPIO driver when the TRX interrupt
 * line is asserted
 *
 * @note Calling context: HWI
 *
 * @param[in] index GPIO index. This is the same index that was passed to
 * GPIO_setCallback(). This allows you to use the same callback function for
 * multiple GPIO interrupts, by using the index to identify the GPIO that
 * caused the interrupt.
 */
static void intAssertedCallback(uint_least8_t index);

/**
 * @brief Start the timer to poll the POCI line being driven low after CS
 * deassertion
 *
 * @param handle Handle to the platform SPI module
 */
static void startReadyTimer(Plat_SPI_Handle handle);

/**
 * @brief Initialize the INT, CS and TRX RESET gpio
 *
 * @param handle Handle to the platform SPI module
 * @return int_fast8_t Less than 0 on failure, otherwise greater than or equal to 0
 */
static int_fast8_t initGpio(Plat_SPI_Handle handle);

/**
 * @brief Initialize the SPI interface to the provided bitrate
 *
 * @param handle Handle to the platform SPI module
 * @param bitRate The desired bitrate in hertz
 * @return int_fast8_t Less than 0 on failure, otherwise greater than or equal to 0
 */
static int_fast8_t initSpi(Plat_SPI_Handle handle, uint32_t bitRate);

/**
 * @brief Initialize the timer interface
 *
 * @param handle Handle to the platform SPI module
 * @return int_fast8_t Less than 0 on failure, otherwise greater than or equal to 0
 */
static int_fast8_t initTimer(Plat_SPI_Handle handle);

/**
 * @brief Send a dummy SPI transaction to the TRX during initialization to
 * force the SCLK line to the correct polarity
 *
 * @param handle Handle to the platform SPI module
 * @return int_fast8_t Less than 0 on failure, otherwise greater than or equal to 0
 */
static int_fast8_t dummySpiTransaction(Plat_SPI_Handle handle);

/*************************** Variable Declarations ****************************/

/**
 * Object global to the SPI platform module used to organize important
 * information regarding operation.
 */
static Plat_SPI_Object Plat_SPI_object =
{
    .status = 0,
    .spiHandle = NULL,
    .pTransactionCompleteCb = NULL,
    .pIntAssertedCb = NULL,
    .pTrxReadyCb = NULL,
    .spiTransaction = { {0U} },
    .transmitLen =  0U,
    .receiveLen = 0U,
};

/**************************** Function Definitions ****************************/

static void spiDriverCallback(SPI_Handle handle, SPI_Transaction *pTransaction)
{
    (void)handle; // Not using the handle argument

    uint8_t i;
    uint8_t numTransactions;
    Plat_SPI_Status status = Plat_SPI_Success;

    if(NULL != pTransaction)
    {
        Plat_SPI_Handle spiHandle = (Plat_SPI_Handle)pTransaction->arg;

        if(NULL != spiHandle)
        {
            if(pTransaction == spiHandle->pLastSPITransaction)
            {
                uint8_t  *pTransmitBuf   = spiHandle->spiTransaction[0U].txBuf;
                uint16_t  transmitBufLen = spiHandle->transmitLen;
                uint8_t  *pReceiveBuf    = spiHandle->spiTransaction[0U].rxBuf;
                uint16_t  receiveBufLen  = spiHandle->receiveLen;

                // Check for failure to send any of the transactions
                numTransactions = (uint8_t)((((uint32_t)pTransaction - (uint32_t)&spiHandle->spiTransaction[0U])/sizeof(SPI_Transaction)) + 1U);
                for(i = 0U; i < numTransactions; i++)
                {
                    if(SPI_TRANSFER_CANCELED == spiHandle->spiTransaction[i].status)
                    {
                        status = Plat_SPI_Canceled;
                        break;

                    }
                    else if(SPI_TRANSFER_COMPLETED != spiHandle->spiTransaction[i].status)
                    {
                        // General catch-all for failures
                        status = Plat_SPI_Failed;
                        break;
                    }
                    else
                    {
                        status = Plat_SPI_Success;
                    }
                }

                PLAT_SPI_SET_INACTIVE(spiHandle);

                /* SDK's enableSPIAndMux -> initIO clobbered POCI's pull-up at
                 * transaction start. Restore it now that the transaction is
                 * complete so the line stays high until the next transaction. */
                restorePociPullUp();

                if(EMPTY_ARRAY == pTransmitBuf)
                {
                    pTransmitBuf = NULL;
                }

                // Workaround for TIDRIVERS-5605: Transmit a dummy byte to set SCLK idle value
                if(NULL != spiHandle->pTransactionCompleteCb)
                {
                    // Execute upper layer callback
                    spiHandle->pTransactionCompleteCb(pTransmitBuf, transmitBufLen,
                                                    pReceiveBuf, receiveBufLen,
                                                    status, spiHandle->arg);
                }
                else
                {
                    PLAT_SPI_SET_OPEN(spiHandle);
                }
            }
        }
    }
}

static void startReadyTimer(Plat_SPI_Handle handle)
{
    uint32_t timeTick = (CPU_TICKS_PER_US * PLAT_SPI_TRX_READY_POLL_INTERVAL_US) - 1U;
    GPTimerCC26XX_stop(handle->timerHandle);              // stop first
    uint32_t timeout = timeTick + (uint32_t)GPTimerCC26XX_getValue(handle->timerHandle);
    GPTimerCC26XX_setMatchValue(handle->timerHandle, timeout); // set match on stopped timer
    GPTimerCC26XX_start(handle->timerHandle);             // then start
}

static void trxReadyCb(GPTimerCC26XX_Handle handle,
                      GPTimerCC26XX_IntMask interruptMask)
{
    Plat_SPI_Handle spiHandle = &Plat_SPI_object;
    /* Stop the periodic timer so the match interrupt only fires once per
     * startReadyTimer call. The next startReadyTimer stops+restarts, so this
     * is safe. Mirrors intAssertedCallback which also stops the timer once
     * TRX_READY is asserted. */
    GPTimerCC26XX_stop(handle);
    if(!PLAT_SPI_IS_TRX_READY(spiHandle))
    {
        PLAT_SPI_SET_TRX_READY(spiHandle);
        if(NULL != spiHandle->pTrxReadyCb)
        {
            spiHandle->pTrxReadyCb(spiHandle->arg);
        }
    }
}

static void intAssertedCallback(uint_least8_t index)
{
    // Get the Plat_IO_Instance that's attached to the GPIO as a user arg
    Plat_SPI_Handle handle = (Plat_SPI_Handle)GPIO_getUserArg(index);

    if((index == TRX_HOST_SPI_INT) && (NULL != handle->pIntAssertedCb))
    {
        GPTimerCC26XX_stop(handle->timerHandle);
        PLAT_SPI_SET_TRX_READY(handle);

        handle->pIntAssertedCb(handle->arg);
    }
}

static int_fast8_t initGpio(Plat_SPI_Handle handle)
{
    int_fast8_t result = -1;

    // Initialize INT and CS gpios
    int_fast16_t  gpioStatus;
    GPIO_init(); // Initialize TI GPIO driver
    gpioStatus = GPIO_setConfig(TRX_HOST_SPI_CS,
                                (GPIO_CFG_OUTPUT | GPIO_CFG_OUT_STR_HIGH  | GPIO_CFG_OUT_HIGH));
    gpioStatus |= GPIO_setConfig(TRX_RESET,
                                (GPIO_CFG_OUTPUT | GPIO_CFG_OUT_STR_HIGH  | GPIO_CFG_OUT_LOW));
    gpioStatus |= GPIO_setConfig(TRX_HOST_SPI_INT,
                                (GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING));

    // Set module handle as user argument so it can be accessed
    // in TI GPIO driver callback
    GPIO_setUserArg(TRX_HOST_SPI_INT, (void *)handle);

    // Set callback with TI GPIO driver and clear any pending interrupts
    GPIO_setCallback(TRX_HOST_SPI_INT, intAssertedCallback);
    GPIO_clearInt(TRX_HOST_SPI_INT);

    // Enable the interrupt
    GPIO_enableInt(TRX_HOST_SPI_INT);

    if(GPIO_STATUS_SUCCESS == gpioStatus)
    {
        // Success
        result = 0;
    }

    return(result);
}

static int_fast8_t initSpi(Plat_SPI_Handle handle, uint32_t bitRate)
{
    int_fast8_t result = -1;

    /* Initialize SPI handle with controller mode */
    SPI_Params spiParams;
    SPI_init();
    SPI_Params_init(&spiParams);

    spiParams.transferMode        = PLAT_SPI_TRANSFER_MODE;
    spiParams.transferCallbackFxn = spiDriverCallback;
    spiParams.mode                = SPI_CONTROLLER;
    spiParams.dataSize            = PLAT_SPI_DATA_SIZE;
    spiParams.bitRate             = bitRate;
    spiParams.frameFormat         = PLAT_SPI_FRAME_FORMAT;

    handle->spiHandle = SPI_open(TRX_HOST_SPI_CONTROLLER, &spiParams);

    if(NULL != handle->spiHandle)
    {
        restorePociPullUp();

        // Success
        result = 0;
    }

    return(result);
}

static int_fast8_t initTimer(Plat_SPI_Handle handle)
{
    int_fast8_t result = -1;

    /* Setup GPTimer in
    * - 32-bit mode (timer A)
    * - up-counting direction (will wrap around)
    * - stop counting when debug single-stepping
    * - one-shot
    */
    GPTimerCC26XX_Params params;
    GPTimerCC26XX_Params_init(&params);
    params.width          = GPT_CONFIG_32BIT;
    params.direction      = GPTimerCC26XX_DIRECTION_UP;
    params.debugStallMode = GPTimerCC26XX_DEBUG_STALL_ON;
    /* PERIODIC (not ONESHOT): GPTimerCC26XX_close's resetHw leaves TAV at
     * 0xFFFFFFFF (== TAILR top). In ONESHOT UP that state stops the timer
     * on the first clock, so on re-open the match interrupt never fires and
     * Plat_SPI_open hangs at the TRX-ready busy-wait. In PERIODIC UP the
     * counter freely wraps from 0xFFFFFFFF to 0 and reaches the match value
     * as expected. trxReadyCb is idempotent (only sets TRX_READY the first
     * time) so re-fires each 2^32 ticks are harmless. */
    params.mode           = GPT_MODE_PERIODIC;
    /* 0 -> CC26X2R1_LAUNCHXL_GPTIMER0A */
    handle->timerHandle = GPTimerCC26XX_open(CONFIG_GPTIMER_0, &params);

    if(NULL != handle->timerHandle)
    {
        GPTimerCC26XX_stop(handle->timerHandle);
        /* register an internal ISR with the GP-Timer module */
        GPTimerCC26XX_registerInterrupt(handle->timerHandle,
                                        trxReadyCb,
                                        (GPTimerCC26XX_IntMask)GPT_INT_MATCH);
        result = 0; // Success
    }

    return(result);
}

static int_fast8_t dummySpiTransaction(Plat_SPI_Handle handle)
{
    int_fast8_t result = -1;

    // Workaround for TIDRIVERS-5605: Transmit a dummy byte to set SCLK idle value
    handle->spiTransaction[0].count  = 1;
    handle->spiTransaction[0].txBuf  = (uint8_t *)EMPTY_ARRAY;
    handle->spiTransaction[0].rxBuf  = NULL;
    handle->spiTransaction[0].arg = (void *)handle;
    handle->pLastSPITransaction = &(handle->spiTransaction[0]);
    bool spiTransferResult = SPI_transfer(handle->spiHandle,
                                        (SPI_Transaction *)&handle->spiTransaction[0]);

    if(spiTransferResult)
    {
        // Callback will set driver open
        while(!PLAT_SPI_IS_OPEN(handle));
        result = 0; // Success
    }

    return(result);
}

void Plat_SPI_Params_init(Plat_SPI_Params *pParams)
{
    if(NULL != pParams)
    {
        pParams->bitRate = PLATFORM_DEFAULT_SPI_BITRATE;
        pParams->arg = (uintptr_t)NULL;
        pParams->pTransactionCompleteCb = NULL;
        pParams->pTrxReadyCallback = NULL;
        pParams->pIntAssertedCb = NULL;
    }
}

Plat_SPI_Status Plat_SPI_assertCs(Plat_SPI_Handle handle)
{
    unsigned int key;
    Plat_Util_startCriticalSection((uintptr_t)&key);
    Plat_SPI_Status status = Plat_SPI_Invalid_Handle;

    if(NULL != handle)
    {
        if(PLAT_SPI_IS_OPEN(handle))
        {
            if(PLAT_SPI_IS_TRX_READY(handle))
            {
                GPIO_write(TRX_HOST_SPI_CS, 0U);
                PLAT_SPI_SET_TRX_NOT_READY(handle);

                // Wait for the TRX to drive POCI low, signalling it is ready
                // to receive. This handshake is tied to the CS falling edge;
                // continuation transactions reuse the already-low CS and must
                // not repeat it (see spi_transport transactionCompleteCb).
                uint32_t elapsedUs = 0U;
                status = Plat_SPI_Success;
                while(0U != GPIO_read(CONFIG_SPI_POCI))
                {
                    Plat_Util_blockCPU(1U);
                    elapsedUs++;
                    if(elapsedUs >= PLAT_SPI_POCI_WAIT_TIMEOUT_US)
                    {
                        // Undo CS assertion and restart the ready timer so
                        // the caller can retry cleanly.
                        GPIO_write(TRX_HOST_SPI_CS, 1U);
                        startReadyTimer(handle);
                        status = Plat_SPI_Failed;
                        break;
                    }
                }
            }
            else
            {
                status = Plat_SPI_Cs_Not_Ready;
            }
        }
        else
        {
            status = Plat_SPI_Failed;
        }
    }
    Plat_Util_endCriticalSection((uintptr_t)&key);

    return(status);
}

Plat_SPI_Status Plat_SPI_deassertCs(Plat_SPI_Handle handle)
{
    unsigned int key;
    Plat_Util_startCriticalSection((uintptr_t)&key);
    Plat_SPI_Status status = Plat_SPI_Invalid_Handle;

    if(NULL != handle)
    {
        if(PLAT_SPI_IS_OPEN(handle))
        {
            GPIO_write(TRX_HOST_SPI_CS, 1U);
            status = Plat_SPI_Success;
            PLAT_SPI_SET_TRX_NOT_READY(handle);

            // Start the timer to guard the CS being asserted before the TRX is ready
            startReadyTimer(handle);
        }
        else
        {
            status = Plat_SPI_Failed;
        }
    }
    Plat_Util_endCriticalSection((uintptr_t)&key);

    return(status);
}

bool Plat_SPI_isTrxReady(Plat_SPI_Handle handle)
{
    unsigned int key;
    Plat_Util_startCriticalSection((uintptr_t)&key);
    bool ready = false;

    if(NULL != handle)
    {
        if(PLAT_SPI_IS_OPEN(handle))
        {
            if(PLAT_SPI_IS_TRX_READY(handle))
            {
                ready = true;
            }
            else
            {
                ready = false;
            }
        }
        else
        {
            ready = false;
        }
    }
    Plat_Util_endCriticalSection((uintptr_t)&key);

    return(ready);
}

Plat_SPI_Handle Plat_SPI_open(const Plat_SPI_Params *pParams)
{
    Plat_SPI_Handle handle = &Plat_SPI_object;

    if(PLAT_SPI_IS_OPEN(handle) || (NULL == pParams))
    {
        handle = NULL;
        goto endSpiOpen;
    }
    else if((NULL == pParams->pTrxReadyCallback) ||
            (NULL == pParams->pIntAssertedCb) ||
            (NULL == pParams->pTransactionCompleteCb))
    {
        handle = NULL;
        goto endSpiOpen;
    }
    handle->status = 0;

    // Initialize INT, CS, and RST gpios
    int_fast8_t gpioStatus = initGpio(handle);
    if(0 > gpioStatus)
    {
        // Could not set configuration for IO
        handle = NULL;
        goto endSpiOpen;
    }

    int_fast8_t timerStatus = initTimer(handle);
    if(0 > timerStatus)
    {
        // Could not set configuration for timer
        handle = NULL;
        goto endSpiOpen;
    }

    /* Initialize SPI handle with controller mode */
    int_fast8_t spiResult = initSpi(handle, pParams->bitRate);
    if(0 > spiResult)
    {
        // Failed allocating SPI instance
        handle = NULL;
        goto endSpiOpen;
    }

    // Erase anything in the SPI transaction array
    memset(&(handle->spiTransaction[0U]), 0U,
            (PLAT_SPI_NUM_TRANSACTIONS * sizeof(SPI_Transaction)));
    // Set no callbacks
    handle->pTransactionCompleteCb = NULL;
    handle->pIntAssertedCb = NULL;
    handle->pTrxReadyCb = NULL;

    int_fast8_t dummyTransactionResult = dummySpiTransaction(handle);
    if(0 > dummyTransactionResult)
    {
        handle = NULL;
        goto endSpiOpen;
    }

    handle->pTransactionCompleteCb = pParams->pTransactionCompleteCb;
    handle->pIntAssertedCb = pParams->pIntAssertedCb;
    handle->transmitLen =  0U;
    handle->receiveLen = 0U;
    handle->arg = pParams->arg;
    PLAT_SPI_SET_INACTIVE(handle);
    PLAT_SPI_SET_OPEN(handle);

    // Wait for TRX to startup
    handle->pTrxReadyCb = NULL;
    PLAT_SPI_SET_TRX_NOT_READY(handle);
    GPIO_write(TRX_RESET, 1U); // Release TRX from reset
    startReadyTimer(handle);

    while(!PLAT_SPI_IS_TRX_READY(handle));
    handle->pTrxReadyCb = pParams->pTrxReadyCallback;

    // Wait until TRX is ready
    Plat_Util_blockCPU(THIRTY_MS_IN_US);

endSpiOpen:
    return(handle);
}

Plat_SPI_Status Plat_SPI_close(Plat_SPI_Handle handle)
{
    Plat_SPI_Status status = Plat_SPI_Invalid_Handle;

    if(NULL != handle)
    {
        if(PLAT_SPI_IS_OPEN(handle))
        {
            if(PLAT_SPI_IS_ACTIVE(handle))
            {
                SPI_transferCancel(handle->spiHandle);
                // Execution will switch to SPI driver callback after canceling
            }

            // Close SPI interface
            SPI_close(handle->spiHandle);
            // Stop the gptimer
            GPTimerCC26XX_stop(handle->timerHandle);
            // Unregister the interrupt
            GPTimerCC26XX_unregisterInterrupt(handle->timerHandle);
            // Close the GPtimer
            GPTimerCC26XX_close(handle->timerHandle);

            // Disable the interrupt on the INT line
            GPIO_disableInt(TRX_HOST_SPI_INT);

            PLAT_SPI_SET_NOT_OPEN(handle);
            status = Plat_SPI_Success;
        }
        else
        {
            // Already closed
            status = Plat_SPI_Success;
        }
    }

    return(status);
}

Plat_SPI_Status Plat_SPI_startTransaction(Plat_SPI_Handle handle,
                                            uint8_t *pTransmitBuf,
                                            uint16_t transmitBufLen,
                                            uint8_t *pReceiveBuf,
                                            uint16_t receiveBufLen)
{
    uint8_t spiTransactionIndex;
    Plat_SPI_Status status = Plat_SPI_Success;
    bool spiTransferResult;

    if(NULL == handle)
    {
        status = Plat_SPI_Invalid_Handle;
    }
    else if(PLAT_SPI_IS_ACTIVE(handle))
    {
        // Platform SPI is busy transmitting/receiving
        status = Plat_SPI_Busy;
    }
    else if(!PLAT_SPI_IS_OPEN(handle))
    {
        // Platform SPI hasn't been opened yet
        status = Plat_SPI_Failed;
    }
    else if(((0 < transmitBufLen) && (NULL == pTransmitBuf)) ||
            ((0 < receiveBufLen) && (NULL == pReceiveBuf)))
    {
        // A NULL buffer was provided but it's associated length was > 0
        status = Plat_SPI_Failed;
    }
    else
    {
        spiTransactionIndex = 0;
        handle->transmitLen = transmitBufLen;
        handle->receiveLen = receiveBufLen;

        if((0 < receiveBufLen) && (0 < transmitBufLen))
        {
            // This is a full-duplex transmit/receive, rx/tx data must be split
            // evenly across SPI transactions

            // Populate the first SPI transaction with the smaller of the receive/transmit buffers
            handle->spiTransaction[spiTransactionIndex].count  = transmitBufLen > receiveBufLen ? receiveBufLen : transmitBufLen;
            handle->spiTransaction[spiTransactionIndex].txBuf  = pTransmitBuf;
            handle->spiTransaction[spiTransactionIndex].rxBuf  = pReceiveBuf;
            handle->spiTransaction[spiTransactionIndex].arg = (void *)handle;

            // If the transfer is symmetric, note in the handle that this cb
            // for this transaction will be the last
            if(transmitBufLen == receiveBufLen)
            {
                handle->pLastSPITransaction = &(handle->spiTransaction[spiTransactionIndex]);
            }

            // Initiate transfer of first transaction
            spiTransferResult = SPI_transfer(handle->spiHandle, (SPI_Transaction *)&(handle->spiTransaction[spiTransactionIndex]));

            if(false == spiTransferResult)
            {
                status = Plat_SPI_Failed;
            }
            else
            {
                PLAT_SPI_SET_ACTIVE(handle);
            }
            spiTransactionIndex++;

            if((transmitBufLen != receiveBufLen) && (status != Plat_SPI_Failed))
            {
                handle->spiTransaction[spiTransactionIndex].count  = transmitBufLen > receiveBufLen ?
                        (transmitBufLen - receiveBufLen) : (receiveBufLen - transmitBufLen);
                handle->spiTransaction[spiTransactionIndex].txBuf  = transmitBufLen > receiveBufLen ?
                        pTransmitBuf + receiveBufLen : (uint8_t *)EMPTY_ARRAY;
                handle->spiTransaction[spiTransactionIndex].rxBuf  = transmitBufLen > receiveBufLen ?
                        NULL : pReceiveBuf + transmitBufLen;
                handle->spiTransaction[spiTransactionIndex].arg = (void *)handle;
                handle->pLastSPITransaction = &(handle->spiTransaction[spiTransactionIndex]);

                // Initiate transfer of second transaction
                spiTransferResult = SPI_transfer(handle->spiHandle, (SPI_Transaction *)&handle->spiTransaction[spiTransactionIndex]);

                if(false == spiTransferResult)
                {
                    status = Plat_SPI_Failed;
                }
            }
        }
        else if(0 < receiveBufLen)
        {
            // Receive only operation
            handle->spiTransaction[spiTransactionIndex].count  = receiveBufLen;
            handle->spiTransaction[spiTransactionIndex].txBuf  = (uint8_t *)EMPTY_ARRAY;
            handle->spiTransaction[spiTransactionIndex].rxBuf  = pReceiveBuf;
            handle->spiTransaction[spiTransactionIndex].arg = (void *)handle;
            handle->pLastSPITransaction = &(handle->spiTransaction[spiTransactionIndex]);

            // Initiate transfer of transaction
            spiTransferResult = SPI_transfer(handle->spiHandle, (SPI_Transaction *)&handle->spiTransaction[spiTransactionIndex]);

            if(false == spiTransferResult)
            {
                status = Plat_SPI_Failed;
            }
            else
            {
                PLAT_SPI_SET_ACTIVE(handle);
            }
        }
        else if(0 < transmitBufLen)
        {
            // Transmit only operation
            handle->spiTransaction[0].count  = transmitBufLen;
            handle->spiTransaction[0].txBuf  = pTransmitBuf;
            handle->spiTransaction[0].rxBuf  = NULL;
            handle->spiTransaction[spiTransactionIndex].arg = (void *)handle;
            handle->pLastSPITransaction = &(handle->spiTransaction[spiTransactionIndex]);

            // Initiate transfer of transaction
            spiTransferResult = SPI_transfer(handle->spiHandle, (SPI_Transaction *)&handle->spiTransaction[spiTransactionIndex]);

            if(false == spiTransferResult)
            {
                status = Plat_SPI_Failed;
            }
            else
            {
                PLAT_SPI_SET_ACTIVE(handle);
            }
        }
        else
        {
            /* No transmit/receive requested, why bother? */
            status = Plat_SPI_Failed;
        }

    }

    return(status);
}
