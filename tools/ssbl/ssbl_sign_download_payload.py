#!/usr/bin/env python
#
#  Copyright (c) 2023, Texas Instruments Incorporated
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  *  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#  *  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  *  Neither the name of Texas Instruments Incorporated nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
import os
import argparse
import hashlib
import ecdsa
import asn1
import zlib
import struct
from pathlib import Path


class bcolors:
    """helper enum class providing color codes"""
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    OKCYAN = "\033[96m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


def sign_with_private_key(
    cmd_payload_file: argparse.FileType, private_key_file: argparse.FileType) -> None:
    """Signs the download command payload binary

        Use this function if you have a key and would like to sign the payload directly with the key file
        The input cmd_payload_file will be modified in place with the calculated signatures.

    Args:
        cmd_payload_file (argparse.FileType): download command payload file
        private_key_file (argparse.FileType): TI or Customer private key file in the PEM format
    """
    print(f"\n{bcolors.HEADER}Preparing to sign {bcolors.UNDERLINE}{cmd_payload_file.name}{bcolors.ENDC}")
    cmd_payload_file.seek(0)
    payload_bytes = cmd_payload_file.read()

    print(f"\nSigning with '{bcolors.UNDERLINE}{private_key_file.name}{bcolors.ENDC}'")

    # Read private key from filesystem and use to sign + verify
    sk = ecdsa.SigningKey.from_pem(private_key_file.read(), hashlib.sha256)
    signature = sk.sign(payload_bytes)
    vk = sk.verifying_key

    try:
        vk.verify(signature, payload_bytes, hashfunc=hashlib.sha256)
        print(f"{bcolors.OKGREEN}    Signature Verified{bcolors.ENDC}")
    except:
        print(f"\n{bcolors.FAIL}!!!!\nSignature Verify Failed\n!!!!\n{bcolors.ENDC}")
        exit()

    # Append the new signature to the end of the payload file
    cmd_payload_file.seek(len(payload_bytes))
    cmd_payload_file.write(signature)

    print(
        f"{bcolors.OKGREEN}    Done:{bcolors.ENDC} Added Signature: {bcolors.OKCYAN}{signature.hex()}{bcolors.ENDC}\n"
    )

def decode_asn1_signature(existing_sig_file: argparse.FileType) -> bytes:
    """Decode a standard ASN1 signature from the provided signature file

    Args:
        existing_sig_file (argparse.FileType): Binary ASN1 formatted signature

    Returns:
        bytes: Signature to append to the provided input payload
    """
    print(f"\nDecoding asn1 signature from {bcolors.UNDERLINE}'{existing_sig_file.name}'{bcolors.ENDC}")
    decoder = asn1.Decoder()
    decoder.start(existing_sig_file.read())

    # Enter the asn1.Types.Constructed element
    decoder.enter()

    # Read first integer component (r)
    tag, value = decoder.read()
    r = value.to_bytes(length=32, byteorder='big')

    # Read second integer component (s)
    tag, value = decoder.read()
    s = value.to_bytes(length=32, byteorder='big')

    # Leave the asn1.Types.Constructed element
    decoder.leave()

    return bytes(r + s)

def sign_with_existing_signature(
    cmd_payload_file: argparse.FileType, existing_sig_file: argparse.FileType, public_key_file: argparse.FileType) -> None:
    """Signs the download command payload binary

        Use this function if you already have a signature instead of using a private key.
        The input cmd_payload_file will be modified in place with the extracted signature.

    Args:
        cmd_payload_file (argparse.FileType): download command payload file
        existing_sig_file (argparse.FileType): Binary signature in asn1 format
        public_key_file (argparse.FileType): Optional public key file in the PEM format to confirm ECDSA verification
    """
    print(f"\n{bcolors.HEADER}Preparing to insert extracted signature from {bcolors.UNDERLINE}{existing_sig_file.name}{bcolors.ENDC}")

    # Decode Signature
    signature = decode_asn1_signature(existing_sig_file)

    # Optionally verify signature with public key
    if (public_key_file):
        cmd_payload_file.seek(0)
        payload_bytes = cmd_payload_file.read()
        vk = ecdsa.VerifyingKey.from_pem(public_key_file.read())

        try:
            vk.verify(signature, payload_bytes, hashfunc=hashlib.sha256)
            print(f"{bcolors.OKGREEN}    Signature Verified{bcolors.ENDC}")
        except:
            print(f"\n{bcolors.FAIL}!!!!\nSignature Verify Failed\n!!!!\n{bcolors.ENDC}")
            exit()

    # Append the new signature to the end of the payload file
    cmd_payload_file.seek(0, os.SEEK_END)
    cmd_payload_file.write(signature)

    print(
        f"{bcolors.OKGREEN}    Done:{bcolors.ENDC} Added Signature: {bcolors.OKCYAN}{signature.hex()}{bcolors.ENDC}\n"
    )

def print_header_info(trx_fw_file: argparse.FileType):
    """Parses the trx fw file and prints out it's header info to the user

    Args:
        trx_fw_file (argparse.FileType): Binary trx fw file used to read header info from
    """
    # It's possible that the NV region of the TRX FW has been populated. Handle this by detecting the header of the image
    trx_fw_content = trx_fw_file.read()
    STATUS_INACTIVE = 0xFFFFFFFF
    hdr_offset = 0
    if trx_fw_content[hdr_offset:hdr_offset+4] != STATUS_INACTIVE.to_bytes(4, 'big'): # endianess doesn't actually matter here
        hdr_offset = 0x1000
        if trx_fw_content[hdr_offset:hdr_offset+4] != STATUS_INACTIVE.to_bytes(4, 'big'): # endianess doesn't actually matter here
            print(f"\n{bcolors.FAIL}!!!!\Failed to parse image header\n!!!!\n{bcolors.ENDC}")
            exit()

    # Capture header contents
    image_status = int.from_bytes(trx_fw_content[hdr_offset:hdr_offset+4], byteorder="little") # image_status starts at offset 0 of the header
    image_id = int.from_bytes(trx_fw_content[hdr_offset+4:hdr_offset+6], byteorder="little")  # image_id starts at offset 4 of the header
    image_sec_ver = trx_fw_content[hdr_offset+6]  # sec_ver starts at offset 6 of the header
    image_ver = trx_fw_content[hdr_offset+7]  # we don't actually care about this for SSBL purposes. Only sec_ver.
    image_addr = int.from_bytes(trx_fw_content[hdr_offset+8:hdr_offset+12], byteorder="little")  # image_addr starts at offset 8 of the header

    print(f"""
Image Header Details:
\tImage Status = 0x{image_status:X}
\tImage ID = 0x{image_id:X}
\tImage Security Version = {image_sec_ver}
\tImage App Version = {image_ver}
\tImage Start Addr = 0x{image_addr:X}
    """)

def main(raw_args=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)

    # Required binary input
    parser.add_argument(
        "--cmd_payload",
        metavar="CMD_PAYLOAD",
        help="Input download_cmd_payload.bin file",
        type=argparse.FileType("r+b"),
        required=True,
    )

    # Required binary input
    parser.add_argument(
        "--trx_fw",
        metavar="TRX_FW",
        help="Input trx_fw.bin file",
        type=argparse.FileType("r+b"),
        required=True,
    )

    # It doesn't make sense to allow signing with a key / existing_sign / padded_sign at the same time
    signature_method_group = parser.add_mutually_exclusive_group(required=True)
    signature_method_group.add_argument(
        "--private_key",
        metavar="PRIVATE_KEY",
        help="Input private_key.pem file",
        type=argparse.FileType("r"),
    )
    signature_method_group.add_argument(
        "--existing_signature",
        metavar="EXISTING_SIGN",
        help="Pre-calculated signature in a binary asn1 format. Optionally, --public_key can be used to confirm ECDSA verification is successful.",
        type=argparse.FileType("rb"),
    )
    signature_method_group.add_argument(
        "--pad_signature",
        help="Don't sign at all. Simply pad the signature field with 0xFF's. The customer must use this when they do not wish to sign with their own private key by way of either --private_key or --existing_signature.",
        action="store_true",
    )

    # Optionally verify the provided signature with a public key
    parser.add_argument(
        "--public_key",
        metavar="PUBLIC_KEY",
        help="Public key .pem file. Optionally used to verify when the --existing_signature method is selected.",
        type=argparse.FileType("r"),
        required=False
    )

    # Argument only to be used by TI employees. A TI employee will call this script with this option before releasing
    # new TRX FW. By *NOT* providing this option it is therefor assumed that an external customer is running the script.
    # By doing so the script will take two additional steps after signing:
    #   1. Concatenate the freshly signed or padded download_cmd_payload.bin and the trx_fw.bin into a single binary
    #       called trx_fw_update_final.bin
    #   2. Calculate a CRC32 over the new trx_fw_update_final.bin file and append it to the end. This can now be
    #       used by a Host application quickly verify the contents haven't been corrupted during transferring.
    parser.add_argument(
        "--internal",
        help="Specifying this option will skip the steps required when this script is ran by an external customer.",
        action="store_true",
        default=False
    )

    args = parser.parse_args(raw_args)

    print_header_info(args.trx_fw)

    # Sign using the specified method
    action_str = "Signed"
    if args.private_key:
        sign_with_private_key(args.cmd_payload, args.private_key)
    if args.existing_signature:
        sign_with_existing_signature(args.cmd_payload, args.existing_signature, args.public_key)
    if args.pad_signature:
        action_str = "Padded"
        # Instead pad with 0xFF's to tell the SSBL there is no optional customer signature used
        args.cmd_payload.seek(0, os.SEEK_END)
        args.cmd_payload.write(bytes([0xFF] * 64))

    if (args.internal):
        print(
            f"{bcolors.OKGREEN}{bcolors.BOLD}Done:{bcolors.ENDC} {action_str} '{bcolors.UNDERLINE}{args.cmd_payload.name}{bcolors.ENDC}'\n"
        )
    else:
        # Concatenate the two binaries into one
        final_bin_path = Path("trx_fw_update_final.bin")
        final_contents = bytearray()

        args.cmd_payload.seek(0)
        final_contents += args.cmd_payload.read()
        args.trx_fw.seek(0)
        final_contents += args.trx_fw.read()

        crc32 = zlib.crc32(final_contents)
        final_contents.extend(struct.pack(">I", crc32))
        with open(final_bin_path, "w+b") as f:
            f.write(final_contents)

        print(
            f"{bcolors.OKGREEN}{bcolors.BOLD}Done:{bcolors.ENDC} {action_str} '{bcolors.UNDERLINE}{final_bin_path}{bcolors.ENDC}'\n"
        )


if __name__ == "__main__":
    main()