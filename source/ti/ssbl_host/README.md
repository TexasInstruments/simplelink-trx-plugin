# Transceiver Updates via Host+SSBL

## Background

Although TI owns the development of the software running on the transceiver (TRX), you as the customer still play a
critical role in the device security story.

Your TRX will come preloaded with trusted software. Updates from TI which can provide additional security or additional
features are supported. To facilitate this upgrade process the TRX also comes equipped with the SSBL (Secure Serial
Bootloader).

The SSBL is a minimal interface which provides a secure method of upgrading your TRX.

* 4-pin SPI Interface
* single or double ecdsa public key authentication (see [Security](#security))
* Up to 40 security updates
* Runs from protected persistent flash

To communicate with the SSBL, it is suggested to start with the ssbl_host example application provided by TI. This will
demonstrate the functionality and provide a starting place for adding the Host to your application.

## So you've received a new update from TI.. Now what?

You will have received a few files from TI to facilitate the update of your devices in a secure and transparent way.
First and foremost, you will receive `trx_fw_upgrade.bin`. TI is responsible for protecting what can be run on your TRX
device. This is done with the second file , `download_cmd_payload.bin`. Finally there is a python tool provided so you
can easily use your own companies private key to add an additional layer of ecdsa public/private key authentication.

### Security

The security of this device is critical to its functionality. We at TI want you to be as confident in your device as we
are. In order to do that we employ at least one ecdsa signature. The first signature covers the
`download_cmd_payload.bin` contents and is itself appended to the `download_cmd_payload.bin` file. Your (customer)
signature will cover the existing contents of the `download_cmd_payload.bin` (**including the TI signature**). If you
choose to do this, only software that has been sanctioned by both TI **and** your company will be able to be programmed.

Whether you choose to sign the `download_cmd_payload.bin` with your own private key or not, it's necessary to explicitly
mark your intent through the provided `ssbl_sign_download_cmd_payload.py` script. See the Customer Private Key Signing
section below to learn more.

**Important: If this step is skipped the download will fail!**

Of course, if you are going to sign the new software with your own customer private key, you will have to give the SSBL
access to the customer public key as well. This can be done by using the APIs provided in the Host example. If you
choose to add your own signatures you should program the public key on the device before it goes out into the field. It
can be programmed later through the host API's, but we'll all sleep better at night knowing the public key has been
provisioned already. **Once the customer public key has been provisioned, all future updates must be signed by the
corresponding customer private key.**

#### Customer Private Key Signing

To make customer signing easy and transparent, a simple python script has been provided `ssbl_sign_download_payload.py`.
Feel free to pass the '-h' option to see usage details. This is actually the exact tool that TI will use before
delivering the update to you. Review/Audit the script if you'd like to learn more. You are required to choose one of the
following three signing options.

1. Use your private key in the .pem file format directly and the script will sign the `download_cmd_payload.bin` in the
correct format. This is less secure because the private key is presumably not in a secure location and is **not** the
recommended choice.
    * This option will automatically perform an ECDSA verify test over the calculated signature and display the results.
1. Use your own method of signing the `download_cmd_payload.bin`. Then pass the resulting signature and the script will
format the contents correctly. You are not strictly required to use this tool, but the tool will guarantee that the
signature is appended to the `download_cmd_payload.bin` correctly.
    * When providing your own signature, you can optionally provide the public key as well so that the tool can perform
    an ECDSA verify test.
1. Do not sign with a customer private key. Instead, pad the `download_cmd_payload.bin` to the correct size and forgo
the optional customer signature all together. Don't worry, the TI signature guarantees that only TI sanctioned firmware
can be run on your TRX device.

### Host -> SSBL communication

As briefly described above, the SSBL communicates over a 4-pin SPI interface. Your product shall have a host mcu
connected to the TRX device requiring a minimum of the following connections. Code documentation has been generated over
the provided Host example. Please review this documentation to understand how to use the Host APIs. See documentation in
the `docs/common/trx` folder. Pin configuration can be done through SysConfig.

* CLK
* CSN
* POCI
* PICO
* RESET_N
* TRIGGER_PIN (Optional)
    * The ssbl_host example in the SDK re-uses the CSN pin to reduce the number of physical connections required

#### Activation

During typical operation your TRX device will only execute the current radio software. When the time comes to perform an
update though, you will need to tell the TRX to begin executing the SSBL instead. You can 'activate/trigger' the SSBL in
two ways.

1. There is a RF API provided to you that will cause the TRX device to activate the SSBL
    * This method will cause the SSBL to be **persistently active through reset**. The only way to put the device back
    into radio software is to use the `BreakOutCmd()` host API or to do a power cycle. More on the `BreakOutCmd()` in
    the Host API Documentation.
1. On startup, the TRX device will quickly read the logic level of the TRIGGER_PIN. If the TRIGGER_PIN is determined to
be active low, then the SSBL will be activated instead of the radio software
    * This method **is not** persistent through reset.

Once the SSBL has been activated, your host mcu can communicate with the SSBL through a series of commands.

#### SSBL Commands

The following are the set of commands supported between the host mcu and the SSBL.

* PING
    * Quickly verify the SSBL has been activated and communication is working
* GET_STATUS
    * Get the status of the previous command.
    * ***Note that the ACK from another command does not indicate the function of the command was performed successfully.
  This command is to be used after every other command to determine the status of said previous command.***
* GET_SEC_VER
    * Query the device for information regarding:
        * Current security version of the TRX
        * Status of the current security version (Active, Inactive, Out of Date)
        * How many more security updates are possible
* BREAK_OUT
    * If the SSBL has been activated via the RF API, this command is the only way to return the TRX device to running the
    radio software besides a power cycle.
    * If there is no authenticatable software image in flash then the command will fail and execution will remain the in
    SSBL until authenticatable software can be written to the TRX device
* RESET
    * Causes a reset of the device
* GET_PART_ID
    * Query the Part ID of the device
* GET_KEY_ID
    * Query the Customer Key ID. The Key ID will be programmed when you use the `SET_CUST_KEY` command. This command can
    be used to verify your public key was programmed successfully or to identify which key in your key ring was used for
    this device.
* SET_CUST_KEY
    * Program you own customer public key onto the TRX device.
    * This command will only successfully complete a single time. ***Be very sure that you have the correct key contents
    before calling this command because you will not have a second chance.***
    * The public key will be written to persistent protected flash and thus cannot be modified or erased
* DOWNLOAD
    * Start the download process
    * The `download_cmd_payload.bin` file which was provided shall be sent during this command as the entire payload
* BANK_ERASE
    * Erase all un-protected flash pages to continue the download process
* SEND_DATA
    * Send a data chunk of the provided `trx_fw_upgrade.bin` block by block.

##### Order of Operations

There is an enforced order of operation between the last three commands. All other commands can be performed in any
order at any time. The enforced order is DOWNLOAD -> BANK_ERASE -> SEND_DATA[N]. This ensures a device is not erased
prematurely or accidentally. You can only BANK_ERASE the device after a DOWNLOAD has been authenticated. Next the
BANK_ERASE cannot be forgotten or skipped which would cause the software update to fail. Lastly, you cannot begin a new
DOWNLOAD until either the update has gone through the required N x SEND_DATA commands or the device is reset. At which
point you may begin the DOWNLOAD process again if needed.

### Steps for Upgrade

1. Receive update package from TI
1. Run the provided `ssbl_sign_download_payload.py` with provided cmd_payload of `download_cmd_payload.bin`, the
provided trx_fw of `trx_fw_update.bin` and selecting one of the following options:

    a. `--private_key` - If this option is used, you must be sure that the TRX has the public key programmed to it.

    b. `--existing_signature` - If this option is used, you must be sure that the TRX has the public key programmed to
    it.

    c. `--pad_signature` - Use this option if you do not wish to sign the image. Instead, pad it to the correct length
    and move on.

1. Transfer both the resulting `trx_fw_update_final.bin` to your Host mcu using whatever method is easiest

    a. The TI Host example has a predefined location of 0x10000 for this binary. You will need to ensure your predefined
    location matches where the binary will be stored on the host mcu.
1. On the host mcu, activate the SSBL using the RF API if you wish.
1. Populate the `ssblDownload_t` struct with the correct details for your situation. See documentation in the
`docs/common/trx` folder for more details.
1. Call the `performSsblDownloadSequence()` API.
1. Wait for and interpret the return result from the `performSsblDownloadSequence()` API
1. Return to typical operation of the device.
