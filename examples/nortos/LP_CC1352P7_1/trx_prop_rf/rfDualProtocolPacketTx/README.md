# rfDualProtocolPacketTx

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
The Dual Protocol Packet TX example illustrates how to transmit packets using two
different protocols alternately on a dual protocol PHY with the TRX Host RF driver.
The tranmsitter transmits packets alternating between FSK and OQPSK modulations
on two different frequencies. This example is meant to be used with another device
programmed with the Dual Protocol Packet RX example. For every OQPSK packet
transmitted, the green LED is toggled. For every FSK packet transmitted, the red
LED is toggled.

Peripherals Exercised
---------------------
* `CONFIG_PIN_GLED` - Toggled when OQPSK packets are transmitted over the RF interface
* `CONFIG_PIN_RLED` - Toggled when FSK packets are transmitted over the RF interface
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
Run the example. On another board, run the Dual Protocol Packet RX example,
which will receive packets from both protocols. This TX example will alternately
transmit packets using OQPSK and FSK modulation every 2 seconds. CONFIG_PIN_GLED
is toggled when OQPSK packets are transmitted. CONFIG_PIN_RLED is toggled when
FSK packets are transmitted. This allows you to visually confirm transmission
from both protocols.

Application Design Details
--------------------------
This example consists of a single task. When the task is executed it:

1. Initializes and gets access to the radio via the TRX Host RF Driver's TRX_Host_open API
2. Sets the RF mode to Proprietary FSK using TRX_Host_utilSetRfMode API
3. Erases non-volatile storage on the TRX to ensure a clean configuration state
4. Stores the dual-protocol PHY configuration (supporting both FSK and OQPSK) to the TRX
5. Persists the configuration to non-volatile memory for faster subsequent startups
6. Explicitly configures and stores an OQPSK TX command (slot 0) including:
    * Frequency (868 MHz)
    * Modem type (OQPSK)
    * PHY configuration ID
7. Explicitly configures and stores a FSK TX command (slot 1) including:
    * Frequency (869 MHz)
    * Modem type (FSK)
    * Same PHY configuration ID as OQPSK
8. Enters an infinite transmission loop that:
    * Alternates between OQPSK and FSK
    * Updates the PHY header based on the selected modulation
    * Stores the packet data stream to the TRX using TRX_Host_storeStream API
    * Submits the appropriate TX command (OQPSK or FSK) using TRX_Host_submitCmd API
    * Toggles CONFIG_PIN_GLED for OQPSK or CONFIG_PIN_RLED for FSK transmissions
    * Sleeps for 2 seconds before the next transmission


Transmitted packets include:
* Header (PHY-specific format: 4 bytes for SUN OQPSK, 1 byte for proprietary FSK)
* Payload (20 bytes filled with 0xAA for both modulations)
