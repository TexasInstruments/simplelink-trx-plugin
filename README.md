# SimpleLink™ Low Power Transceiver (TRX) Plugin

This repository redistributes TI's [SimpleLink™ Low Power Transceiver (TRX)
Plugin](https://www.ti.com/tool/download/SIMPLELINK-TRX-PLUGIN), which provides 
the components needed to work with Texas Instruments
CC140xP FSK, O-QPSK and OFDM transceivers. The TRX PLUGIN is one of many within
the [SimpleLink Low Power ecosystem](https://www.ti.com/tool/SIMPLELINK-LOWPOWER-SDK).

The CC140xP transceivers can be used with any host device with a compatible SPI
interface. This plugin includes a host driver and examples implemented for the
F2 family of wireless connectivity MCUs (CC1312R7, CC1352P7, CC2652R7,
CC1314R10 and CC1354P10). The host driver is designed for portability to other
designs and platforms.

This plugin is intended to work with the [SimpleLink Low Power F2 SDK](https://github.com/TexasInstruments/simplelink-lowpower-f2-sdk). 
You will need to install the F2 SDK before using this plugin.

This is version 1.00.00.52 of the SimpleLink TRX Plugin.

## What's New

  - Device Configuration API can be used to set allow/disallow standby,
    to set the GPIO configuration and for the RF Mode. Please see the
    API guide for details.
  - The BP-CC140XP board has been divided into one 868 MHz band variant
    (BP-CC140XP-8-FE) and one 915 MHz band variant (BP-CC140XP-9-FE).
    Please make sure to adjust the frequency settings in the embedded
    example appropriately, since the SAW filter on these boards differ.
## About this Repository

Although TI also offers this plugin via [a classic installer](https://www.ti.com/tool/download/SIMPLELINK-TRX-PLUGIN), it's
available as a Git repository to cater to various use cases:

* **CI/CD tooling integration**: Using Git instead of installers can ease
  integration with CI/CD tooling pipelines.
* **Increased visibility and change tracking**: Git improves the ability to
  identify changes that may affect (or not!) your application.
* **Simplified workflow for rebasing**: Git makes it simpler for you to rebase
  your changes to new releases.

## Setup Instructions

### Edit **imports.mak**

At the base of the plugin, you will find **imports.mak**; this file is used by
the build system to find your installed dependencies, including the SimpleLink
Low Power F2 SDK. Please update **imports.mak** with full paths to where you
have these dependencies installed on your system.

Notably for Windows users, the _Windows_ variant of some tools may be required.
For example, the `SYSCONFIG_TOOL` will need to have a **.bat** extension.

Refer to the comments in **imports.mak** for details on setting variables and
recommended versions. Also see the [Resources](#resources) section below for
download locations.

### Build the Plugin

With a few exceptions (libraries without sources), the plugin is provided
without prebuilt libraries.

If you have cloned the SimpleLink Low Power F2 SDK from github you will first
have to build the SDK libraries following the instructions in the SDK readme
file. If you have installed the SimpleLink Low Power F2 SDK from www.ti.com this
is not needed.

After editing **imports.mak** as described above, navigate to the root of the
plugin (the same directory where **imports.mak** resides) and build using CMake
and the SDK toolchain, following the same conventions as the SimpleLink Low
Power F2 SDK.

## Resources

Dependency download locations:

* [SimpleLink Low Power F2 SDK](https://www.ti.com/tool/SIMPLELINK-LOWPOWER-F2-SDK)
  * Alternatively, download the [SimpleLink Low Power F2 SDK on github](https://github.com/TexasInstruments/simplelink-lowpower-f2-sdk)
* [SysConfig (SYSCONFIG_TOOL)][sysconfig download]
  * This is also included with CCS
* [TI CLANG Compiler (TICLANG_ARMCOMPILER)][ticlang download]
* [ARM Gnu Toolchain (GCC_ARMCOMPILER)][gcc download]
* [CMake](https://cmake.org/)
* [GNU make](https://www.gnu.org/software/make/)
  * This is also included with CCS in the **ccs/utils/bin/** directory (and
    called `gmake`).

## Technical Support

Please consider creating a post on [TI's E2E forum](https://e2e.ti.com).

[sysconfig download]: https://www.ti.com/tool/SYSCONFIG
[ticlang download]: https://www.ti.com/tool/download/ARM-CGT-CLANG
[gcc download]: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
