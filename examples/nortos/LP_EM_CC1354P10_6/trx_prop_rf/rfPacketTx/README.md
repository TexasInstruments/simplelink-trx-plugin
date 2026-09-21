# rfPacketTx

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
The Packet TX example illustrates how to do simple packet transmission using
the TRX Host RF driver. This example is meant to be used with the Packet RX
example or SmartRF Studio. For every packet transmitted, the green LED is toggled.

Extended Radio Commands
-----------------------
The GenFSK variant of this example (rfPacketTxGenFSK.c) can transmit using
either the regular Transmit command or the extended Transmit command
(TRX_RadioCommand_ExtTransmit). Set `USE_EXTENDED_COMMANDS` to 1 at the top of
rfPacketTxGenFSK.c to select the extended command. When using the extended commands:
* Frequency is specified in Hz (the regular command uses kHz). The example
  receives on the same RF frequency in both modes.
* fixed_length and rssi_threshold fields are available; 
* On PHYs supporting fixed length transmission, the fixed length is the length of transmission and reception. On PHYs supporting only variable length transmission, this field is ignored on TX and is used as the maximum length filter on the RX.
* The RSSI treshold is only relevant when using search strategies energy/ preamble detect used in carrier sense/ sniff mode. We leave that field for its default value in this example.

Peripherals Exercised
---------------------
* `CONFIG_PIN_GLED` - Toggled when data is transmitted over the RF interface
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
Run the example. On another board, run the Packet RX example.
CONFIG_PIN_GLED is toggled when data is transmitted.

Application Design Details
--------------------------
This examples consists of a single task. When the task is executed it:

1. Initializes and gets access to the radio via the TRX Host RF Driver's TRX_Host_open API
2. Explicitly configures an TX command including
    * Frequency
    * Power
    * PHY and Modem
    * Trigger Type
    * Payload Buffer
3. Sends the TX command to the TRX using the TRX_Host_storeCmds API which then starts transmitting
4. Receives a callback from the TRX RF Host driver when a packet is transmitted and toggles CONFIG_PIN_GLED
5. Sleeps for the packet interval
6. Transmits packets forever by repeating steps 3-5
