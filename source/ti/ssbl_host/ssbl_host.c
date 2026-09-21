/*
 * Copyright (c) 2022-2026, Texas Instruments Incorporated
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

#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/Timer.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rom.h)
#include DeviceFamily_constructPath(driverlib/flash.h)
#include <ti/ssbl_host/ssbl_host.h>
#include <ti/ssbl/bldrcmds.h>

// Allows for simpler integration with the TRX Host RF Driver
#ifndef CONFIG_TIMER_0
    #define CONFIG_TIMER_0 CONFIG_TIMER_SSBL
#endif

// There are multiple functions which are unused in this file due to it being
// a resource for use outside of this simple starter example
#if __ICCARM__
    #pragma diag_suppress = Pe177
#elif __GNUC__
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include <ti/display/Display.h>
extern Display_Handle display;

//*****************************************************************************
//
// The byte sequence that is sent to acknowledge a received packet.
//
//*****************************************************************************
static const uint8_t g_ui8ACK[2] = {0, BLDR_CMD_ACK};

/**
 * @brief Structure containing security version info
 *
 */
typedef struct
{
    uint8_t version; ///< Version of the current running TRX software
    uint32_t status; ///< Status of the current running TRX software
    uint8_t slot; ///< Version slot of the current running TRX software. Indicating how many more updates are possible
} ssblSecVersionInfo_t;

/**
 * @brief Simple typedef representing a part ID
 *
 */
typedef uint32_t ssblPartId_t;

/**
 * @brief Structure representing a single image block hash
 *
 */
typedef struct
{
    uint8_t hash[32]; ///< Hash over a single image block
} ssblBlockHash_t;

/**
 * @brief All download info which could be required/helpful throughout the update sequence
 *
 * Some of these fields are parsed directly from the TI provided download cmd payload.
 * Others are calculated or inferred based on all information provided
 *
 */
typedef struct
{
    uint16_t imageId;                             ///< Image ID
    uint8_t secVersion;                           ///< Security version
    uint32_t size;                                ///< Image size
    uint32_t remainingSize;                       ///< Remaining size
    uint32_t startAddress;                        ///< Image start address (Where to be programmed on the TRX)
    uint32_t endAddress;                          ///< Image end address (calculated based on start and size)
    uint8_t numBlocks;                            ///< Number of blocks in the image
    uint8_t currBlock;                            ///< Current block of the update
    ssblBlockHash_t blockHashes[TRX_NUM_SECTORS]; ///< Worst case number of block Hashes based on the number of sectors
                                                  ///< on the TRX device
    ssblEccSignature_t tiSign;                    ///< TISignature
    ssblEccSignature_t custSign;                  ///< Customer Signature
} ssblDownloadInfo_t;

/* Callback used to indicate response timeout has occurred */
void timeoutCallback(Timer_Handle myHandle, int_fast16_t status);

static BLDR_STATUS_T sendPingCommand(const SPI_Handle spiHandle);
static BLDR_STATUS_T sendGetStatusCommand(const SPI_Handle spiHandle);
static BLDR_STATUS_T sendGetSecVerCommand(const SPI_Handle spiHandle, ssblSecVersionInfo_t *secVersionInfo);
static BLDR_STATUS_T sendBreakOutCommand(const SPI_Handle spiHandle, bool checkStatus);
static BLDR_STATUS_T sendResetCommand(const SPI_Handle spiHandle);
static BLDR_STATUS_T sendGetPartIdCommand(const SPI_Handle spiHandle, ssblPartId_t *partId);
static BLDR_STATUS_T sendGetKeyIdCommand(const SPI_Handle spiHandle, ssblKeyId_t *keyId);
static BLDR_STATUS_T sendGetFwHashCommand(const SPI_Handle spiHandle, ssblFwHash_t *fwHash);
static BLDR_STATUS_T sendSetCustKeyCommand(const SPI_Handle spiHandle, const ssblCustKey_t *custKey);
static BLDR_STATUS_T sendDownloadCommand(const SPI_Handle spiHandle, uint8_t *downloadCmdBuffer, uint16_t buffLength);
static BLDR_STATUS_T sendBankEraseCommand(const SPI_Handle spiHandle);
static BLDR_STATUS_T sendSendDataCommand(const SPI_Handle spiHandle,
                                         uint8_t *blockBuff,
                                         uint32_t blockSize,
                                         uint32_t blockNum);
static uint32_t swapWord(const uint8_t *pui8Data);
static uint16_t swapShort(const uint8_t *pui8Data);
static bool isMemRangeAllOnes(uint32_t *pMemRangeBase, uint32_t byteCount);

static BLDR_STATUS_T sendPacket(const SPI_Handle spiHandle,
                                const BLDR_CMD_ID_T cmdId,
                                const uint8_t *payload,
                                const uint16_t payloadSize);
static BLDR_STATUS_T receivePacket(const SPI_Handle spiHandle, uint8_t *payload, uint16_t *payloadSize);
static BLDR_STATUS_T sendBytes(const SPI_Handle spiHandle, const uint8_t *payload, const uint16_t payloadSize);

//*****************************************************************************
//
// This is the data buffer assigned for transfers to the SSBL.
// The payload buffer being used starts at byte 3 of the first 32-bit
// word of the assigned buffer by later on allocating a pointer to this
// position.
// The max size is decided by the max size of a download command.
// sizeof(max_num_hashes) + [ BLDR_CMD_OVERHEAD ] + [ BLDR_DOWNLOAD_CMD_OVERHEAD ]
//
//*****************************************************************************
#ifndef TRX_SSBL_DYNAMIC_ALLOCATION
static uint8_t
    g_pui8DataBuffer[(TRX_NUM_SECTORS * sizeof(ssblBlockHash_t)) + BLDR_CMD_OVERHEAD + BLDR_DOWNLOAD_CMD_OVERHEAD];
static uint8_t *g_pui8PayloadBuffer = &g_pui8DataBuffer[2];
#else
static uint8_t *g_pui8DataBuffer;
uint8_t *g_pui8PayloadBuffer;
#endif

/**
 * Timeout related global variables
 */
Timer_Handle timeoutTimer;
static bool hasTimeoutOccurred = false;

/**
 * @brief Quickly confirm the CRC32 at the end of the trx_fw_update_final.bin in
 * flash matches the stored value
 *
 * @param trxFwUpdateBuffer - address where the trx_fw_update_final.bin has been
 * stored in flash
 * @return BLDR_STATUS_T
 */
BLDR_STATUS_T crcCheckTrxFwUpdateBuffer(const uint8_t *trxFwUpdateBuffer)
{
    uint32_t imageSize, downloadPayloadSize, totalSize;
    uint8_t numBlocks;
    uint32_t calcCrc, storedCrc;

    imageSize           = swapWord(&trxFwUpdateBuffer[3]);
    numBlocks           = (imageSize + (TRX_MAIN_FLASH_SECTOR_SIZE - 1)) / TRX_MAIN_FLASH_SECTOR_SIZE;
    downloadPayloadSize = BLDR_DOWNLOAD_CMD_OVERHEAD + (sizeof(ssblBlockHash_t) * numBlocks);
    totalSize           = imageSize + downloadPayloadSize;

    // Sanity check the values before we attempt to use them for CRC calculation
    if ((FlashSizeGet() <= ((uint32_t)trxFwUpdateBuffer + totalSize)) ||
        (imageSize >= (TRX_NUM_SECTORS * TRX_MAIN_FLASH_SECTOR_SIZE)))
    {
        return BLDR_CMD_RET_FAILURE;
    }

    calcCrc   = HapiCrc32((uint8_t *)trxFwUpdateBuffer, totalSize, 0);
    storedCrc = swapWord(&trxFwUpdateBuffer[totalSize]);
    if (calcCrc != storedCrc)
    {
        return BLDR_CMD_RET_FAILURE;
    }

    return BLDR_CMD_RET_SUCCESS;
}

/**
 * @brief Performs all steps required to complete a software update
 *
 * @param downloadStruct
 * @return BLDR_STATUS_T
 */
BLDR_STATUS_T performSsblDownloadSequence(const ssblDownload_t *downloadStruct)
{
    SPI_Params spiParams;
    SPI_Handle ssblHostSpi;
    Timer_Params timerParams;
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;
    ssblDownloadInfo_t downloadInfo;
    uint32_t downloadPayloadSize;

#ifdef TRX_SSBL_DYNAMIC_ALLOCATION
    g_pui8DataBuffer    = (uint8_t *)malloc((TRX_NUM_SECTORS * sizeof(ssblBlockHash_t)) + BLDR_CMD_OVERHEAD +
                                         BLDR_DOWNLOAD_CMD_OVERHEAD);
    g_pui8PayloadBuffer = &g_pui8DataBuffer[2];
#endif

    SPI_init();
    Timer_init();

    /*
     * Verify Input parameters make sense. If programKey is true then custKey
     * must be provided.
     */
    if ((downloadStruct->programKey && !downloadStruct->custKey) || (!downloadStruct->trxFwUpdateBuffer))
    {
        /*
         * All other error codes are relative to the specific SSBL CMD. This is
         * not a SSBL CMD and thus there is not a BLDR_STATUS_T for it.
         */
        return (BLDR_STATUS_T)BLDR_CMD_RET_NO_CUST_KEY;
    }

    /*
     * Confirm if there is a potentially valid image in flash. If not then return right away
     * and indicate as such. Buffer and bytecount must be a multiple of 4. To make this quick
     * we can check just the first 8 bytes of the buffer. This would not be a valid image and
     * therefore the only reason it should be all ones is if it is erased
     */
    if (isMemRangeAllOnes((uint32_t *)downloadStruct->trxFwUpdateBuffer, 8))
    {
        /*
         * All other error codes are relative to the specific SSBL CMD. This mem check
         * however is not a SSBL CMD and thus there is not a BLDR_STATUS_T for it.
         */
        return (BLDR_STATUS_T)BLDR_CMD_RET_NO_IMAGE_FOUND;
    }

    /*
     * Do a quick CRC check over the TRX FW image to determine if the contents
     * of the trxFwUpdateBuffer are valid
     */
    if (BLDR_CMD_RET_FAILURE == crcCheckTrxFwUpdateBuffer((uint8_t *)downloadStruct->trxFwUpdateBuffer))
    {
        /*
         * All other error codes are relative to the specific SSBL CMD. This CRC check
         * however is not a SSBL CMD and thus there is not a BLDR_STATUS_T for it.
         */
        return (BLDR_STATUS_T)BLDR_CMD_RET_CRC_VERIFY_FAILED;
    }

    // Hold TRX in reset
    GPIO_write(CONFIG_GPIO_SSBL_RESET, GPIO_CFG_OUT_LOW);

    // Allow rest pin to settle
    usleep(50);

    // Trigger the SSBL on the TRX as configured
    GPIO_write(CONFIG_GPIO_SSBL_TRIGGER, !(downloadStruct->triggerSsbl));

    // Allow trigger pin to settle
    usleep(50);

    // Allow the TRX to come out of reset
    GPIO_write(CONFIG_GPIO_SSBL_RESET, GPIO_CFG_OUT_HIGH);

    // Allow the SSBL time to detect trigger
    usleep(400); // Value found experimentally + buffer time

    // Avoid additional SPI glitches by forcing CSN HIGH
    GPIO_write(CONFIG_GPIO_SSBL_TRIGGER, GPIO_CFG_OUT_HIGH);

    // Open SPI as controller (default)
    SPI_Params_init(&spiParams);
    spiParams.frameFormat = SPI_POL0_PHA0;
    spiParams.bitRate     = 4000000;
    ssblHostSpi           = SPI_open(CONFIG_SPI_SSBL_HOST, &spiParams);

    /*
     * Setting up the timer in one shot callback mode that calls the callback
     * function if the timeout occurs.
     */
    Timer_Params_init(&timerParams);
    if (downloadStruct->custKey)
    {
        timerParams.period = 4000000; /* 4 seconds to allow for both customer key and TI key verification */
    }
    else
    {
        timerParams.period = 2500000; /* 2.5 seconds to allow for just the TI key verification */
    }
    timerParams.periodUnits   = Timer_PERIOD_US;
    timerParams.timerMode     = Timer_ONESHOT_CALLBACK;
    timerParams.timerCallback = timeoutCallback;
    timeoutTimer              = Timer_open(CONFIG_TIMER_0, &timerParams);
    if (NULL == timeoutTimer)
    {
        HANDLE_FAIL();
    }

    // Parse downloadCmd to get helpful information out of it
    downloadInfo.imageId      = swapShort(&downloadStruct->trxFwUpdateBuffer[0]);
    downloadInfo.secVersion   = downloadStruct->trxFwUpdateBuffer[2];
    downloadInfo.size         = swapWord(&downloadStruct->trxFwUpdateBuffer[3]);
    downloadInfo.startAddress = swapWord(&downloadStruct->trxFwUpdateBuffer[7]);

    // Calculate some helpful values to save for later use (round up the numBlocks)
    downloadInfo.endAddress = downloadInfo.startAddress + downloadInfo.size;
    downloadInfo.numBlocks  = (downloadInfo.size + (TRX_MAIN_FLASH_SECTOR_SIZE - 1)) / TRX_MAIN_FLASH_SECTOR_SIZE;
    downloadPayloadSize     = BLDR_DOWNLOAD_CMD_OVERHEAD + (sizeof(ssblBlockHash_t) * downloadInfo.numBlocks);

    // Verify the host has a line of communication with the SSBL
    status = sendPingCommand(ssblHostSpi);
    if (BLDR_CMD_RET_SUCCESS != status)
    {
        HANDLE_FAIL();
    }

    // Verify Security Version is as expected
    ssblSecVersionInfo_t initialSecVersionInfo;
    status = sendGetSecVerCommand(ssblHostSpi, &initialSecVersionInfo);
    if ((BLDR_CMD_RET_SUCCESS != status) || (initialSecVersionInfo.version > downloadInfo.secVersion))
    { // Device version is > new version number (downgrade protection)
        HANDLE_FAIL();
    }

    // Program key as specified
    if (downloadStruct->programKey)
    {
        // Set the customer key (one time only operation)
        status = sendSetCustKeyCommand(ssblHostSpi, downloadStruct->custKey);
        if (BLDR_CMD_RET_SUCCESS != status)
        {
            HANDLE_FAIL();
        }
    }

    // If key is provided, verify the current keyId matches what was given
    if (downloadStruct->custKey)
    {
        // Verify the provided key was written successfully
        ssblKeyId_t newKeyId;
        status = sendGetKeyIdCommand(ssblHostSpi, &newKeyId);
        if ((BLDR_CMD_RET_SUCCESS != status) || (memcmp(downloadStruct->custKey->keyId, newKeyId, sizeof(newKeyId))))
        {
            HANDLE_FAIL();
        }
    }

    // The order of these three commands are enforced.
    // DOWNLOAD -> BANK_ERASE -> SEND_DATA[N]
    // If you send these three out of order you will get a failure response

    // Start the download
    status = sendDownloadCommand(ssblHostSpi, downloadStruct->trxFwUpdateBuffer, downloadPayloadSize);
    if (BLDR_CMD_RET_SUCCESS != status)
    {
        HANDLE_FAIL();
    }

    // Erase the existing application
    status = sendBankEraseCommand(ssblHostSpi);
    if (BLDR_CMD_RET_SUCCESS != status)
    {
        HANDLE_FAIL();
    }

    // Send each new image block one by one
    downloadInfo.remainingSize = downloadInfo.size;
    for (downloadInfo.currBlock = 0; downloadInfo.currBlock < downloadInfo.numBlocks; downloadInfo.currBlock++)
    {
        uint32_t blockSize   = TRX_MAIN_FLASH_SECTOR_SIZE;
        uint32_t blockOffset = (blockSize * downloadInfo.currBlock) + downloadPayloadSize;
        if (downloadInfo.currBlock == (downloadInfo.numBlocks - 1))
        {
            blockSize = downloadInfo.remainingSize;
        }

        // Send individual block
        status = sendSendDataCommand(ssblHostSpi,
                                     &downloadStruct->trxFwUpdateBuffer[blockOffset],
                                     blockSize,
                                     downloadInfo.currBlock);
        if (BLDR_CMD_RET_SUCCESS != status)
        {
            HANDLE_FAIL();
        }

        // Update the remaining size based on the size of the previous block
        downloadInfo.remainingSize -= blockSize;
    }

    // Verify the download was successful by checking the new security version info
    ssblSecVersionInfo_t newSecVersionInfo;
    status = sendGetSecVerCommand(ssblHostSpi, &newSecVersionInfo);
    if ((BLDR_CMD_RET_SUCCESS != status) || (newSecVersionInfo.status != 0xAAAAAAAA) ||
        (newSecVersionInfo.version != downloadInfo.secVersion))
    {
        HANDLE_FAIL();
    }

    // Get and display the new FW Hash
    ssblFwHash_t currFwHash;
    status = sendGetFwHashCommand(ssblHostSpi, &currFwHash);
    if (BLDR_CMD_RET_SUCCESS != status)
    {
        HANDLE_FAIL();
    }
    // clang-format off
    Display_printf(display, 0, 0, "Current FW Hash = 0x%x%x%x%x%x%x%x%x\n",
                   (uint32_t *)currFwHash[0],
                   (uint32_t *)currFwHash[1],
                   (uint32_t *)currFwHash[2],
                   (uint32_t *)currFwHash[3],
                   (uint32_t *)currFwHash[4],
                   (uint32_t *)currFwHash[5],
                   (uint32_t *)currFwHash[6],
                   (uint32_t *)currFwHash[7]);
    // clang-format on

    // Don't attempt to send a getStatus cmd. it's expected for the device not to respond
    // status in this case only refers to if the cmd was ACK'ed.
    status = sendBreakOutCommand(ssblHostSpi, false);
    if (BLDR_CMD_RET_SUCCESS != status)
    {
        HANDLE_FAIL();
    }

    Timer_close(timeoutTimer);
    SPI_close(ssblHostSpi);

#ifdef TRX_SSBL_DYNAMIC_ALLOCATION
    free(g_pui8DataBuffer);
#endif
    return status;
}

/*
 * This callback will be called whenever the TRX has not responded in the expected
 * timeout period.
 */
void timeoutCallback(Timer_Handle myHandle, int_fast16_t status)
{
    hasTimeoutOccurred = true;
}

/**
 * @brief Send a Ping Command
 *
 * Status, in this case is only if an ACK was received.
 *
 * @param spiHandle
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendPingCommand(const SPI_Handle spiHandle)
{
    return sendPacket(spiHandle, BLDR_CMD_ID_PING, NULL, 0);
}

/**
 * @brief Send a Get Status Command and return the result
 *
 * @param spiHandle
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendGetStatusCommand(const SPI_Handle spiHandle)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS == sendPacket(spiHandle, BLDR_CMD_ID_GET_STATUS, NULL, 0))
    {
        uint16_t rxLength = 1;
        // receive response
        if (BLDR_CMD_RET_SUCCESS == (status = receivePacket(spiHandle, g_pui8DataBuffer, &rxLength)))
        {
            sendBytes(spiHandle, (const uint8_t *)g_ui8ACK, 2);

            // Return status that was received
            status = (BLDR_STATUS_T)g_pui8PayloadBuffer[0];
        }
    }

    return status;
}

/**
 * @brief Send a Get Security Version Command and return the status
 *
 * If successful, the retrieved security version information will reside in the
 * secVersionInfo parameter.
 *
 * @param spiHandle
 * @param secVersionInfo
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendGetSecVerCommand(const SPI_Handle spiHandle, ssblSecVersionInfo_t *secVersionInfo)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS == (status = sendPacket(spiHandle, BLDR_CMD_ID_GET_SEC_VER, NULL, 0)))
    {
        // receive response
        uint16_t rxLength = 5;
        if (BLDR_CMD_RET_SUCCESS == (status = receivePacket(spiHandle, g_pui8DataBuffer, &rxLength)))
        {
            sendBytes(spiHandle, (uint8_t *)g_ui8ACK, 2);

            secVersionInfo->version = g_pui8PayloadBuffer[0];
            uint8_t *statusBuff     = (uint8_t *)&secVersionInfo->status;
            statusBuff[0]           = g_pui8PayloadBuffer[1];
            statusBuff[1]           = g_pui8PayloadBuffer[2];
            statusBuff[2]           = g_pui8PayloadBuffer[3];
            statusBuff[3]           = g_pui8PayloadBuffer[4];
            secVersionInfo->slot    = g_pui8PayloadBuffer[5];

            // Return status that was received
            status = sendGetStatusCommand(spiHandle);
        }
    }

    return status;
}

/**
 * @brief Send a Break Out Command
 *
 * This function will only check the status of the break out command if requested
 * via the checkStatus parameter.
 *
 * This is useful because if the break out command was to succeed, the TRX device
 * will no longer be running the SSBL software. Which means any attempt to get
 * the status would not be understood.
 *
 * @param spiHandle
 * @param checkStatus
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendBreakOutCommand(const SPI_Handle spiHandle, bool checkStatus)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS == (status = sendPacket(spiHandle, BLDR_CMD_ID_BREAK_OUT, NULL, 0)))
    {
        if (checkStatus)
        {
            // Return status that was received
            status = sendGetStatusCommand(spiHandle);
        }
    }

    return status;
}

/**
 * @brief Send a Reset Command
 *
 * @param spiHandle
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendResetCommand(const SPI_Handle spiHandle)
{
    return sendPacket(spiHandle, BLDR_CMD_ID_RESET, NULL, 0);
}

/**
 * @brief Send a Get Part ID Command and return the status
 *
 * If successful, the retrieved part id will reside in the partId parameter.
 *
 * @param spiHandle
 * @param partId
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendGetPartIdCommand(const SPI_Handle spiHandle, ssblPartId_t *partId)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS == (status = sendPacket(spiHandle, BLDR_CMD_ID_GET_PART_ID, NULL, 0)))
    {
        // receive response
        uint16_t rxLength = sizeof(ssblPartId_t);
        if (BLDR_CMD_RET_SUCCESS == (status = receivePacket(spiHandle, g_pui8DataBuffer, &rxLength)))
        {
            sendBytes(spiHandle, (uint8_t *)g_ui8ACK, 2);

            ((uint8_t *)partId)[0] = g_pui8PayloadBuffer[3];
            ((uint8_t *)partId)[1] = g_pui8PayloadBuffer[2];
            ((uint8_t *)partId)[2] = g_pui8PayloadBuffer[1];
            ((uint8_t *)partId)[3] = g_pui8PayloadBuffer[0];

            // Return status that was received
            status = sendGetStatusCommand(spiHandle);
        }
    }

    return status;
}

/**
 * @brief Send a Get Key ID Command and return the status
 *
 * If successful, the retrieved key ID will reside in the keyId parameter
 *
 * @param spiHandle
 * @param keyId
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendGetKeyIdCommand(const SPI_Handle spiHandle, ssblKeyId_t *keyId)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS == (status = sendPacket(spiHandle, BLDR_CMD_ID_GET_KEY_ID, NULL, 0)))
    {
        // receive response
        uint16_t rxLength = sizeof(ssblKeyId_t);
        if (BLDR_CMD_RET_SUCCESS == (status = receivePacket(spiHandle, g_pui8DataBuffer, &rxLength)))
        {
            sendBytes(spiHandle, (uint8_t *)g_ui8ACK, 2);

            memcpy((uint8_t *)keyId, g_pui8PayloadBuffer, sizeof(ssblKeyId_t));

            // Return status that was received
            status = sendGetStatusCommand(spiHandle);
        }
    }

    return status;
}

/**
 * @brief Send a Get FW Hash ID Command and return the status
 *
 * If successful, the retrieved FW Hash will reside in the fwHash parameter
 *
 * @param spiHandle
 * @param fwHash
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendGetFwHashCommand(const SPI_Handle spiHandle, ssblFwHash_t *fwHash)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS == (status = sendPacket(spiHandle, BLDR_CMD_ID_GET_FW_HASH, NULL, 0)))
    {
        // receive response
        uint16_t rxLength = sizeof(ssblFwHash_t);
        if (BLDR_CMD_RET_SUCCESS == (status = receivePacket(spiHandle, g_pui8DataBuffer, &rxLength)))
        {
            sendBytes(spiHandle, (uint8_t *)g_ui8ACK, 2);

            memcpy((uint8_t *)fwHash, g_pui8PayloadBuffer, sizeof(ssblFwHash_t));

            // Return status that was received
            status = sendGetStatusCommand(spiHandle);
        }
    }

    return status;
}

/**
 * @brief Send a Set Customer Key Command and return the status
 *
 * @param spiHandle
 * @param custKey
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendSetCustKeyCommand(const SPI_Handle spiHandle, const ssblCustKey_t *custKey)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS ==
        (status = sendPacket(spiHandle, BLDR_CMD_ID_SET_CUST_KEY, (uint8_t *)custKey, sizeof(ssblCustKey_t))))
    {
        // Return status that was received
        status = sendGetStatusCommand(spiHandle);
    }

    return status;
}

/**
 * @brief Send a Download Command and return the status
 *
 * @param spiHandle
 * @param downloadCmdBuffer
 * @param buffLength
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendDownloadCommand(const SPI_Handle spiHandle, uint8_t *downloadCmdBuffer, uint16_t buffLength)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS ==
        (status = sendPacket(spiHandle, BLDR_CMD_ID_DOWNLOAD, (uint8_t *)downloadCmdBuffer, buffLength)))
    {
        // Return status that was received
        status = sendGetStatusCommand(spiHandle);
    }

    return status;
}

/**
 * @brief Send a Bank Erase Command and return the status
 *
 * @param spiHandle
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendBankEraseCommand(const SPI_Handle spiHandle)
{
    BLDR_STATUS_T status = BLDR_CMD_RET_FAILURE;

    if (BLDR_CMD_RET_SUCCESS == (status = sendPacket(spiHandle, BLDR_CMD_ID_BANK_ERASE, NULL, 0)))
    {
        // Return status that was received
        status = sendGetStatusCommand(spiHandle);
    }
    return status;
}

/**
 * @brief Send a Send Data Command and return the status
 *
 * @param spiHandle
 * @param blockBuff
 * @param blockSize
 * @param blockNum
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendSendDataCommand(const SPI_Handle spiHandle,
                                         uint8_t *blockBuff,
                                         uint32_t blockSize,
                                         uint32_t blockNum)
{
    BLDR_STATUS_T status  = BLDR_CMD_RET_FAILURE;
    uint8_t *blockSizePtr = (uint8_t *)&blockSize;
    uint8_t *blockNumPtr  = (uint8_t *)&blockNum;

    g_pui8PayloadBuffer[1] = blockSizePtr[3];
    g_pui8PayloadBuffer[2] = blockSizePtr[2];
    g_pui8PayloadBuffer[3] = blockSizePtr[1];
    g_pui8PayloadBuffer[4] = blockSizePtr[0];
    g_pui8PayloadBuffer[5] = blockNumPtr[3];
    g_pui8PayloadBuffer[6] = blockNumPtr[2];
    g_pui8PayloadBuffer[7] = blockNumPtr[1];
    g_pui8PayloadBuffer[8] = blockNumPtr[0];
    memcpy(&g_pui8PayloadBuffer[9], blockBuff, blockSize);

    // The NULL payload and a non NULL size indicates the payload has been pre-prepared
    if (BLDR_CMD_RET_SUCCESS == (status = sendPacket(spiHandle, BLDR_CMD_ID_SEND_DATA, NULL, blockSize + 8)))
    {
        // Return status that was received
        status = sendGetStatusCommand(spiHandle);
    }

    return status;
}

/**
 * @brief Send a payload in packet form
 *
 * spiHandle and cmdID are the only required params. If either payload or
 * payloadSize are zero then only the cmdID will be passed to the lower level
 * packet handler.
 *
 * In consideration of the SEND_DATA command, if a payloadSize is provided but
 * not a payload. It is assumed that the payload has been copied into the
 * g_pui8PayloadBuffer already and the memcpy operation is skipped.
 *
 * @param spiHandle
 * @param cmdId
 * @param payload
 * @param payloadSize
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendPacket(const SPI_Handle spiHandle,
                                const BLDR_CMD_ID_T cmdId,
                                const uint8_t *payload,
                                const uint16_t payloadSize)
{
    SPI_Transaction transaction;
    uint16_t finalSize = payloadSize;
    uint32_t ui32Crc, swappedCrc;

    g_pui8PayloadBuffer[0] = cmdId;
    // Special use case for sendData cmd. payloadbuffer has already been filled
    if (payload && payloadSize)
    {
        memcpy(&g_pui8PayloadBuffer[1], payload, payloadSize);
    }

    // Include the size, checksum and cmdID bytes in the total size
    finalSize += (sizeof(finalSize) + sizeof(ui32Crc) + 1);
    memcpy((void *)g_pui8DataBuffer, (void *)&finalSize, sizeof(finalSize));

    // Calculate the checksum to be sent out with the data and add it to the buffer
    ui32Crc    = HapiCrc32((uint8_t *)g_pui8DataBuffer, finalSize - sizeof(ui32Crc), 0);
    swappedCrc = swapWord((uint8_t *)&ui32Crc);
    memcpy((void *)&g_pui8DataBuffer[finalSize - sizeof(ui32Crc)], (void *)&swappedCrc, sizeof(swappedCrc));

    // Perform SPI transfer
    transaction.count = finalSize;
    transaction.txBuf = (void *)g_pui8DataBuffer;
    transaction.rxBuf = (void *)NULL;
    SPI_transfer(spiHandle, &transaction);

    // Start Timeout Timer
    hasTimeoutOccurred = false;
    if (Timer_start(timeoutTimer) == Timer_STATUS_ERROR)
    {
        // Failed to start timer
        return BLDR_CMD_RET_FAILURE;
    }

    // Wait for non zero byte response
    uint8_t ui8Rsp    = 0;
    transaction.count = 1;
    transaction.txBuf = NULL;
    transaction.rxBuf = &ui8Rsp;
    while ((ui8Rsp == 0) && !hasTimeoutOccurred)
    {
        SPI_transfer(spiHandle, &transaction);
    }

    // Stop Timeout Timer
    Timer_stop(timeoutTimer);

    if (hasTimeoutOccurred)
    {
        /*
         * All other error codes are relative to the specific SSBL CMD. This timeout
         * check however is not a SSBL CMD and thus there is not a BLDR_STATUS_T for it.
         */
        return (BLDR_STATUS_T)BLDR_CMD_RET_RESP_TIMEOUT;
    }

    if (ui8Rsp != BLDR_CMD_ACK)
    {
        return BLDR_CMD_RET_FAILURE;
    }

    return BLDR_CMD_RET_SUCCESS;
}

/**
 * @brief Send a payload not in packet form
 *
 * Sends the payloadSize bytes of the provided payload buffer without first
 * formatting it into a packet.
 *
 * @param spiHandle
 * @param payload
 * @param payloadSize
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T sendBytes(const SPI_Handle spiHandle, const uint8_t *payload, uint16_t payloadSize)
{
    SPI_Transaction transaction;

    // Perform SPI transfer
    transaction.count = payloadSize;
    transaction.txBuf = (void *)payload;
    transaction.rxBuf = (void *)NULL;
    SPI_transfer(spiHandle, &transaction);

    return BLDR_CMD_RET_SUCCESS;
}

/**
 * @brief Receive a packet
 *
 * Receives a packet from the SSBL on the CC140X. The will populate the payload
 * and payloadSize pointers based on the content received.
 *
 * ** Warning ** this function does not time out. It will keep attempting to
 * receive a packet until a full packet is received.
 *
 * @param spiHandle
 * @param payload
 * @param payloadSize
 * @return BLDR_STATUS_T
 */
static BLDR_STATUS_T receivePacket(const SPI_Handle spiHandle, uint8_t *payload, uint16_t *payloadSize)
{
    SPI_Transaction transaction;
    uint32_t ui32Crc;
    uint16_t ui16Size;
    uint16_t ui16PayloadSize;
    uint8_t tempBytes[4];

    // Start Timeout Timer
    hasTimeoutOccurred = false;
    if (Timer_start(timeoutTimer) == Timer_STATUS_ERROR)
    {
        // Failed to start timer
        return BLDR_CMD_RET_FAILURE;
    }

    // Wait for a non-zero value. Because it is guaranteed to be non-zero,
    // the first value received is the first byte of the 'packet size' field.
    // **in little endian**.
    transaction.count = 1;
    transaction.txBuf = NULL;
    transaction.rxBuf = payload;
    do
    {
        SPI_transfer(spiHandle, &transaction);
    } while ((payload[0] == 0) && !hasTimeoutOccurred);

    // Stop Timeout Timer
    Timer_stop(timeoutTimer);

    if (hasTimeoutOccurred)
    {
        /*
         * All other error codes are relative to the specific SSBL CMD. This timeout
         * check however is not a SSBL CMD and thus there is not a BLDR_STATUS_T for it.
         */
        return (BLDR_STATUS_T)BLDR_CMD_RET_RESP_TIMEOUT;
    }

    // If the first non-zero byte has been received, we can read the second
    // byte of the 'packet size' field and begin receiving the remainder of the
    // packet
    transaction.rxBuf = &payload[1];
    SPI_transfer(spiHandle, &transaction);
    ui16Size = (*(uint16_t *)payload);

    // Receive the payload - This will be used for cmd processing
    ui16PayloadSize   = ui16Size - BLDR_CMD_OVERHEAD;
    transaction.count = ui16PayloadSize;
    transaction.rxBuf = &payload[sizeof(ui16Size)];
    SPI_transfer(spiHandle, &transaction);

    // Receive the CRC of the packet (remaining sizeof(ui32Crc) bytes)
    transaction.count = sizeof(ui32Crc);
    transaction.rxBuf = tempBytes;
    SPI_transfer(spiHandle, &transaction);
    ui32Crc = swapWord(tempBytes);

    // Calculate the CRC of the packet and compare against the received CRC
    if (HapiCrc32(payload, ui16Size - sizeof(ui32Crc), 0) != ui32Crc)
    {
        return BLDR_CMD_RET_FAILURE;
    }

    // Make sure to return the number of bytes received.
    *payloadSize = ui16PayloadSize;

    return BLDR_CMD_RET_SUCCESS;
}

/**
 * @brief Swap a series of 4 bytes from a buffer and return a single 32-bit word
 *
 * @param pui8Data
 * @return swapped_word
 */
static uint32_t swapWord(const uint8_t *pui8Data)
{
    // Align the bytes in the buffer into a 32-bit value.
    return ((pui8Data[0] << 24) | (pui8Data[1] << 16) | (pui8Data[2] << 8) | (pui8Data[3]));
}

/**
 * @brief Swap 2 bytes from a buffer and return a single 16-bit short
 *
 * @param pui8Data
 * @return swapped_short
 */
static uint16_t swapShort(const uint8_t *pui8Data)
{
    // Align the bytes in the buffer into a 16-bit value.
    return ((pui8Data[0] << 8) | pui8Data[1]);
}

/**
 * @brief Check if a memory range is all ones (erased)
 *
 * @param pMemRangeBase    Pointer to start of memory range (will return false if not a multiple of 4)
 * @param byteCount        Number of bytes in the range (will return false if not a multiple of 4)
 * @return true if memory is all ones, else false
 */
static bool isMemRangeAllOnes(uint32_t *pMemRangeBase, uint32_t byteCount)
{
    // Return false if pMemRangeBase is not a multiple of 4 (word aligned)
    if ((uint32_t)pMemRangeBase & (uint32_t)0x3 || (uint32_t)byteCount & (uint32_t)0x3)
    {
        return false;
    }

    // Check whether the entire address range is 0xFFFFFFFF
    uint32_t wordComb = 0xFFFFFFFF;
    uint32_t *pWord   = pMemRangeBase;
    uint32_t *pEnd    = (uint32_t *)((uint32_t)pMemRangeBase + byteCount);
    while (pWord < pEnd)
    {
        wordComb &= *(pWord++);
    }
    return (wordComb == 0xFFFFFFFF);
}
