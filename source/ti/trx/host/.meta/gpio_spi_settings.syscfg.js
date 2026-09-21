/*
 * Copyright (c) 2022, Texas Instruments Incorporated
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

/* This file can be loaded in .syscfg files programmatically in order to load
 * GPIO and SPI settings for the Host <--> TRX interface
 */

/* ======== GPIO ======== */
const GPIO = scripting.addModule("/ti/drivers/GPIO");
const GPIO1 = GPIO.addInstance();
const GPIO2 = GPIO.addInstance();
const GPIO6 = GPIO.addInstance();
const GPIO7 = GPIO.addInstance();
const GPIO8 = GPIO.addInstance();
const GPIO9 = GPIO.addInstance();
const GPIO10 = GPIO.addInstance();

const Timer  = scripting.addModule("/ti/drivers/Timer", {}, false);
const Timer1 = Timer.addInstance();

/**
* Write custom configuration values to the imported modules.
*/
GPIO1.interruptTrigger  = "Falling Edge";
GPIO1.$name             = "TRX_HOST_SPI_INT";
GPIO1.pull              = "Pull Up";
GPIO1.gpioPin.$assign   = "boosterpack.23";

GPIO2.$name                         = "TRX_HOST_SPI_CS";
GPIO2.mode                          = "Output";
GPIO2.initialOutputState            = "High";
GPIO2.outputStrength                = "High";
GPIO2.gpioPin.$assignAllowConflicts = "boosterpack.18";
scripting.suppress(".* is connected to LaunchPad SPI Bus .*\. Consider selecting it in 'use hardware' above\. .*", GPIO2, "gpioPin");


GPIO6.$name              = "TRX_RESET";
GPIO6.gpioPin.$assignAllowConflicts    = "boosterpack.36";
GPIO6.mode               = "Output";
GPIO6.initialOutputState = "High";

/* ======== SPI    ======== */
const SPI   = scripting.addModule("/ti/drivers/SPI");
const SPI1  = SPI.addInstance();

const regex = /CC26.4|CC13.4|CC26.3/g;
let boardName = system.deviceData.board.source;
boardName = boardName.replace(/.*\//, ""); /* Strip off everything up to and including the last '/' */
boardName = boardName.replace(/\..*/, ""); /* Strip off everything after and including the first '.' */
if(regex.test(boardName))
{
    SPI1.spi.$assignAllowConflicts = "SPI1";
}
else
{
    SPI1.spi.$assignAllowConflicts = "SSI1";
}

SPI1.$name                             = "TRX_HOST_SPI_CONTROLLER";
SPI1.spi.sclkPin.$assignAllowConflicts = "boosterpack.7";
SPI1.spi.pociPin.$assignAllowConflicts = "boosterpack.14";
SPI1.spi.picoPin.$assignAllowConflicts = "boosterpack.15";
SPI1.sclkPinInstance.$name             = "CONFIG_SPI_CLK";
SPI1.sclkPinInstance.outputStrength    = "High";
SPI1.pociPinInstance.$name             = "CONFIG_SPI_POCI";
SPI1.picoPinInstance.$name             = "CONFIG_SPI_PICO";
SPI1.picoPinInstance.outputStrength    = "High";
SPI1.spi.dmaRxChannel.$assignAllowConflicts = "DMA_CH16";
SPI1.spi.dmaTxChannel.$assignAllowConflicts = "DMA_CH17";
scripting.suppress("Connected to hardware.* is connected to LaunchPad SPI Bus on .*\. Consider selecting it in 'use hardware' above\. .*", SPI1.spi, "sclkPin");
scripting.suppress("Connected to hardware.* is connected to LaunchPad SPI Bus on .*\. Consider selecting it in 'use hardware' above\. .*", SPI1.spi, "pociPin");
scripting.suppress("Connected to hardware.* is connected to LaunchPad SPI Bus on .*\. Consider selecting it in 'use hardware' above\. .*", SPI1.spi, "picoPin");
scripting.suppress("Resource conflict.*", SPI1.spi, "dmaRxChannel");
scripting.suppress("Resource conflict.*", SPI1.spi, "dmaTxChannel")

/* ========== Timer ============ */
Timer1.$name               = "CONFIG_TIMER_0";
Timer1.useGPTimer          = true;
Timer1.timerInstance.$name = "CONFIG_GPTIMER_0";

/* ============== Log ============ */
const enableLogs = false;
if(enableLogs)
{
    const log_settings = system.getScript("/ti/trx/host/.meta/trx_host_logging.syscfg.js");
}
