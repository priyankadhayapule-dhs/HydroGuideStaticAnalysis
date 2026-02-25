#!/usr/bin/env python3

"""
Induce a synchronization failure while imaging by overwriting the B-mode frame index.

The script may take some time to induce an error, because the write by the script has to
after the data has been written to SOM memory by the DAU, but before SW has read the frame.
Since, ADB is slow hitting the window involves some luck. 
"""

import argparse
import FPGAPci as pci
import os
import registers_v2 as reg
import som
import time


def auto_int(x):
    return int(x,0)

def arguments():
    "Process program arguments."

    argParserObj = argparse.ArgumentParser(description="Induce syncrhonization error while imaging.")

    argParserObj.add_argument('--debug',action="store_true", help="Turn on debug messages.")

    a = argParserObj.parse_args()
    return(a)

args = arguments()


p = pci.FPGAPci()
if (args.debug):
    p.DEBUG_LEVEL = 0x8000

# This finds the base address of the FIFO buffer in SOM memory.
s = som.som(p)

# Retrieve B-Mode frame info from SE registers
nFAddr = (p.readTOP(reg.SE_ADDR_DT0_FADDR)[0])
nOffset = nFAddr - reg.PCIE_MMAP_ADDR

# Determine the host address.
nHostAddr = nOffset + s.SOM_FIFO_BASE
    
    
print("FAddr   : 0x{:08x}".format(nFAddr))
print("Offset  : 0x{:08x}".format(nOffset))
print("HostAddr: 0x{:08x}".format(nHostAddr))

print()
print("Press Ctrl-C to stop.")

while (True):
    # Clear frame index in buffer 0 of B-Mode data.
    p.run(['adb', 'shell', '/data/local/thortools/dmawrite', 'fifo', '0x{:x}'.format(nHostAddr), '0'])
    
    

