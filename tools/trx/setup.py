#
# Copyright (c) 2022-2023, Texas Instruments Incorporated
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
#   Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from setuptools import setup, find_packages
from distutils.util import convert_path
from os import path

this_directory = path.abspath(path.dirname(__file__))
with open(path.join(this_directory, "README.md"), encoding="utf-8") as f:
    long_description = f.read()

main_ns = {}
ver_path = convert_path("trx/version.py")
with open(ver_path) as ver_file:
    exec(ver_file.read(), main_ns)


setup(
    name="trx",
    version=main_ns["__version__"],
    description="Build/Parse TRX frames",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="",
    author="a0132628",
    author_email="a0132628@ti.com",
    license="BSD-3",
    packages=["trx"],
    zip_safe=True,
    install_requires=[
        # 'blessings==1.7',
        # 'colorama==0.4.4',
        # 'tabulate==0.8.9',
        # 'typer[all]',
        # 'click-spinner',
        # 'coloredlogs==15.0.1',
        # 'questionary==1.9.0',
        "pyelftools==0.28",
        "pyserial==3.5",
        "construct==2.10.67",
        "requests",
        "pandas<=2.3.2",
        "openpyxl",
        "crc",
    ],
    entry_points={
        "console_scripts": [],
        "pytest11": [],
    },
)
