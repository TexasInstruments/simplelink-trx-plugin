# rfDiagnostics

---

Project Setup using the System Configuration Tool (SysConfig)
-------------------------
The purpose of SysConfig is to provide an easy to use interface for configuring drivers, RF stacks, and more. The .syscfg file provided with each example
project has been configured and tested for that project. Changes to the .syscfg file may alter the behavior of the example away from default. Some parameters configured in SysConfig may require the use of specific APIs or additional modifications in the application source code. More information can be found in SysConfig by hovering over a configurable and clicking the question mark (?) next to it's name.

Example Summary
---------------
Some of the most common TRX Host RF Driver commands have been exposed over an AT Command Interface such that it can be exercised by Host SW (running on an PC, MPU or MCU) or by a human using a serial terminal emulator. This example allows the user to exercise both the built-in and external TRX radios simultaneously.

Peripherals Exercised
---------------------
* `CONFIG_UART_0` - Used to access the AT interface from a host serial session
* `TRX_HOST_SPI_CONTROLLER` - SPI interface between Host device and TRX. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `CONFIG_GPTIMER_0` - Timer used in TRX RF Host driver operation. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_HOST_SPI_INT` - IO from TRX to Host indicating SPI message from TRX --> Host. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_HOST_SPI_CS` - SPI Chip Select IO. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer
* `TRX_RESET` - IO used to reset the TRX from the Host. Controlled by the TRX Host RF Driver's hardware abstraction (HAL) layer

Resources & Jumper Settings
---------------------------
> If you're using an IDE (such as CCS or IAR), please refer to Board.html in your project directory for resources used and board-specific jumper settings. Otherwise, you can find Board.html in the directory &lt;SDK_INSTALL_DIR&gt;/source/ti/boards/&lt;BOARD&gt;.

Board Specific Settings
-----------------------
The TRX Host RF Driver uses a HAL layer called the "platform". All IO, SPI, and Timer settings must be adjusted/implemented according to the host device setup being used. By default, this example will have a correct HAL configuration out-of-box for all supported TI launchpads.

Note: If you are using the BP-CC140xP-8-FE as the evaluation board for the CC140x device, the frequency range to use in the examples must be 862-870 MHz.
If you are using the BP-CC140xP-9-FE as the evaluation board for the CC140x device, the frequency range to use in the examples must be 902-928 MHz.
These limitations are due to the presence of a SAW filter which limit the frequency range on RX.

Example Usage
-------------
Open a serial session (e.g. HyperTerminal,puTTY, etc.) to the appropriate COM port. Note: the COM port can be determine via Device Manager in Windows or via ls /dev/tty* in Linux.

The connection should have the following settings:

    Baud-rate:    115200
    Data bits:    8
    Stop bits:    1
    Parity:       None
    Flow Control: None

By default the target echoes back any character that is typed in the serial session.

Note: Two boards are recommended in order to see the effect of these commands


AT API
-------------

The AT Command Interface uses ASCII characters so that a terminal emulator can send the commands, but also uses framing so that SW can format and parse the AT commands.

The frame format is shown below

    |----------------|-----------------------------|-------------|--------------------------|---------------------------|---------------|--------------|
    | Start of Frame | Command Type                | Command ID  |         Radio ID         | Get/Set Parameter         | Parameters    | End Of Frame |
    |                |                             |             |        (Optional)        | (parameter command only)  |               |              |
    |================|=============================|=============|==========================|===========================|===============|==============|
    | 'AT' / 'at'    | '+'       (Control Command) |             | '1' (built-in) - Default |  '=' (set)                |               |  '\n' or     |
    |                | 'P' / 'p' (Parameter)       |             | '2' (TRX)                |  '?' (get)                |               |  '\r'        |
    |----------------|-----------------------------|-------------|--------------------------|---------------------------|---------------|--------------|

The AT Command Interface uses 2 command types:

1. +x: Control Commands
1. Pxx: Parameters

## AT Control Commands

    |------------|------------------------------------|---------------|--------------------------------------------------------|
    | Command ID | Description                        | Parameter(s)  | Example                                                |
    |============|====================================|===============|========================================================|
    |    I/i     | Initialize the Radio               | PHY ID (int)  | Initialize PHY0 on built-in radio: AT+I 0 OR AT+I1 0   |
    |            |                                    | (Optional)    | Initialize PHY0 on TRX radio:                AT+I2 0   |
    |            |                                    | PHY ID2 (int) | Initialize PHY0 on TRX radio:                AT+I2 0 2 |
    |------------|------------------------------------|---------------|--------------------------------------------------------|
    |    RS/rs   | System reset                       | None          | AT+RS                                                  |
    |------------|------------------------------------|---------------|--------------------------------------------------------|
    |    EM/em   | Enable WiSUN MDR                   | 0 - Disable   | Disable WiSUN MDR on TRX radio:    AT+EM2 0            |
    |            |                                    | R - Region    | Enable WiSUN MDR on TRX radio:  AT+EM2 <Region>        |
    |------------|------------------------------------|---------------|--------------------------------------------------------|

## AT Parameters

Parameters offer set (=) and get (?) functionality and are specific to either the built-in or TRX radio. The radio ID is optional
and defaults to the built-in radio when not provided. The general format for AT parameter commands is ATP&lt;PARAM&lt;&lt;RADIO_ID&lt;.

The supported parameters are:

    |--------------------------------------------------------------------------------|----------------------------|----------------------------------|
    | Command ID | R/W |         Radio ID         | Description                      | Parameter(s)               | Example                          |
    |            |     |        (Optional)        |                                  |                            |                                  |
    |============|=====|==========================|==================================|============================|==================================|
    |     FR     | R/W | '1' (built-in) - Default | Read/Write frequency in Hz       | Frequency (4 bytes)        | - Built-in radio: ATPFR?         |
    |     fr     |     | '2' (TRX)                | (Optional-MDR) Read/Write        |                            | - TRX radio:      ATPFR2?        |
    |            |     |                          | Channel based on phy             |                            | - Built-in radio: ATPFR=<Freq>   |
    |            |     |                          |                                  |                            | - external radio: ATPFR2=<Freq>  |
    |            |     |                          |                                  |                            | -          ATPFR2=<Freq> <Freq2> |
    |            |     |                          |                                  |                            | -          ATPFR2=<Chan> <Chan2> |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     PW     | R/W | '1' (built-in) - Default | Read/Write tx power in dBm       | Power in decimal between   | - Built-in radio: ATPPW?         |
    |            |     | '2' (TRX)                |                                  | -10 to 14 dBm (20 dBm on   | - TRX radio:      ATPPW2?        |
    |     pw     |     |                          |                                  | 1352P). Note ccfg changes  | - Built-in radio: ATPPW=<pwr>    |
    |            |     |                          |                                  | are required for 14 dBm    | - TRX radio:      ATPPW2=<pwr>   |
    |            |     |                          |                                  | output power; this applies |                                  |
    |            |     |                          |                                  | to CC13x2P running the     |                                  |
    |            |     |                          |                                  | default PA                 |                                  |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     CR     |  R  | '1' (built-in) - Default | Read Current RSSI when the radio | None                       | - Built-in radio: ATPCR?         |
    |     cr     |     | '2' (TRX)                | is in cont. RX (Test Mode 3)     |                            | - TRX radio:      ATPCR2?        |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     TM     | R/W | '1' (built-in) - Default | Read/Write test mode             | Test mode                  | - Built-in radio: ATPTM?         |
    |     tm     |     | '2' (TRX)                |                                  | 0: Exit                    | - TRX radio:      ATPTM2?        |
    |            |     |                          |                                  | 1: Carrier Wave            | - Built-in radio: ATPTM=<mode>   |
    |            |     |                          |                                  | 2: Modulated               | - TRX radio:      ATPTM2=<mode>  |
    |            |     |                          |                                  | 3: cont. RX                |                                  |
    |            |     |                          |                                  | 4: PER TX                  |                                  |
    |            |     |                          |                                  | 5: PER RX                  |                                  |
    |            |     |                          |                                  | 6: WiSUN MDR TX            |                                  |
    |            |     |                          |                                  | 7: WiSUN MDR RX            |                                  |
    |            |     |                          |                                  | 8: WiSUN MDR CS+TX         |                                  |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     PP     | R/W | '1' (built-in) - Default | Read/Write Number of PER Tx/Rx   | Number of Tx/Rx Packets    | - Built-in radio: ATPPP?         |
    |     pp     |     | '2' (TRX)                | Packets                          | (0 - 65535)                | - TRX radio:      ATPPP2?        |
    |            |     |                          |                                  |                            | - Built-in radio: ATPPP=<int>    |
    |            |     |                          |                                  |                            | - TRX radio:      ATPPP2=<int>   |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     PL     | R/W | '1' (built-in) - Default | Read/Write PER Tx/Rx Packet      | Tx/Rx Payload Length       | - Built-in radio: ATPPL?         |
    |     pl     |     | '2' (TRX)                | Length                           | (2 - 255)*                 | - TRX radio:      ATPPL2?        |
    |            |     |                          |                                  |                            | - Built-in radio: ATPPL=<int>    |
    |            |     |                          |                                  |                            | - TRX radio:      ATPPL2=<int>   |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     PN     | R   | '1' (built-in) - Default | Read the PHY names for the       | None                       | - Built-in radio: ATPPN?         |
    |     pn     |     | '2' (TRX)                | available PHYs                   |                            | - TRX radio:      ATPPN2?        |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     AR     | R   | '1' (built-in) - Default | Read average RSSI during PER RX  | None                       | - Built-in radio: ATPAR?         |
    |     ar     |     | '2' (TRX)                |                                  |                            | - TRX radio:      ATPAR2?        |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     MR     | R   | '1' (built-in) - Default | Read maximum RSSI during PER RX  | None                       | - Built-in radio: ATPMR?         |
    |     mr     |     | '2' (TRX)                |                                  |                            | - TRX radio:      ATPMR2?        |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     LR     | R   | '1' (built-in) - Default | Read lowest/minimum RSSI during  | None                       | - Built-in radio: ATPLR?         |
    |     lr     |     | '2' (TRX)                | PER RX                           |                            | - TRX radio:      ATPLR2?        |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     AE     | R/W |                          | Read/Write AT command echo       | 0: Disable AT command echo | - Read echo param: ATPAE?        |
    |     ae     |     |                          | ( echo is enabled by default)    | 1: Enable AT command echo  | - Write echo param: ATPAE=<int>  |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|
    |     RV     | R   | '1' (built-in) - Default | Read radio version               | None                       | - Built-in radio: Not supported  |
    |     rv     |     | '2' (TRX)                |                                  |                            | - TRX radio:      ATPRV2?        |
    |------------|-----|--------------------------|----------------------------------|----------------------------|----------------------------------|

    * For BLE packets, the length set is the PDU Payload Length (12 - 37 bytes). If the length is set outside this, a default
      PDU Payload Length of 37 will be used

It is important to note that most of the parameters (except PP and PL)  requires that the initialization command has been run ( "AT+I x").

The format of a parameter read command to get the Tx Power parameter is:

        Built-in radio: ATPPW?<CR> OR ATPPW1?<CR>
        TRX radio:      ATPPW2?<CR>

The response to a read will be of the format shown below, depending on the parameter it will be hex or decimal:

        -10<CR>

The format of an AT command to set the Frequency Parameter to 868 MHz is:

        Built-in radio: ATPFR=868000000<CR> OR ATPFR1=868000000<CR>
        TRX radio:      ATPFR2=868000000<CR>

The response of a write will be of the format:

        OK<CR>

## Command Responses

    |-----------------------------------------------------------------|
    | Response                        | Description                   |
    |=================================|===============================|
    | OK<CR>                          | Command executed successfully |
    |---------------------------------|-------------------------------|
    | AtProcess_Status_CmdIdError<CR> | Command ID not valid          |
    |---------------------------------|-------------------------------|
    | AtProcess_Status_ParamError<CR> | Command failed due to         |
    |                                 | a parameter error             |
    |---------------------------------|-------------------------------|
    | AtProcess_Status_Error<CR>      | Command cound not be executed |
    |-----------------------------------------------------------------|

### Test Modes

The AT interface supports the following test modes:

- Carrier Wave
- Modulated Signal
- Cont. RX
- PER Tx
- PER Rx

#### Carrier Wave:

This is enabled by setting test mode to 1. To configure the carrier wave test mode use:

        Built-in radio: ATPTM=1<CR> OR ATPTM1=1<CR>
        TRX radio:      ATPTM2=1<CR>

To exit the carrier wave test mode:

        Built-in radio: ATPTM=0<CR> OR ATPTM1=0<CR>
        TRX radio:      ATPTM2=0<CR>
#### Modulated Signal:

This is enabled by setting test mode to 2. To configure the Modulated Signal test mode use:

        Built-in radio: ATPTM=2<CR> OR ATPTM1=2<CR>
        TRX radio:      ATPTM2=2<CR>

To exit the Modulated Signal test mode:

       Built-in radio: ATPTM=0<CR> OR ATPTM1=0<CR>
       TRX radio:      ATPTM2=0<CR>

#### Continuous RX:

This is enabled by setting test mode to 3. To configure the cont. RX test mode use:

        Built-in radio: ATPTM=3<CR> OR ATPTM1=3<CR>
        TRX radio:      ATPTM2=3<CR>

The user may request the RSSI data from the radio by polling the CR (Current RSSI), while the radio is running in receive mode, by

        Built-in radio: ATPCR?<CR> OR ATPCR1?<CR>
        TRX radio:      ATPCR2?<CR>

A value of -128 indicates an error in the RSSI value.

To exit this test mode:

        Built-in radio: ATPTM=0<CR> OR ATPTM2=0<CR>
        TRX radio:      ATPTM2=0<CR>

#### PER TX:

This is enabled by setting test mode to 4, it will run for the number of packets configure in the `PP` Parameter. To configure PER Tx test mode to run for 100 packets:

        Built-in radio: ATPPP=100<CR> OR ATPPP1=100<CR>
                        ATPTM=4<CR>   OR ATPTM1=4<CR>
        TRX radio:      ATPPP2=100<CR>
                        ATPTM2=4<CR>

While the test is running the below responses should be expected:

        Packet: 0
        Packet: 1
        Packet: 2
        ...
        ...
        Packet: 99

Once the test completes the below response should be expected.

        OK<CR>

Other related parameters to this test is:

- PPL: PER Tx Packet Length

#### PER Rx:

This is enabled by setting test mode to 5. It will run until test mode is set back to 0 (exit). To start PER RX:

        Built-in radio: ATPTM=5<CR> OR ATPTM1=5<CR>
        TRX radio:      ATPTM2=5<CR>

While the test is running the below responses should be expected:

       RX in Progress....
       Exit by setting Test Mode = 0 (ATPTM=0)

       Note: Packet Length filtering is utilized, so the packet length
       on the receiver must be larger or equal to the packet length on the transmitter
       OK

Once the test completes (ATPTM=0) the below response should be expected.

        Packets Received:    x
        Average RSSI:        -y dBm
        Max RSSI:            -n dBm
        Min RSSI:            -m dBm
        OK


Where x is the number of packets received with CRC OK, and y is the average RSSI of all of the received packets (CRC OK)

#### WiSUN MDR TX:

NOTE: MDR feature is available only on CC1407 tranceiver

This is enabled by setting test mode to 6, it will run for the number of packets configure in the `PP` Parameter. There are prequisites required to run this mode.

The device must be MDR Enabled:

        TRX radio:      AT+EM2 2<CR>

Initialization must have 2 phys set from the MDR Table. To get the updated MDR Phy list:

        TRX radio:      ATPPN2?<CR>

Then initialize with 2 phys, with 1st phy being set to whatever the desired base phy is to be:

        TRX radio:      AT+I2 0 2<CR>

If 2 phy frequencies are requested, run the ATPFR command with the desired frequencies. Channels can also be set using this API. Here are some examples:

        TRX radio(frequency):      ATPFR2=922500000 922700000

        TRX radio(channel):      ATPFR2=4 2

If the frequency for the 2nd phy is desired to be the default next available frequency, the initial channel or frequency can be set and the default next frequency will be set:

        TRX radio(frequency):      ATPFR2=922500000

        TRX radio(channel):      ATPFR2=4

        TRX radio:      ATPFR2?

To configure MDR Tx test mode to run for 100 packets:

        TRX radio:      ATPPP2=100<CR>
                        ATPTM2=6<CR>

While the test is running the below responses should be expected:

        Packet: 0
        Packet: 1
        Packet: 2
        ...
        ...
        Packet: 99

Once the test completes the below response should be expected.

        OK<CR>

Other related parameters to this test is:

- PPL: MDR Tx Packet Length

#### WiSUN MDR Rx:

NOTE: MDR feature is available only on CC1407 tranceiver

This is enabled by setting test mode to 7. It will run until test mode is set back to 0 (exit). There are prequisites required to run this mode.

The device must be MDR Enabled:

        TRX radio:      AT+EM2 2<CR>

Initialization must at least 1 phy set from the MDR Table. To get the updated MDR Phy list:

        TRX radio:      ATPPN2?<CR>

Then initialize with either 1 or 2 phys, with 1st phy being set to whatever the desired base phy is to be:

        TRX radio:      AT+I2 0<CR>
                        AT+I2 0 2<CR>

Then set the frequency or channel by running the ATPFR command. Here are examples of setting with frequency and channel respectively:

        TRX radio(freq or chan):      ATPFR2=922500000
                                      ATPFR2=4

To start MDR RX:

        TRX radio:      ATPTM2=7<CR>

While the test is running the below responses should be expected:

       MDR RX in Progress....
       Exit by setting Test Mode = 0 (ATPTM=0)

       Note: Packet Length filtering is utilized, so the packet length
       on the receiver must be larger or equal to the packet length on the transmitter
       OK

Once the test completes (ATPTM2=0) the below response should be expected.

        Packets Received:    x
        Average RSSI:        -y dBm
        Max RSSI:            -n dBm
        Min RSSI:            -m dBm
        PHY RX'd: <PHY NAME & INFO>
        OK


Where x is the number of packets received with CRC OK, and y is the average RSSI of all of the received packets (CRC OK)

#### WiSUN MDR CS+TX:

NOTE: MDR feature is available only on CC1407 tranceiver

This is enabled by setting test mode to 8, it will run for the number of packets configure in the `PP` Parameter. There are prequisites required to run this mode.

To change from the project default CS Settings, look in the PhySettings_trx_mdr.c file and look in the PhySettings_Trx_supportedMdrRegions array.
Both FSK and OFDM have configurable phy settings in this file that can be configured at build time.

The device must be MDR Enabled:

        TRX radio:      AT+EM2 2<CR>

Initialization must have 2 phys set from the MDR Table. To get the updated MDR Phy list:

        TRX radio:      ATPPN2?<CR>

Then initialize with 2 phys, with 1st phy being set to whatever the desired base phy is to be:

        TRX radio:      AT+I2 0 2<CR>

If 2 phy frequencies are requested, run the ATPFR command with the desired frequencies. Channels can also be set using this API. Here are some examples:

        TRX radio(frequency):      ATPFR2=922500000 922700000

        TRX radio(channel):      ATPFR2=4 2

To configure MDR Cs+Tx test mode to run for 100 packets:

        TRX radio:      ATPPP2=100<CR>
                        ATPTM2=8<CR>

While the test is running the below responses should be expected:

        Packet: 0
        Packet: 1
        Packet: 2
        ...
        ...
        Packet: 99

Once the test completes the below response should be expected.

        OK<CR>

Other related parameters to this test is:

- PPL: MDR Tx Packet Length

## How to Add New PHY Parameter

A PHY in this example is one entry in the `PhySettings_Trx_supportedPhys` array. Each entry points at an "RF config"
blob - the radio register settings - that lives in a `rcl_settings_<name>.c`/`.h` pair. There are two cases:

- Case A - the RF config already exists in the example (reuse an existing `PTR_*_RF_CONFIG`):skip step 1.
- Case B - the PHY needs a new RF config (a new `rcl_settings_<name>` file): do all steps.

The steps below use `<name>` for the RF config (e.g. `myphy`) and `<NAME>` for its uppercase form.

#### 1. Add the new rcl_settings file (Case B only)

Generate the `rcl_settings_<name>.c` and `rcl_settings_<name>.h` using SmartRF Studio 8 and import the files into the CCS project and place 
both into `source/ti/trx/rfconfig/BP_EM_CC1407P_SE/`. The header declares `extern const uint32_t
LRF_mainRegConfig_<name>[];` and any `TRX_PHY_FEATURE_*_<NAME>` option-mask macros for the PHY.

#### 2. In PhySettings_trx.c

1. `#include "source/ti/trx/rfconfig/BP_EM_CC1407P_SE/rcl_settings_<name>.h"` (Case B only).
2. Define `#define PTR_<NAME>_RF_CONFIG ((uint8_t *)LRF_mainRegConfig_<name>)` (Case B only).
3. Add a `PhySettings_Trx_RfSetting` struct to the `PhySettings_Trx_supportedPhys` array - uncomment the EXAMPLE
   STRUCT at the end of the array or copy an existing entry. Set `.rfConfig` to the `PTR_*_RF_CONFIG` macro,
   `.modem` to the PHY's modem, `.optionMask` to the `TRX_PHY_FEATURE_*` bits from the rcl_settings header (or `0`),
   the `.header` union fields for that modem, and a descriptive `.PhySettings_phyName`.
4. Set `.PhySettings_phyIndex` to a unique index that matches the entry's position in the array.

#### 3. In PhySettings_trx.h

1. Add the new PHY index to the `PhySettings_Trx_phyIndex` enum, before `Num_Phy_Settings` (which must stay last -
   it is the PHY count).
2. If the PHY belongs to a special group (Gen FSK, XLR, OFDM-LR), extend the matching
   `PHY_SETTINGS_*_PHY_INDEX_START/END` range so it stays contiguous and covers the new index.

#### 4. Special RF mode or payload header (only if needed)

A SUN FSK/OFDM/OQPSK PHY that reuses `TRX_RfMode_SUN` needs no further change. If the PHY needs a different RF
mode (e.g. Prop FSK, XLR, narrow-band peak AGC) or a non-SUN over-the-air header, update `radio_trx.c`:

- `Radio_Trx_setupPhy()` selects the RF mode from the PHY index / range macros.
- `composePayload()` selects the payload-header format from the modem type and range macros. An unhandled modem
  returns `-1`.

Callbacks (`txCallback`, `rxCallback`, `rxTestCallback`, `mdr*Callback`) are PHY-agnostic - they respond to TRX
events, not to the PHY index - so adding a PHY does not require a new callback. Only modify a callback if a new modem
type changes how received payloads or headers must be parsed.

NOTE: OFDM PHYs are only available on the CC1407 transceiver.

#### Adding a PHY to MDR

To enable a PHY for MDR, follow the same steps but make the changes in `PhySettings_trx_mdr.c` and
`PhySettings_trx_mdr.h`. Refer to the existing structs for examples.
