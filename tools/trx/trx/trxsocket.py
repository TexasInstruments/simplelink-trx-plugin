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

import socket
import select

from trx.transaction import TxTransaction, RxTransaction
from trx.trxrequests import (
    nice,
    PingRequest,
    LastStatus,
    CommandStore,
    StreamStore,
    StreamFlush,
    TxParams,
    CommandId,
    CommandSubmit,
)
from construct import SelectError


class TRXSocket:
    def __init__(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect(("127.0.0.1", 5001))
        self._inbuf = b""

    def send(self, transaction):
        return self.socket.send(transaction.build())

    def receive(self, timeout: int = 0.2):
        recvd = []
        while True:
            ready, *_ = select.select([self.socket], [], [], timeout)
            if not ready:
                break
            self._inbuf += self.socket.recv(1024)

            # print(':'.join(['%02X'%x for x in self._inbuf]))
            while len(self._inbuf):
                try:
                    t = RxTransaction.parse(self._inbuf)
                    # print(t)
                    recvd.append(t)
                    print(":".join(["%02X" % x for x in self._inbuf[: t.len + 4]]))
                    self._inbuf = self._inbuf[t.len + 4 :]
                except SelectError:
                    break
        return recvd


if __name__ == "__main__":
    trx = TRXSocket()

    t = TxTransaction(requests=[PingRequest(0xAE)])
    print("Sending..")
    print(t)
    print(nice(t.build()))
    trx.send(t)

    print("Receiving..")
    rsp = trx.receive()
    for r in rsp:
        print(r)

    print("Sending..")
    t = TxTransaction(
        requests=[CommandStore(cmd_id=CommandId.Transmit, retention_policy=3, params=TxParams(stream_id=63))]
    )
    print(t)
    print(nice(t.build()))
    trx.send(t)

    print("Receiving..")
    rsp = trx.receive()
    for r in rsp:
        print(r)

    print("Sending stream-store 1..")
    t = TxTransaction(
        requests=[StreamStore(stream_id=63, start=1, md=1, retention=0, data=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10])]
    )
    print(t)
    print(nice(t.build()))
    trx.send(t)

    print("Receiving..")
    rsp = trx.receive()
    for r in rsp:
        print(r)

    print("Sending stream-store 2..")
    t = TxTransaction(requests=[StreamStore(stream_id=63, start=0, md=0, retention=0, data=[11, 12, 13, 14, 15, 16])])
    print(t)
    print(nice(t.build()))
    trx.send(t)

    print("Receiving..")
    rsp = trx.receive()
    for r in rsp:
        print(r)

    print("Sending..")
    t = TxTransaction(requests=[CommandSubmit(slot=0)])
    print(t)
    print(nice(t.build()))
    trx.send(t)

    print("Receiving..")
    rsp = trx.receive()
    for r in rsp:
        print(r)

    print("Sending stream-store 2..")
    t = TxTransaction(requests=[StreamFlush(stream_id=63)])
    print(t)
    print(nice(t.build()))
    trx.send(t)

    print("Receiving..")
    rsp = trx.receive()
    for r in rsp:
        print(r)
