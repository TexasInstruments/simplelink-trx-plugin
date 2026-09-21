# rfMdrRx

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
NOTE: MDR examples are applicable only when using CC1407 tranceiver.

The WiSUN Modulation and Data Rate (MDR) negotiation receive example shows a
simplified implementation of a receiving MDR packets with user selectable
PHYs, channels, and payload lengths. It also provided an example of how to
implement Sync Frame Detection (SFD) using the TRX, the TRX Host Driver, and
an interrupt setup on an IO. For every packet received, CONFIG_PIN_GLED is
toggled and for every sync frame detected CONFIG_PIN_RLED is toggled.

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
Run the example and an infinite MDR capable RX command is automatically issued
to the TRX radio. On another board, run any packet TX example of the same base
PHY and frequency. When a packet is received, CONFIG_PIN_GLED is
toggled and when a sync frame is detected CONFIG_PIN_RLED is toggled. Note that
when using the MDR TX examples without modification, the RLED toggle may not be
visible due to the short timing between sync frames.

Application Design Details
--------------------------
This examples consists of a single task. When the task is executed it:

1. Initializes and gets access to the radio via the TRX Host RF Driver's TRX_Host_open API
2. Initializes GPIO for the LEDs and Sync Frame Detection (SFD)
3. Stores a front end and WiSUN PHY configuration using TRX_Host_storeConfig
4. Based on the PHY and base channel being used determines the appropriate
frequency delta configuration to use and loads it to the TRX using TRX_Host_storeConfig
5. Explicitly configures an RX command including
    * Frequency
    * PHY and Modem
    * Base index of the RX channel in the frequency delta table
    * Trigger Type
    * Payload Buffer
    * Repeat
    * MDR Enabled
6. Sends the repeating RX command to the TRX using the TRX_Host_storeCmds API which then starts receiving
7. Receives a callback from the TRX RF Host driver when a packet is received and toggles CONFIG_PIN_RLED when the sync frame is detected and CONFIG_PIN_GLED
when the packet data is received
