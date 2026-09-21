cmake_minimum_required(VERSION 3.21)

set(TICLANG_ARMCOMPILER $ENV{TICLANG_ARMCOMPILER})
set(IAR_ARMCOMPILER $ENV{IAR_ARMCOMPILER})
set(GCC_ARMCOMPILER $ENV{GCC_ARMCOMPILER})

# Only look in local folders for components and platforms
set(CMAKE_ONLY_WRAPPER_COMPONENTS 1)
set(CMAKE_ONLY_LOCAL_PLATFORMS 1)

# ensure SDK env var exists and expose it as a CMake variable
if(NOT DEFINED ENV{SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR})
    message(FATAL_ERROR "Environment variable SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR is not set")
endif()
set(SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR $ENV{SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR})

# Setup the CMake build environment
set(CMAKE_COMMON_INSTALL_DIR ${CMAKE_SOURCE_DIR})
include(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/cmake/scripts/cmake-common/Configure.cmake)
project(coresdk LANGUAGES)

# Include directories that participate in the library build
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/kernel/freertos)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/kernel/nortos)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/kernel/tirtos7/packages/ti/sysbios)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/third_party/ecc)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/third_party/fatfs)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/third_party/psa_crypto/library)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/third_party/spiffs)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/devices/cc13x1_cc26x1)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/devices/cc13x2_cc26x2)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/devices/cc13x2x7_cc26x2x7)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/devices/cc13x4_cc26x4)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/display)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/drivers)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/log)
add_subdirectory(source/ti/trx/host)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/utils)
add_subdirectory(${SIMPLELINK_CC13XX_CC26XX_SDK_INSTALL_DIR}/source/ti/utils/json)