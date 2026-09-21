/*
 * Copyright (c) 2022-2025, Texas Instruments Incorporated
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

#ifndef __BLDRCMDS_H__
#define __BLDRCMDS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bootloader Command ID Enum
 *
 */
typedef enum
{
    BLDR_CMD_ID_PING         = 0,  ///< Ping Command
    BLDR_CMD_ID_GET_STATUS   = 1,  ///< Get Status Command
    BLDR_CMD_ID_GET_SEC_VER  = 2,  ///< Get Security Version Command
    BLDR_CMD_ID_BREAK_OUT    = 3,  ///< Break Out Command
    BLDR_CMD_ID_RESET        = 4,  ///< Reset Command
    BLDR_CMD_ID_GET_PART_ID  = 5,  ///< Get Part ID Command
    BLDR_CMD_ID_GET_KEY_ID   = 6,  ///< Get Customer Key ID Command
    BLDR_CMD_ID_SET_CUST_KEY = 7,  ///< Set Customer Key Command
    BLDR_CMD_ID_DOWNLOAD     = 8,  ///< Download Command
    BLDR_CMD_ID_BANK_ERASE   = 9,  ///< Bank Erase Command
    BLDR_CMD_ID_SEND_DATA    = 10, ///< Send Data Command
    BLDR_CMD_ID_GET_FW_HASH  = 11, ///< Get Firmware Hash
} BLDR_CMD_ID_T;

/**
 * @brief Bootloader Status Enum
 *
 */
typedef enum
{
    BLDR_CMD_RET_SUCCESS      = 0x40, ///< Success Status
    BLDR_CMD_RET_UNKNOWN_CMD  = 0x41, ///< Unknown Command Status
    BLDR_CMD_RET_INVALID_CMD  = 0x42, ///< Invalid Command Status
    BLDR_CMD_RET_INVALID_ADR  = 0x43, ///< Invalid Address Status
    BLDR_CMD_RET_FLASH_FAIL   = 0x44, ///< Flash Failure Status
    BLDR_CMD_RET_SEC_VER_FAIL = 0x45, ///< Security Version Failure status
    BLDR_CMD_RET_OUT_OF_ORDER = 0x46, ///< Out of Order Commands status
    BLDR_CMD_RET_NO_APP       = 0x47, ///< No App (Can't breakout) status
    BLDR_CMD_RET_AUTH_FAIL    = 0x48, ///< Authentication Failure status
    BLDR_CMD_RET_FAILURE      = 0x49, ///< General Failure status
} BLDR_STATUS_T;

//*****************************************************************************
//
//! A helpful define which specifies the overhead each and every cmd has
//! [ length, length ] ... cmd_payload ...  [ crc, crc, crc, crc ]
//
//*****************************************************************************
#define BLDR_CMD_OVERHEAD (6U)

//*****************************************************************************
//
//! A helpful define which specifies the constant front overhead of a download
//! cmd. This only includes the content before the variably sized blockHashes.
//!   [ imgID, imgID, secVer, size, size, size, size, addr, addr, addr, addr ]
//
//*****************************************************************************
#define BLDR_DOWNLOAD_PAYLOAD_FRONT_OVERHEAD (11U)

//*****************************************************************************
//
//! A helpful define which specifies the constant overhead of a download cmd.
//! This does not include the variably sized blockHashes portion of the cmd.
//! +   ... block hashes ...
//! + [ ti_signature ] + [ cust_signature ]
//
//*****************************************************************************
#define BLDR_DOWNLOAD_CMD_OVERHEAD (BLDR_DOWNLOAD_PAYLOAD_FRONT_OVERHEAD + 64U + 64U)

//*****************************************************************************
//
//! A helpful define which specifies the constant overhead of a send data cmd.
//! This does not include the variably sized block data portion of the cmd.
//!   [ size, size, size, size, num, num, num, num]
//
//*****************************************************************************
#define BLDR_SEND_DATA_CMD_OVERHEAD (8U)

//*****************************************************************************
//
//! This is the value that is sent to acknowledge a packet.
//
//*****************************************************************************
#define BLDR_CMD_ACK (0xCCU)

//*****************************************************************************
//
//! This is the value that is sent to not-acknowledge a packet.
//
//*****************************************************************************
#define BLDR_CMD_NAK (0x33U)

#ifdef __cplusplus
}
#endif

#endif // __BLCMDS_H__
