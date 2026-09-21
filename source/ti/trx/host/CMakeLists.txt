# Copyright (c) 2022, Texas Instruments Incorporated
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# *  Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# *  Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# *  Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION ${TI_MIN_CMAKE_VERSION})
include(CMakeSources.cmake)

project(trxhost LANGUAGES C ASM)

set (trxhost_include_dirs
    ${CORESDK_CC13XX_CC26XX_INSTALL_DIR}/source
)

set (trxhost_defs)
set (trxhost_defs_log
    ti_log_Log_ENABLE
    ti_log_Log_ENABLE_LogModule_TRX_HOST_CORE_SPI=1
    ti_log_Log_ENABLE_LogModule_TRX_HOST_CORE_RF=1
)

set(trxhost_common_ticlang_opts
    "-fno-strict-aliasing"
)
set(trxhost_common_gcc_opts)
set(trxhost_common_iar_opts)


# Utility fxn that works well with conventions the trxhost libraries follow
function(trxhost_add_lib libname tiarch srcs cdefs extracopts extracincs)
    # local utility variable
    set (ref ${libname}_${tiarch})

    ti_add_library(${ref}
        SOURCES ${srcs}
        OUTPUT_NAME ${libname}
        ADD_SDK_INCLUDE_PATH "source"
    )

    # This strange business exports libraries outside of this function, where
    # ti_export_package() neeeds it
    #
    # TODO, cmake-common needs to better support this.  Maybe
    # ti_export_package() needs to take (optionally?) these two args.
    set(TI_LIBRARIES ${TI_LIBRARIES} PARENT_SCOPE)
    set(TI_COMPONENT_TARGETS ${TI_COMPONENT_TARGETS} PARENT_SCOPE)

    target_compile_options(${ref}
        PRIVATE
        ${trxhost_common_${TI_TOOLCHAIN_NAME}_opts}
        ${extracopts}
    )

    # Specify the default compiler settings and other toolchain-related
    # configurations
    # TODO: add this to ti_add_library()
    target_link_libraries(${ref}
        PRIVATE CMakeCommon::${TI_TOOLCHAIN_NAME}_${tiarch}
    )

    target_compile_definitions(${ref}
        PRIVATE ${cdefs}
    )

    target_include_directories(${ref}
        PUBLIC
        ${extracincs}
    )
endfunction()

ti_init_package(NAMESPACE trxhost)

foreach(TI_ARCH m4f m33f)
    message (DEBUG "lib:  ${TI_TOOLCHAIN_NAME} + ${TI_ARCH}")

    trxhost_add_lib(trx_host ${TI_ARCH}
            "${SOURCES_TRXHOST}"
            "${trxhost_defs}"
            ""
            "${trxhost_include_dirs}"
        )

    trxhost_add_lib(trx_host_log ${TI_ARCH}
        "${SOURCES_TRXHOST}"
        "${trxhost_defs};${trxhost_defs_log}"
        ""
        "${trxhost_include_dirs}"
    )
endforeach()

ti_export_package()