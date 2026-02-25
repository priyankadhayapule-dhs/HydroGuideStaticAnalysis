#!/usr/bin/env python3

"""
Utilities for accessing SOM functions.
"""

from array import array
import glob
import os
import subprocess as proc
import registers_v2 as reg
import FPGAPci as pci
import time

# Tablet
SOM_DAUPCI_PATH = '/sys/bus/pci/drivers/dau_pci/0000:01:00.0/'

def set_som_data_path():
    """
    Test if EVM dir exists and if so use it for the data path.
    If not then use the tablet directory.
    """

    if None:
        EVM_DATA_PATH = '/data/data/com.echonous.liveimagedemo/'
        TABLET_DATA_PATH = '/data/data/com.echonous.kosmosapp/'
    
        statusObj = proc.run( ['adb', 'shell', 'if [ -d {} ]; then echo {}; else echo {}; fi'.format(EVM_DATA_PATH, EVM_DATA_PATH, TABLET_DATA_PATH)] , stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
        if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
            print("ERROR::set_som_data_path: Problem with adb command.")
            print(statusObj.stdout)
            return(TABLET_DATA_PATH)
        else:
            return(statusObj.stdout)
    else:
        SOM_DATA_PATH = '/data/echonous/'
        statusObj = proc.run( ['adb', 'shell', 'mkdir', SOM_DATA_PATH] , stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
        if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
            print("ERROR: Setting creating SOM data path dir on EVM/Tablet.")
            print(statusObj.stdout)

        return(SOM_DATA_PATH)

    
SOM_DATA_PATH = set_som_data_path()
print("SOM Data Path: ", SOM_DATA_PATH)


class som:

    def __init__(self, mob=None):
        "Constructor. Pass in an initialized FPGAPci object."

        super().__init__()

        if (mob is None):
            self.mob = pci.FPGAPci()
            self.mob.open()
        else:
            self.mob = mob

        self.dma_info_parse()

 
    def dma_write(self, sBuffer, nAddr, sFile):
        "Write a file to SOM memory."

        #self.mob.DEBUG_LEVEL = 0x8000


    def dma_read(self, sSOMBuf, nAddr, nWords, sFile=None):
        """
        Read data from SOM memory. 

        If sFile is not provided then data is returned as a list.

        sSOMBuf is one of fifo | seq | diag
        nAddr is the address within one of those buffers.
        """

        if (sFile is None):
            statusObj = self.mob.run(['adb', 'shell', '/data/local/thortools/dmaread', sSOMBuf, "0x{0:016x}".format(nAddr), str(nWords)])
    
            lsData = statusObj.stdout.split()
            lnData = []
            if len(lsData) < 9:
                for e in lsData[1:]:
                    lnData.append(int(e,16))
            else:
                for el in range(int(len(lsData)/9)+1):
                    for e in lsData[el*9+1:el*9+9]:
                        lnData.append(int(e,16))
    
            print("Read - Address: 0x{0:08x} Words: {1}".format(nAddr, nWords))
            print(lnData)
    
            return(lnData)
        else: 
            statusObj = self.mob.run(['adb', 'shell', '/data/local/thortools/dmaread', sSOMBuf, "0x{0:016x}".format(nAddr), str(nWords), '-f', sFile])
            # TODO convert to data or just use adb_dmaread below.
            return(-1)


    def dma_info_parse(self):
        """
        Parse DMA Info command output to get SOM buffer base addresses.

        Fifo Buffer:
                Address: 0x0000000082A3C000
                Length (words):  2048000 (0x001F4000)

        Sequence Buffer:
                Address: 0x000000008320C000
                Length (words):  256000 (0x0003E800)

        Diagnostics Buffer:
                Address: 0x0000000083306000
                Length (words):  256000 (0x0003E800)
        """

        statusObj = self.mob.run(['adb', 'shell', '/data/local/thortools/dmainfo'])

        lsLines = statusObj.stdout.split('\n')
        nLine   = 0
        nDone   = 0
        while (nDone < 3):
            sLine   = lsLines[nLine]
            #print(sLine)

            if ("Fifo Buffer" in sLine):
                nLine = nLine + 1
                sLine = lsLines[nLine]

                if ("Address" in sLine):
                    self.SOM_FIFO_BASE = self.parse_addr(sLine)
                    print("SOM_FIFO_BASE  : {0:016x}".format(self.SOM_FIFO_BASE))

                nLine = nLine + 1
                sLine = lsLines[nLine]
                if ("Length" in sLine):
                    self.SOM_FIFO_LENGTH = self.parse_length(sLine)
                    print("SOM_FIFO_LENGTH: {0:016x}".format(self.SOM_FIFO_LENGTH))
                    nDone = nDone +1

            if ("Sequence Buffer" in sLine):
                nLine = nLine + 1
                sLine = lsLines[nLine]

                if ("Address" in sLine):
                    self.SOM_SEQR_BASE = self.parse_addr(sLine)
                    print("SOM_SEQR_BASE  : {0:016x}".format(self.SOM_SEQR_BASE))

                nLine = nLine + 1
                sLine = lsLines[nLine]
                if ("Length" in sLine):
                    self.SOM_SEQR_LENGTH = self.parse_length(sLine)
                    print("SOM_SEQR_LENGTH: {0:016x}".format(self.SOM_SEQR_LENGTH))
                    nDone = nDone +1

            if ("Diagnostics" in sLine):
                nLine = nLine + 1
                sLine = lsLines[nLine]

                if ("Address" in sLine):
                    self.SOM_DIAG_BASE = self.parse_addr(sLine)
                    print("SOM_DIAG_BASE  : {0:016x}".format(self.SOM_DIAG_BASE))

                nLine = nLine + 1
                sLine = lsLines[nLine]
                if ("Length" in sLine):
                    self.SOM_DIAG_LENGTH = self.parse_length(sLine)
                    print("SOM_DIAG_LENGTH: {0:016x}".format(self.SOM_DIAG_LENGTH))
                    nDone = nDone +1

            nLine   = nLine + 1

        self.dictB = {}
        self.dictB['fifo'] = (self.SOM_FIFO_BASE, self.SOM_FIFO_LENGTH)
        self.dictB['seqr'] = (self.SOM_SEQR_BASE, self.SOM_SEQR_LENGTH)
        self.dictB['diag'] = (self.SOM_DIAG_BASE, self.SOM_DIAG_LENGTH)


    def parse_addr(self, sLine):
        "Parse address from DMA info command."

        ss = sLine.strip()
        ls = ss.split(':')
        return(int(ls[1],16))

    def parse_length(self, sLine):
        "Parse length from DMA info command."

        ss = sLine.strip()
        ls = ss.split(':')
        ss = ls[1].strip()
        ls = ss.split(' ')
        return(int(ls[0]))

    def clear(self, sBuffer, nWords=0, nClearValue=0):
        "Clear a DMA buffer."

        tBL = self.dictB[sBuffer]

        nAddr = tBL[0]

        if (nWords is 0):
            nW = tBL[1]
        else:
            nW = nWords
        
        statusObj = self.mob.run(['adb', 'shell', '/data/local/thortools/dmawrite', sBuffer, "{0:16x}".format(nAddr), '-s', "{0:08x}".format(nClearValue), str(nW)])
        print("Clear Buffer", sBuffer, nW, statusObj.stdout)

        
def create_ramp_pattern_file(nWords, nStart=0, sFile='test.dat'):
    "Write data to file"

    with open(sFile, 'wb') as FID:
        for i in range(nWords):
            n = nStart + i
            ln = []
            for j in range(4):
                ln.append(n & 0xff)
                n = n >> 8
            bn = bytes(ln)
            FID.write(bn)
        FID.close()

def file_to_ln(sFile, nWords):
    "Read binary file and return as list of ints."

    with open(sFile, 'rb') as FID:
        bn = FID.read()

        ln = []
        
        n = 0
        while (n < len(bn)):
            nData = 0
            for b in range(4):
                nData = nData | (bn[n] << (8 * b))
                n = n + 1
    
            ln.append(nData)
    
        return(ln)


def adb_clear_som(nAddr, nWords, nFill=0, sBuffer='fifo'):
    "Clear SOM FIFO memory."

    statusObj = proc.run(['adb', 'shell', '/data/local/thortools/dmawrite', sBuffer, "0x{:08x}".format(nAddr), '-s', "0x{:08x}".format(nFill), str(nWords)], stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
    if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
        print("ERROR::adb_clear_som: Problem with adb shell damwrite command.")
        print(statusObj.stdout)
        return(statusObj.returncode)
    else:
        return(0)


def adb_dmaread(sSOMBuf, nAddr, nWords, sFile=None):
    "Read SOM DMA buffer to file."
    
    if (sFile is None):
        statusObj = proc.run( ['adb', 'shell', '/data/local/thortools/dmaread', sSOMBuf, "0x{0:016x}".format(nAddr), str(nWords)] , stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
        if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
            print("ERROR::adb_dmaread: Problem with adb dmaread command.")
            print(statusObj.stdout)
            return([statusObj.returncode])
        else:

            lsData = statusObj.stdout.split()
            lnData = []
            if len(lsData) < 9:
                for e in lsData[1:]:
                    lnData.append(int(e,16))
            else:
                for el in range(int(len(lsData)/9)+1):
                    for e in lsData[el*9+1:el*9+9]:
                        lnData.append(int(e,16))
        
            #print("Read - Address: 0x{0:08x} Words: {1}".format(nAddr, nWords))
            #print(lnData)
    
            return(lnData)
    else: 
        statusObj = proc.run( ['adb', 'shell', '/data/local/thortools/dmaread', sSOMBuf, "0x{0:016x}".format(nAddr), str(nWords), '-f', SOM_DATA_PATH+sFile] , stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
        if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
            print("ERROR::adb_dmaread: Problem with adb dmaread command.")
            print(statusObj.stdout)
        else:
            rc = adb_pull_file(sFile)
            if (rc is 0):
                ln = file_to_ln(sFile, nWords)
                return(ln)
        
        return(-1)

    
def adb_dmawrite(sSOMBuf, nAddr, sFile):
    "Write file to SOM buffer."
    
    statusObj = proc.run( ['adb', 'shell', '/data/local/thortools/dmawrite', sSOMBuf, "0x{0:016x}".format(nAddr), '-f', sFile] , stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
    if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
        print("ERROR::adb_dmawrite: Problem with adb dmawrite command.")
        print(statusObj.stdout)
        return(statusObj.returncode)
    else:
        return(0)

    
def adb_pull_file(sFile):
    "Read file from SOM."
    global SOM_DATA_PATH
    statusObj = proc.run( ['adb', 'pull', SOM_DATA_PATH+sFile, sFile] , stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
    if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
        print("ERROR::adb_pull_file: Problem with adb pull command.")
        print(statusObj.stdout)
        return(-1)
    else:
        return(0)

    
def adb_push_file(sFile):
    "Move data file to SOM."
    global SOM_DATA_PATH

    statusObj = proc.run( ['adb', 'push', sFile, SOM_DATA_PATH+sFile] , stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
    if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
        print("ERROR::adb_push_file: Problem with adb push command.")
        print(statusObj.stdout)
        return(statusObj.returncode)
    else:
        return(0)


def adb_get_frame_irq():
    "Retrieve IRQ count from SOM frame_irq file."

    sFile = 'frame_irq'

    statusObj = proc.run(['adb', 'pull', SOM_DAUPCI_PATH+sFile, sFile], stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
    if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
        print("ERROR::adb_get_frame_irq: Problem pulling "+sFile)
        print(statusObj.stdout)
        return(statusObj.returncode)
    else:
        print(statusObj.stdout)

    if (os.path.exists(sFile)):
        with open(sFile, 'r') as FID:
            nIRQ = int(FID.read(),16)

            return (nIRQ)

    else:
        print("ERROR::adb_get_frame_irq: Could not find file ", sFile)
        
    return(-1)
        


def adb_get_error_irq():
    "Retrieve IRQ count from SOM frame_irq file."

    sFile = 'error_irq'


    statusObj = proc.run(['adb', 'pull', SOM_DAUPCI_PATH+sFile, sFile], stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
    if ((statusObj.returncode < 0) or ('error' in statusObj.stdout)):
        print("ERROR::adb_get_error_irq: Problem pulling "+sFile)
        print(statusObj.stdout)
        return(statusObj.returncode)
    else:
        print(statusObj.stdout)

    if (os.path.exists(sFile)):
        with open(sFile, 'r') as FID:
            nIRQ = int(FID.read(),16)

            return (nIRQ)

    else:
        print("ERROR::adb_get_error_irq: Could not find file ", sFile)
        
    return(-1)
        


def get_frame_data(sSomBuffer, nSomAddr, sFile, nFB, nFStride, bDebug=False, bReadFifo=True):
    """
    Parse raw frame data from file.

    Sort frame data in ascending order by frame index.

    Return data in following format:

    list[(header_list, data_list)]

    List of tuples with two sublists for header and frame data.
    """
    
    nWords = int(nFStride/4 * nFB)
    if (bReadFifo):
        lnRaw = adb_dmaread(sSomBuffer, nSomAddr,  nWords, sFile)

        if (lnRaw is -1):
            return(-1)
    else:
        lnRaw = file_to_ln(sFile, nWords)

    # Gather Headers and find lowest frame index as starting point
    nStep = int(nFStride/4)
    nStartIndex = 0xffffff
    nStartOffset = 0
    for ii in range(0, len(lnRaw), nStep):
        lnHeader = lnRaw[ii:ii+4]
        if (bDebug):
            print("Frame Offset: 0x{:x} Header: ".format(ii), lnHeader)

        if (lnHeader[0] < nStartIndex):
            nStartIndex = lnHeader[0]
            nStartOffset = ii
            
    if (bDebug):
        print("Start: {} Offset: 0x{:x}".format(nStartIndex, nStartOffset))

    # Re-arrange data in ascending order
    ltln = []

    nOffset = nStartOffset
    for f in range(nFB):
        lnHeader = lnRaw[nOffset:nOffset+4]
        lnData = lnRaw[nOffset+4:nOffset+nStep]
        ltln.append((lnHeader, lnData))
    
        nOffset = nOffset + nStep
        if (nOffset >= len(lnRaw)):
            nOffset = 0

    return(ltln)

