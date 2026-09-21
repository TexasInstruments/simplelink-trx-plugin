# rfMdrTx

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

The WiSUN Modulation and Data Rate (MDR) negotiation transmit example shows a
simplified implementation of a transmitting MDR packets with user selectable
PHYs, channels, and payload lengths by chaining two TX commands. When
CONFIG_GPIO_BTN1 is pressed, the example will send the user selected number
of packets and for every packet transmitted, CONFIG_PIN_GLED is toggled.

Peripherals Exercised
---------------------
* `TRX_HOST_SPI_CONTROLLER` - SPI interface between Host device and TRX. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `CONFIG_GPTIMER_0` - Timer used in TRX RF Host driver operation. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_HOST_SPI_INT` - IO from TRX to Host indicating SPI message from TRX --> Host. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_HOST_SPI_CS` - SPI Chip Select IO. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_RESET` - IO used to reset the TRX from the Host. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `CONFIG_GPIO_BTN1` - Button to start sending packets

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
Run the example and press CONFIG_GPIO_BTN1 then MDR transmissions are issued
to the TRX radio. On another board, run the rfMdrRx example of the same base
PHY and frequency. When a packet is transmitted, CONFIG_PIN_GLED is
toggled.

Application Design Details
--------------------------
This examples consists of a single task. When the task is executed it:

1. Initializes and gets access to the radio via the TRX Host RF Driver's TRX_Host_open API
2. Stores a front end and WiSUN PHY configuration using TRX_Host_storeConfig
3. Based on the PHY and base channel being used determines:
    * Frequency of the first TX (Header)
    * Identifier of the PHY the payload is transmitted on as per WiSUN spec
    * Frequency of the second TX (Payload) as per WiSUN spec
4. Explicitly configures two TX commands including
    * Frequency
    * PHY and Modem
    * Trigger Type
    * Payload Buffer
    * Chaining first TX (Header) to the second TX (Payload)
5. Stores the data streams to the TRX using TRX_Host_storeStream()
6. Store the commands to the TRX using TRX_Host_storeCmds()
7. Waits for a button push
8. Submit the TX command chain to the radio and toggle CONFIG_PIN_GLED every
time a packet is transmitted