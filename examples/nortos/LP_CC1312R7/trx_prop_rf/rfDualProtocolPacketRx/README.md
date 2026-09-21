# rfDualProtocolPacketRx

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
The Dual Protocol Packet RX example demonstrates the dual-protocol receive capability
of the TRX. This feature allows the receiver to simultaneously listen to syncwords
from both FSK and OQPSK modulations on the same or different frequencies and starts
demodulating the packet on the first received syncword.
This example is meant to be used with another device programmed with the Dual
Protocol Packet TX example. For every OQPSK packet received, the green LED is toggled.
For every FSK packet received, the red LED is toggled.

Peripherals Exercised
---------------------
* `CONFIG_PIN_GLED` - Toggled when OQPSK packets are received over the RF interface
* `CONFIG_PIN_RLED` - Toggled when FSK packets are received over the RF interface
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
Run the example. On another board, run the Dual Protocol Packet TX example,
which will alternately transmit packets using both FSK and OQPSK modulation.
This RX example will receive packets from both protocols. CONFIG_PIN_GLED is
toggled when OQPSK packets with CRC OK are received. CONFIG_PIN_RLED is toggled
when FSK packets with CRC OK are received. The modem that received the packet is
identified from the modem mask prepended to each received packet. This allows you
to visually confirm reception from both protocols.

Application Design Details
--------------------------
This example consists of a single task. When the task is executed it:

1. Initializes and gets access to the radio via the TRX Host RF Driver's TRX_Host_open API
2. Erases non-volatile storage on the TRX to ensure a clean configuration state
3. Sets the RF mode to Proprietary FSK using TRX_Host_utilSetRfMode API
4. Stores the dual-protocol PHY configuration (supporting both FSK and OQPSK) to the TRX
5. Persists the configuration to non-volatile memory
6. Explicitly configures a dual RX command including:
    * Two frequency channels (868 MHz and 869 MHz in this example)
    * Dual modem support (FSK | OQPSK)
    * Repeat mode enabled for continuous reception
7. Registers the RX buffer to store incoming packet data
8. Sends the repeating dual RX command to the TRX using the TRX_Host_storeCmds API, which starts receiving
9. Receives callbacks from the TRX RF Host driver when packets are received from either protocol
10. Reads the modem mask prepended to the received packet to determine which modem received it
11. Toggles CONFIG_PIN_GLED for OQPSK packets or CONFIG_PIN_RLED for FSK packets
12. Runs indefinitely, continuously receiving packets from both protocols


Note
----
The modem on which the packet is received is represented by a 1 Byte long field (TRX_RadioCommand_Modem) prepended to the received packet.
The receiver cannot receive two packets simultaneously. Once a sync word is detected, the receiver will proceed to demodulate solely on that channel until packet completion (successful reception or CRC failure).