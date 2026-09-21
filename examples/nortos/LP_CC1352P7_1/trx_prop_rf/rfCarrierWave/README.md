# rfcarrierwave

---

Project Setup using the System Configuration Tool (SysConfig)
-------------------------
The purpose of SysConfig is to provide an easy to use interface for configuring
drivers, RF stacks, and more. The .syscfg file provided with each example
project has been configured and tested for that project. Changes to the .syscfg
file may alter the behavior of the example away from default. Some parameters
configured in SysConfig may require the use of specific APIs or additional
modifications in the application source code. More information can be found in
SysConfig by hovering over a configurable and clicking the question mark (?)
next to it's name.

Example Summary
---------------
The carrier wave (CW) example sends a continuous carrier wave or pseudo-random
modulated signal on a fixed frequency.

Peripherals Exercised
---------------------
* `TRX_HOST_SPI_CONTROLLER` - SPI interface between Host device and TRX. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `CONFIG_GPTIMER_0` - Timer used in TRX RF Host driver operation. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_HOST_SPI_INT` - IO from TRX to Host indicating SPI message from TRX --> Host. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_HOST_SPI_CS` - SPI Chip Select IO. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_RESET` - IO used to reset the TRX from the Host. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer

Resources & Jumper Settings
---------------------------
> If you're using an IDE (such as CCS or IAR), please refer to Board.html in your project
directory for resources used and board-specific jumper settings. Otherwise, you can find
Board.html in the directory &lt;SDK_INSTALL_DIR&gt;/source/ti/boards/&lt;BOARD&gt;.

Board Specific Settings
-----------------------
The TRX Host RF Driver uses a HAL layer called the "platform". All IO, SPI, and Timer settings must be adjusted/implemented according to the host device setup being used. By default, this example will have a correct HAL configuration out-of-box for all supported TI launchpads.

Note: If you are using the BP-CC140xP-8-FE as the evaluation board for the CC140x device, the frequency range to use in the examples must be 862-870 MHz.
If you are using the BP-CC140xP-9-FE as the evaluation board for the CC140x device, the frequency range to use in the examples must be 902-928 MHz.
These limitations are due to the presence of a SAW filter which limit the frequency range on RX.

Example Usage
-------------
Run the example.

Application Design Details
--------------------------
This examples consists of a single task.

To switch between carrier wave (0) and modulated signal (1) and PN9 (2) set the
following in the code (CW is set as default):

    cmdTxTest.mode = TRX_TxTestMode_Unmodulated; // Valid options: TRX_TxTestMode_Unmodulated, TRX_TxTestMode_Modulated, TRX_TxTestMode_PN9
    cmdTxTest.pattern = 0xAB; // 1-byte pattern modulated during transmission. Only used if mode = TRX_TxTestMode_Modulated

When the task is executed it:

1. Initializes and gets access to the radio via the TRX Host RF Driver's TRX_Host_open API
2. Explicitly configures a TX test command including
    * Carrier wave mode and pattern
    * Frequency
    * TX Power
    * PHY
3. Sends the TX test command to the TRX using the TRX_Host_storeCmds API which then starts sending the modulated or unmodulated signal forever
