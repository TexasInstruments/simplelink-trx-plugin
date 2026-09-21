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

#include <stdint.h>

/* Driver Header files */
#include <ti/drivers/Board.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/display/Display.h>

#include "ssbl_host.h"
#include "ti_drivers_config.h"

extern void *spinUpApplicationTasks();

/* Stack size in bytes */
#define THREADSTACKSIZE 1024

/*
 * This will also be used by performSsblDownloadSequence to display the resulting
 * TRX FW hash if the download sequence is successful
 */
Display_Handle display;

/*
 * Warning - This is only an example key. This should not be used for production devices.
 *
 * This key should only be used for early testing. This is the same public key that can
 * be found at the root of this example folder alongside it's pairing private key.
 *
 * Open source tools such as OpenSSL can be used to generate your own private/public key
 * pair. The private key of which should be securely maintained with very limited access
 * protocols. The public key from the pair can be swapped in for this example key for
 * continued testing
 *
 */
// This variable may not be used depending on the needs of the user
#if __ICCARM__
    #pragma diag_suppress = Pe177
#elif __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-const-variable"
#endif
// clang-format off
static const ssblCustKey_t customerKey = {
    .keyId = { 0xF0, 0x0D, 0xCA, 0xFE, 0xBE, 0xEF, 0xFE, 0xED },
    .key = {{
        /* Uncompressed Prefix */
        0x04,

        /* X Component */
        0xce,0x7d,0x73,0x79,0xad,0xb5,0x64,0x3a,0x58,0x25,0xe9,0xd5,0xd4,0x81,0x21,0x0d,
        0x63,0x73,0x87,0x34,0xd8,0x98,0x35,0xf1,0x05,0x34,0x6c,0xf6,0x30,0xd6,0x91,0x2f,

        /* Y Component */
        0x6c,0x92,0xe0,0x81,0x4d,0x05,0xf1,0x3a,0x8d,0x3e,0x28,0x22,0xcf,0x9e,0xe9,0x29,
        0xfb,0x3c,0x0f,0x38,0xd7,0x50,0x7e,0x6f,0x4f,0x77,0xa3,0xe3,0x15,0xbb,0x51,0x27
    }},
};
// clang-format on
#if __ICCARM__
    #pragma diag_default = Pe177
#elif __GNUC__
    #pragma GCC diagnostic pop
#endif

static const uint32_t *trxFwUpdateBuffer = (uint32_t *)0x10000;

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions. */
    Display_init();
    GPIO_init();

    /* Configure the LED pins */
    GPIO_setConfig(CONFIG_GPIO_LED_RED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_GREEN, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    /* Open the display for output */
    display = Display_open(Display_Type_UART, NULL);
    if (display == NULL)
    {
        /* Failed to open display driver */
        while (1) {}
    }

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_RED, CONFIG_GPIO_LED_ON);

    Display_printf(display, 0, 0, "Starting the Secure Serial Bootloader(SSBL) Host example");
    Display_printf(display,
                   0,
                   0,
                   "This example requires external wires to be connected to the header pins."
                   " Please see the Readme.html for further details. Additionally, this "
                   "example assumes the 'trx_fw_update_final.bin' has been programmed at"
                   " 0x10000.\n");

    /*
     * This is one example of how your application should setup the ssblDownload_t struct for the
     * SSBL Host. Your usage may be slightly different. Please configure the ssblDownload_t struct
     * to fit your use case. See <DOCS> to learn more on the options.
     *
     * If optional customer key signature verification is desired then the customer public key must
     * be programmed to the CC140X device before a FW update can be successful. By running this
     * example with
     *     * ( ssblHostDownloadStruct.programKey == true ) and
     *     * ( ssblHostDownloadStruct.custKey == &customerKey )
     * the SSBL Host will handle programming the provided customerKey to the CC140X.
     *
     * **Warning** - Programming the customer public key is a one time operation. It cannot be
     * undone and a new/different public key cannot be programmed after the first key is programmed.
     * Be VERY sure you want to program the customer key before you continue.
     */
    const ssblDownload_t ssblHostdownloadStruct = {
        .programKey        = false,                        /* ** See warning above ** */
        .custKey           = NULL,                         // &customerKey,   /* ** See warning above ** */
        .trxFwUpdateBuffer = (uint8_t *)trxFwUpdateBuffer, /* Must be word aligned */
        .triggerSsbl       = true,
    };

    Display_printf(display, 0, 0, "SSBL Host configured and beginning download sequence...\n");

    BLDR_STATUS_T status;
    if (BLDR_CMD_RET_SUCCESS != (status = performSsblDownloadSequence(&ssblHostdownloadStruct)))
    {

        Display_print1(display, 0, 0, "Download Sequence Failed. Status: 0x%x\n", status);
        while (1)
        {
            ;
        }
    }

    Display_printf(display, 0, 0, "Download sequence completed successfully :) !!\n");

    GPIO_write(CONFIG_GPIO_LED_RED, CONFIG_GPIO_LED_OFF);
    GPIO_write(CONFIG_GPIO_LED_GREEN, CONFIG_GPIO_LED_ON);

    /*
     * Now that the SSBL Host work is complete. Continue on to spin up the actual application/stack tasks
     */
    spinUpApplicationTasks();

    /*
     * Since this thread was created as a detached thread the resources for this thread will be released
     * after returning.
     */
    return (NULL);
}
