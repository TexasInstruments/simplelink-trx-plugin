/******************************************************************************
 Group: CMCU LPRF
 Target Device: cc13xx_cc26xx

 ******************************************************************************
 
 Copyright (c) 2023-2026, Texas Instruments Incorporated
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

@file trx_host_private.c

@brief Implements internal helper functions of the TRX Host driver.

 */

/********************************** Includes **********************************/

/* Standard Header Files */
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Module Header Files */
#include <ti/trx/TRX.h>
#include <ti/trx/host/trx_host.h>
#include <ti/trx/host/private/trx_host_private.h>
#include <ti/trx/host/utility/list/list.h>
#include <ti/trx/host/hal/platform_util.h>
#include <ti/trx/host/utility/utils.h>
#include <ti/trx/host/transport/spi_transport.h>
#include <ti/trx/host/utility/buffer_manager/buffer_manager.h>

/* TI Logging Files */
#include <ti/log/Log.h>


/********************************** Defines ***********************************/

/*********************************** Macros ***********************************/

/*********************************** Enums ************************************/

/********************************** Structs ***********************************/

/********************************** Typedefs **********************************/

/********************************* Prototypes *********************************/

/*************************** Variable Declarations ****************************/

/************************* Extern Function Definitions ************************/

TRX_Host_Stream *resolveStream(TRX_Host_Handle handle, uint8_t id)
{
    TRX_Host_Stream *pStream = NULL;
    if((id < TRX_MAX_STREAMS) && (TRX_RESERVED_ID != handle->registeredStreams[id].id))
    {
        pStream = &(handle->registeredStreams[id]);
    }

    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Stream with id %d resolved to 0x%x", id, pStream);

    return(pStream);
}

TRX_Host_Config *resolveConfig(TRX_Host_Handle handle, uint8_t id)
{
    TRX_Host_Config *pConfig = NULL;
    if((id < TRX_MAX_CONFIGURATIONS) && (TRX_RESERVED_ID != handle->registeredConfigs[id].id))
    {
        pConfig = &(handle->registeredConfigs[id]);
    }

    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Config with id %d resolved to 0x%x", id, pConfig);

    return(pConfig);
}

TRX_Host_Cmd *resolveCmd(TRX_Host_Handle handle, uint8_t slot)
{
    TRX_Host_Cmd *pCmd = NULL;
    if((slot < TRX_MAX_NUM_COMMANDS) && (TRX_RESERVED_ID != handle->registeredCmds[slot].slot))
    {
        pCmd = &(handle->registeredCmds[slot]);
    }

    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Cmd with slot %d resolved to 0x%x", slot, pCmd);

    return(pCmd);
}

SPI_Transport_Last_Status_Map *getEmptyLastStatusMap(TRX_Host_Handle handle)
{
    unsigned int key;
    Plat_Util_startCriticalSection((uintptr_t)&key);
    uint8_t i = 0;
    const uint8_t maxMaps = sizeof(handle->lastStatusMap)/sizeof(SPI_Transport_Last_Status_Map);
    while((i < maxMaps) && (Req_INVALID != handle->lastStatusMap[handle->lastStatusMapIndex].req))
    {
        incrementAndWrap((uint8_t *)&(handle->lastStatusMapIndex), maxMaps);
        i++;
    }

    SPI_Transport_Last_Status_Map *pMap = NULL;
    if(Req_INVALID == handle->lastStatusMap[handle->lastStatusMapIndex].req)
    {
        pMap = &(handle->lastStatusMap[handle->lastStatusMapIndex]);
        pMap->req = Req_CMD_STATUS; // Set to something other than invalid to signal map is consumed
    }
    Plat_Util_endCriticalSection((uintptr_t)&key);

    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Using SPI_Transport_Last_Status_Map 0x%x", pMap);

    return(pMap);
}

void freeLastStatusMap(SPI_Transport_Last_Status_Map *pMap)
{
    // Setting the req to invalid signals that this map is free to use
    pMap->req = Req_INVALID;
}

SPI_Transport_Container *getSpiContainer(TRX_Host_Handle handle, uint8_t minLength)
{
    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Requesting SPI_Transport_Container with at least %d bytes available", minLength);
    SPI_Transport_Container *hostSpiContainer = NULL;

    unsigned int key;
    Plat_Util_startCriticalSection((uintptr_t)&key);
    // Check the tail of the TX Queue first
    hostSpiContainer = (SPI_Transport_Container *)Util_List_tail(&(handle->spiTxQueue));
    if((NULL != hostSpiContainer) && (SPI_TRANSPORT_MAX_TLEN != hostSpiContainer->index))
    {
        const uint8_t padding = PADDING_REQUIRED(hostSpiContainer->index);
        const uint8_t lenWithReq = (hostSpiContainer->index + minLength + padding);

        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF getSpiContainer(): hostSpiContainer->index: %d, Padding: %d, lenWithReq: %d", hostSpiContainer->index, padding, lenWithReq);

        if(SPI_TRANSPORT_MAX_TLEN >= lenWithReq)
        {
            hostSpiContainer = (SPI_Transport_Container *)Util_List_popTail(&(handle->spiTxQueue));

            // Clear the CRC that was created
            (void)memset(&hostSpiContainer->trxSpiBlock.payload[hostSpiContainer->index], 0, SPI_TRANSPORT_CRC_SIZE);

            handle->tid_tx -= 1U;
            if(0U == handle->tid_tx)
            {
                handle->tid_tx = TRN_HDR_TID_TX_MAX_VALUE - 1U;
            }

            Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Using SPI_Transport_Container from the TX queue 0x%x", hostSpiContainer);

        }
        else
        {
            hostSpiContainer = NULL;
        }
    }
    else
    {
        hostSpiContainer = NULL;
    }
    Plat_Util_endCriticalSection((uintptr_t)&key);

    // Otherwise we need to allocate a brand new block
    if((0U < handle->numFreeHostSpiContainers) && (NULL == hostSpiContainer))
    {
        // const uint8_t countBefore = handle->numFreeHostSpiContainers;
        hostSpiContainer = (SPI_Transport_Container *)Buffer_Manager_getBuffer();
        if (NULL != hostSpiContainer)
        {
            handle->numFreeHostSpiContainers -= 1U;
            hostSpiContainer->index = 0U;
            Util_List_clear(&(hostSpiContainer->lastStatusMaps));
            Util_List_setMaxEntries(&(hostSpiContainer->lastStatusMaps), 0xFF);
            // Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: ALLOC: Container allocated 0x%x, Free count: %d -> %d", hostSpiContainer, countBefore, handle->numFreeHostSpiContainers);
        }
        else
        {
            // Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Buffer_Manager_getBuffer() returned NULL despite %d free containers available", countBefore);
        }
    }

    if(NULL == hostSpiContainer)
    {
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: No SPI_Transport_Container available with at least %d bytes available. Free: %d", minLength, handle->numFreeHostSpiContainers);

    }

    return(hostSpiContainer);
}

void freeSpiContainer(TRX_Host_Handle handle, SPI_Transport_Container *pSpiContainer)
{
    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Freeing spiContainer 0x%x, Free count before: %d", pSpiContainer, handle->numFreeHostSpiContainers);

    SPI_Transport_Last_Status_Map *pMap = (SPI_Transport_Last_Status_Map *)Util_List_pop(&(pSpiContainer->lastStatusMaps));
    while(NULL != pMap)
    {
        freeLastStatusMap(pMap);
        pMap = (SPI_Transport_Last_Status_Map *)Util_List_pop(&(pSpiContainer->lastStatusMaps));
    }

    Buffer_Manager_Status fbStatus = Buffer_Manager_freeBuffer((uint8_t *)pSpiContainer);
    if(Buffer_Manager_Success != fbStatus)
    {
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_ERROR,
            "RF: freeSpiContainer: Buffer_Manager_freeBuffer FAILED status=%d ptr=0x%x (LEAK)",
            fbStatus, pSpiContainer);
    }
    handle->numFreeHostSpiContainers += 1U;
    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Container freed, Free count after: %d", handle->numFreeHostSpiContainers);
}

TRX_Request *getNextEmptyRequest(SPI_Transport_Container *pSpiContainer, uint8_t reqSize)
{
    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Requesting next empty req with at least %d bytes available", reqSize);

    TRX_Request *pReq = NULL;
    const uint8_t padding = PADDING_REQUIRED(pSpiContainer->index);
    const uint8_t lenWithReq = (pSpiContainer->index + reqSize + padding);

    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF getNextEmptyRequest(): pSpiContainer->index: %d, padding: %d, lenWithReq: %d",pSpiContainer->index, padding, lenWithReq);

    if(SPI_TRANSPORT_MAX_TLEN >= lenWithReq)
    {
        pSpiContainer->index += padding; // Increment to the next 4-byte boundary
        pReq = (TRX_Request *)&(pSpiContainer->trxSpiBlock.payload[pSpiContainer->index]);
        pSpiContainer->index += reqSize;
    }

    if(NULL == pReq)
    {
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Not enough space available in current SPI_Transport_Container");
    }
    else
    {
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Building request at 0x%x in SPI_Transport_Container 0x%x", pReq, pSpiContainer);
    }

    return(pReq);
}

TRX_Host_Stream *registerStream(TRX_Host_Handle handle, uint8_t id,
                                       uint8_t *pData, uint16_t len,
                                       TRX_Stream_Retention retention,
                                       TRX_Stream_Type type,
                                       TRX_Host_Callback callback,
                                       uint64_t subscribedEvents)
{
    TRX_Host_Stream *pStream = NULL;
    if(id < TRX_MAX_STREAMS)
    {
        pStream = &(handle->registeredStreams[id]);
        pStream->id = id;
        pStream->retention = retention;
        pStream->len = len;
        pStream->data = pData;
        pStream->cb = callback;
        pStream->subscribedEvents = subscribedEvents | TRX_EventLastStatusError | TRX_EventStreamStoreOverflow;
        pStream->index = 0U;
        pStream->type = type;
    }
    return(pStream);
}

TRX_Host_Config *registerConfig(TRX_Host_Handle handle, uint8_t id,
                                       uint8_t *pData, uint16_t len,
                                       uint32_t reference, uint8_t persisted,
                                       TRX_Host_Callback callback,
                                       uint64_t subscribedEvents)
{
    TRX_Host_Config *pConfig = NULL;
    if(id < TRX_MAX_CONFIGURATIONS)
    {
        pConfig = &(handle->registeredConfigs[id]);
        pConfig->id = id;
        pConfig->persisted = persisted;
        pConfig->len = len;
        pConfig->data = pData;
        pConfig->reference = reference;
        pConfig->cb = callback;
        pConfig->subscribedEvents = subscribedEvents | TRX_EventLastStatusError;
    }
    return(pConfig);
}

TRX_Host_Cmd *registerCommand(TRX_Host_Handle handle,
                                     TRX_Request_CommandStore *pCmdStore,
                                     TRX_Host_Callback callback,
                                     uint64_t subscribedEvents)
{
    TRX_Host_Cmd *pCmd = NULL;
    if(pCmdStore->slot < TRX_MAX_NUM_COMMANDS)
    {
        pCmd = &(handle->registeredCmds[pCmdStore->slot]);
        pCmd->slot = pCmdStore->slot;
        pCmd->cb = callback;
        pCmd->subscribedEvents = subscribedEvents | TRX_EventLastStatusError;
        pCmd->cmdStore = pCmdStore;
    }
    return(pCmd);
}

void prepAndSendTransaction(TRX_Host_Handle handle, SPI_Transport_Container *pSpiContainer)
{
    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF:prepAndSendTransaction begins for SPI_Transport_Container 0x%x", pSpiContainer);
    // This operation must happen atomically
    unsigned int key;
    Plat_Util_startCriticalSection((uintptr_t)&key);

    // TODO: TRXHOST-664 There shouldn't be a need to pad here but the SPI driver
    // seems to have some trouble
    if(pSpiContainer->index < SPI_TRANSPORT_MAX_TLEN)
    {
        const uint8_t padding = PADDING_REQUIRED(pSpiContainer->index);

        if((pSpiContainer->index + padding) <=  SPI_TRANSPORT_MAX_TLEN)
        {
            pSpiContainer->index += padding; // Increment to the next 4-byte boundary
        }
    }

    pSpiContainer->trxSpiBlock.header.host.len = pSpiContainer->index;
    pSpiContainer->trxSpiBlock.header.host.tid_tx = handle->tid_tx;
    pSpiContainer->trxSpiBlock.header.host.tid_ack = handle->tid_ack;

    // Update global tid_tx field
    incrementAndWrap((uint8_t *)&handle->tid_tx, TRN_HDR_TID_TX_MAX_VALUE);
    if(0U == handle->tid_tx)
    {
        // TID of 0U is a special case
        handle->tid_tx = 1U;
    }

    // Add CRC to end of transaction (not padded)
    uint8_t *pSpiBlock = (uint8_t *)&(pSpiContainer->trxSpiBlock);
    uint16_t crc = Plat_Util_computeCRC(pSpiBlock, SPI_TRANSPORT_HEADER_LEN + pSpiContainer->index);
    pSpiBlock[SPI_TRANSPORT_HEADER_LEN + pSpiContainer->index] = (uint8_t)((crc >> 8) & 0x00FF);
    pSpiBlock[SPI_TRANSPORT_HEADER_LEN + pSpiContainer->index + 1U] = (uint8_t)(crc & 0x00FF);

    // Send over SPI
    int8_t pushStatus = Util_List_push(&(handle->spiTxQueue), (Util_List_Entry *)pSpiContainer);
    Assert_isTrue((0U == pushStatus), 1U);

    SPI_Transport_Status spiStatus = SPI_Transport_transmit(handle->transportHandle);

    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Sent SPI_Transport_Container 0x%x to SPI layer with status %d", pSpiContainer, spiStatus);

    Assert_isTrue((SPI_Transport_Success == spiStatus), 2U);
    Plat_Util_endCriticalSection((uintptr_t)&key);
}

TRX_Host_Status buildAndSendStream(TRX_Host_Handle handle,
                                   TRX_Host_Stream *pStream,
                                   uint16_t lenToSend,
                                   bool blockForContainer)
{
    TRX_Host_Status status = TRX_Host_Success;
    SPI_Transport_Container *pSpiContainer = NULL;
    uint16_t remainingData = lenToSend;

    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF:buildAndSendStream begins for stream id %d, len %d, Free containers: %d", pStream->id, lenToSend, handle->numFreeHostSpiContainers);
    while(0U < remainingData)
    {
        const uint8_t lenToCopy = (remainingData <= MAX_STREAM_DATA_CHUNK) ? remainingData : MAX_STREAM_DATA_CHUNK;
        while(NULL == pSpiContainer)
        {
            const uint8_t requiredSpace = sizeof(TRX_Request_StreamStore) + lenToCopy;
            pSpiContainer = getSpiContainer(handle, requiredSpace);
            if((NULL == pSpiContainer) && (!blockForContainer))
            {
                Log_printf(LogModule_TRX_HOST_CORE_RF, Log_INFO, "RF: No available SPI_Transport_Containers to build stream %d (needed %d bytes)", pStream->id, requiredSpace);

                status = TRX_Host_No_Resources;
                goto endBuildAndSendStream;
            }
        }
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Got SPI_Transport_Container 0x%x for stream %d chunk, %d bytes to copy", pSpiContainer, pStream->id, lenToCopy);
        
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Building TRX_Request_StreamStore for stream %d with len %d", pStream->id, lenToCopy);

        TRX_Request_StreamStore *pStreamStoreSpiBlock = (TRX_Request_StreamStore *)getNextEmptyRequest(pSpiContainer, sizeof(TRX_Request_StreamStore));
        if(NULL == pStreamStoreSpiBlock)
        {
            while(1);
        }
        pSpiContainer->index += lenToCopy;
        (void)memcpy(pStreamStoreSpiBlock->data, &(pStream->data[pStream->index]), lenToCopy);
        pStreamStoreSpiBlock->header.request = Req_STREAM_STORE;
        pStreamStoreSpiBlock->header.len = sizeof(TRX_Request_StreamStore) + lenToCopy;
        uint8_t md = (pStream->len != (pStream->index + lenToCopy)) ? 1 : 0;
        if (TRX_Stream_Tx == pStream->type || TRX_Stream_Tx_Start == pStream->type)
        {
            pStreamStoreSpiBlock->start = (pStream->index == 0);
        }
        else 
        {
            pStreamStoreSpiBlock->start = 0;
        }
        if (TRX_Stream_Tx == pStream->type || TRX_Stream_Tx_End == pStream->type)
        {
            pStreamStoreSpiBlock->md = md;
        }
        else
        {
            pStreamStoreSpiBlock->md = 1;
        }
        pStreamStoreSpiBlock->retention = pStream->retention;
        pStreamStoreSpiBlock->stream_id = pStream->id;

        SPI_Transport_Last_Status_Map *pMap = getEmptyLastStatusMap(handle);
        if(NULL == pMap)
        {

            Log_printf(LogModule_TRX_HOST_CORE_RF, Log_ERROR, "RF: No available SPI_Transport_Last_Status_Maps to build stream %d", pStream->id);

            // Too many reqs out at once
            status = TRX_Host_No_Resources;
            goto endBuildAndSendStream;
        }

        pMap->cb = (uintptr_t)pStream->cb;
        pMap->id = pStream->id;
        pMap->md = md;
        pMap->req = Req_STREAM_STORE;
        (void)Util_List_push(&(pSpiContainer->lastStatusMaps), (Util_List_Entry *)pMap);

        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Sending SPI_Transport_Container 0x%x for stream %d", pSpiContainer, pStream->id);
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "Stream with id %d calls prepAndSendTransaction", pStream->id);
        prepAndSendTransaction(handle, pSpiContainer);
        pSpiContainer = NULL;

        pStream->index += lenToCopy;
        remainingData -= lenToCopy;

        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Done Building TRX_Request_StreamStore for stream %d. Remaining data: %d", pStream->id, remainingData);
    }

endBuildAndSendStream:
    if(NULL != pSpiContainer)
    {
        Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: Freeing unused SPI_Transport_Container 0x%x at end of buildAndSendStream for stream %d", pSpiContainer, pStream->id);
        freeSpiContainer(handle, pSpiContainer);
    }
    Log_printf(LogModule_TRX_HOST_CORE_RF, Log_VERBOSE, "RF: buildAndSendStream completed for stream %d with status %d, Free containers: %d", pStream->id, status, handle->numFreeHostSpiContainers);
    return(status);
}
