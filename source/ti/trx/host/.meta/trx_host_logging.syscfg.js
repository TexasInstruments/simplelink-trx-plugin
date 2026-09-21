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
 * Logging settings for the Host <--> TRX interface
 */

/* ============== Log ============ */
const LogModule = scripting.addModule("/ti/log/LogModule");
LogModule.enable_ERROR = true;
LogModule.enable_WARNING = true;
LogModule.enable_INFO = true;
LogModule.enable_VERBOSE = true;

const ITM = scripting.addModule("/ti/drivers/ITM");

const LogModule_TRX_HOST_CORE_SPI = LogModule.addInstance();
LogModule_TRX_HOST_CORE_SPI.$name = "LogModule_TRX_HOST_CORE_SPI";
LogModule_TRX_HOST_CORE_SPI.enable_ERROR   = true;
LogModule_TRX_HOST_CORE_SPI.enable_WARNING = true;
LogModule_TRX_HOST_CORE_SPI.enable_INFO    = false;
LogModule_TRX_HOST_CORE_SPI.enable_VERBOSE = false;
LogModule_TRX_HOST_CORE_SPI.loggerSink = "/ti/log/LogSinkITM";

const LogModule_TRX_HOST_HAL = LogModule.addInstance();
LogModule_TRX_HOST_HAL.$name = "LogModule_TRX_HOST_HAL";
LogModule_TRX_HOST_HAL.enable_ERROR   = true;
LogModule_TRX_HOST_HAL.enable_WARNING = true;
LogModule_TRX_HOST_HAL.enable_INFO    = false;
LogModule_TRX_HOST_HAL.enable_VERBOSE = false;
LogModule_TRX_HOST_HAL.loggerSink = "/ti/log/LogSinkITM";

const LogModule_TRX_HOST_CORE_RF = LogModule.addInstance();
LogModule_TRX_HOST_CORE_RF.$name = "LogModule_TRX_HOST_CORE_RF";
LogModule_TRX_HOST_CORE_RF.enable_ERROR   = true;
LogModule_TRX_HOST_CORE_RF.enable_WARNING = true;
LogModule_TRX_HOST_CORE_RF.enable_INFO    = false;
LogModule_TRX_HOST_CORE_RF.enable_VERBOSE = false;
LogModule_TRX_HOST_CORE_RF.loggerSink = "/ti/log/LogSinkITM";

const LogModule_App= LogModule.addInstance();
LogModule_App.$name = "LogModule_App";
LogModule_App.enable_ERROR   = true;
LogModule_App.enable_WARNING = true;
LogModule_App.enable_INFO    = true;
LogModule_App.enable_VERBOSE = false;
LogModule_App.loggerSink = "/ti/log/LogSinkITM"

ITM.swoPin.$assignAllowConflicts  = "boosterpack.32";
