
"""
Class to access FPGA through PCI via SOM commands.
"""

import subprocess as proc

class FPGAPci:
    """
    This class contains routines for accessing the FPGA registers and memory via the SOM across PCI.

    Use Model:
        myPci = FPGAPci()
        myPci.open()

        myPci.write(0xaa112233, 0xdd112233)   # write a data word dd to address aa
        rdData = myPci.read(0xaa112233) # read address aa
    """

    TEST = False
    DEBUG_LEVEL = 0

    EXIT_ON_ERROR = False 

    ADDRESS_LIMIT = 0xFFFFFFFF

    def __init__(self, sADB='adb'):
        "Constructor"

        self.ADB = sADB

    def iam(self):
        "Kind"
        return("PCI")


    def run(self, lsCmd):
        """Run an external program as a subprocess."""

        if ((self.DEBUG_LEVEL == 0x8000) or (self.TEST is True)):
            ls = []
            for e in lsCmd:
                ls.append(str(e))
            print(" ".join(ls))
   
        if (self.TEST):
            statusObj = proc.CompletedProcess(args=lsCmd, returncode=0, stdout="0x0: 0")
            return(statusObj)

 
        statusObj = proc.run(lsCmd, stdout=proc.PIPE, stderr=proc.STDOUT, universal_newlines=True)
    
        if (statusObj.returncode < 0):
            print("FPGAPci::run: ERROR: ", statusObj.stderr)
            if (self.EXIT_ON_ERROR):
                exit(statusObj.returncode)
        else:
            if (self.DEBUG_LEVEL == 0x8000):
                print(statusObj.stdout)

        return (statusObj)


    def adb_shell(self, sCmd):
        "Run an ADB shell command."

        lsADB = [self.ADB, 'shell']
        lsADB.append(sCmd)

        statusObj  = self.run(lsADB)
        if (statusObj.stdout == "DAU Power is off"):
            print("FPGAPci::adb_shell: ERROR: PCIe Link is not Active")
            if (self.EXIT_ON_ERROR):
                exit(statusObj.returncode)

        return(statusObj)


    def adb_start(self):
        "Start ADB shell as root."
     
        lsADB = [self.ADB, 'root']

        if (self.TEST):
            print(" ".join(lsADB))
            return(0)
        else:
            print("Starting ADB as root")
            statusObj = self.run(lsADB)
            print(statusObj.stdout.rstrip('\n'))
            return(statusObj.returncode)


    def link_up(self):
        "Start the PCIu link."

        print("Starting the PCIe link.")
        sCmd = "/data/local/thortools/daupower on"
        statusObj = self.adb_shell(sCmd)
        print(statusObj.stdout.rstrip('\n'))


    def link_down(self):
        "Stop the PCIu link."

        print("Stopping the PCIe link.")
        sCmd = "daupower off"
        statusObj = self.adb_shell(sCmd)
        print(statusObj.stdout.rstrip('\n'))


    def sw_build(self):
        "Print the SW build number."

        sCmd = "getprop ro.build.id"
        idStatus = self.adb_shell(sCmd)
        sCmd = "getprop ro.build.version.incremental"
        buildStatus = self.adb_shell(sCmd)

        print("SOM SW ID :",idStatus.stdout.rstrip('\n'))
        print("BUILD     :",buildStatus.stdout.rstrip('\n'))


    def fpga_build(self):
        "Print the SW build number."

        nRevID = self.read(0x200000)
        nBuild = self.read(0x200004)
        nDate = self.read(0x200008)

        print("FPGA RevID: {0} Build: {1} Date: {2:2d}{3:2d}{4:2d}".format(nRevID[0], nBuild[0], nDate[0] >> 16, (nDate[0] >> 8) & 0xff, nDate[0] & 0xff))


    def open(self):
        """
        Initialize ADB access. 
        Start the PCIe link if link owner.
        Print the SOM SW Build Number.
        Print the FPGA Build Number.
        """

        self.adb_start()
        self.link_up()
        print(80*'=')
        self.sw_build()
        self.fpga_build()
        print(80*'=')


    def close(self):
        "Bring the link down to a stable point."

        #self.link_down()


    def read(self, nAddr, nWords=1, sFile=None):
        "Alias"
        return (self.readPCI(nAddr, nWords, sFile))

    def readTOP(self, nAddr, nWords=1, sFile=None):
        "Alias"
        return (self.readPCITOP(nAddr, nWords, sFile))


    def readPCI(self, nAddr, nWords=1, sFile=None):
        """ Read one word via PCI. """

        if ((nAddr < 0) or (nAddr >= self.ADDRESS_LIMIT)):
            print("FPGAPci:readPCI: Error Address {0:08x} out of current accepted range. Range: 0x{1:08x}".format(nAddr, self.ADDRESS_LIMIT))
            if (self.EXIT_ON_ERROR):
                exit(-1)
            else:
                return([0] * nWords)

        if (nWords < 1):
            print("FPGAPci:readPCI: Error Invalid Word Length {0} Address: 0x{1:08x}.".format(nWords, nAddr))
            if (self.EXIT_ON_ERROR):
                exit(-1)
            else:
                return([0])

        #lsCmd = ['dauread', nAddr, nWords]
        sCmd = "/data/local/thortools/dauread -a 0x{0:08x} {1}".format(nAddr, nWords)

        if (sFile != None):
            #lsCmd = lsCmd + ['-f', sFile]
            sCmd = "/data/local/thortools/dauread -f {0} -a 0x{1:08x} {2} ".format(sFile, nAddr, nWords)
        statusObj = self.adb_shell(sCmd)

        # adb shell returns its own returncode and not the returncode of the
        # command it was asked to execute. The output must be parsed to check for errors.

        if (statusObj.returncode != 0):
            if (self.EXIT_ON_ERROR):
                exit(statusObj.returncode)
            return(statusObj.returncode)

        else:
            if (sFile is None):
                lsData = statusObj.stdout.split()
                #nData = int(lsData[1], 16)
                lnData = []
                if len(lsData) < 9:
                    for e in lsData[1:]:
                        lnData.append(int(e,16))
    
                    if (self.DEBUG_LEVEL == 1):
                        print("Read - Address: 0x{0:08x} Words: {1}".format(nAddr, nWords))
                        print(statusObj.stdout)
    
                    #return(nData)
                    return(lnData)
                else:
                    for el in range(int(len(lsData)/9)+1):
                        for e in lsData[el*9+1:el*9+9]:
                            lnData.append(int(e,16))
        
                        if (self.DEBUG_LEVEL == 1):
                            print("Read - Address: 0x{0:08x} Words: {1}".format(nAddr, nWords))
                            print(statusObj.stdout)
    
                    #return(nData)
                    return(lnData)
            else:
                if (self.DEBUG_LEVEL == 1):
                    print("Read - Address: 0x{0:08x} Words: {1} to Data File {2}".format(nAddr, nWords, sFile))
                    print(statusObj.stdout)

                return([statusObj.returncode])

    def readPCITOP(self, nAddr, nWords=1, sFile=None):
        """ Read one word via PCI. """

        if ((nAddr < 0) or (nAddr >= self.ADDRESS_LIMIT)):
            print("FPGAPci:readPCI: Error Address {0:08x} out of current accepted range. Range: 0x{1:08x}".format(nAddr, self.ADDRESS_LIMIT))
            if (self.EXIT_ON_ERROR):
                exit(-1)
            else:
                return([0] * nWords)

        if (nWords < 1):
            print("FPGAPci:readPCI: Error Invalid Word Length {0} Address: 0x{1:08x}.".format(nWords, nAddr))
            if (self.EXIT_ON_ERROR):
                exit(-1)
            else:
                return([0])

        #lsCmd = ['dauread', nAddr, nWords]
        sCmd = "/data/local/thortools/dauread 0x{0:08x} {1}".format(nAddr, nWords)

        if (sFile != None):
            #lsCmd = lsCmd + ['-f', sFile]
            sCmd = "/data/local/thortools/dauread -f {0} 0x{1:08x} {2} ".format(sFile, nAddr, nWords)
        statusObj = self.adb_shell(sCmd)

        # adb shell returns its own returncode and not the returncode of the
        # command it was asked to execute. The output must be parsed to check for errors.

        if (statusObj.returncode != 0):
            if (self.EXIT_ON_ERROR):
                exit(statusObj.returncode)
            return(statusObj.returncode)

        else:
            if (sFile is None):
                lsData = statusObj.stdout.split()
                #nData = int(lsData[1], 16)
                lnData = []
                if len(lsData) < 9:
                    for e in lsData[1:]:
                        lnData.append(int(e,16))
   
                    if (self.DEBUG_LEVEL == 1):
                        print("Read - Address: 0x{0:08x} Words: {1}".format(nAddr, nWords))
                        print(statusObj.stdout)
   
                    #return(nData)
                    return(lnData)
                else:
                    for el in range(int(len(lsData)/9)+1):
                        for e in lsData[el*9+1:el*9+9]:
                            lnData.append(int(e,16))

                        if (self.DEBUG_LEVEL == 1):
                            print("Read - Address: 0x{0:08x} Words: {1}".format(nAddr, nWords))
                            print(statusObj.stdout)

                    #return(nData)
                    return(lnData)
            else:
                if (self.DEBUG_LEVEL == 1):
                    print("Read - Address: 0x{0:08x} Words: {1} to Data File {2}".format(nAddr, nWords, sFile))
                    print(statusObj.stdout)

                return([statusObj.returncode])



    
    def write(self, nAddr, nData=0, sFile=None):
        "Alias"

        return (self.writePCI(nAddr, nData, sFile))


    def writePCI(self, nAddr, nData, sFile=None):
        """ Write one word via Serial Port """
       
        if ((nAddr < 0) or (nAddr >= self.ADDRESS_LIMIT)):
            print("FPGAPci:writePCI: Error Address {0:08x} out of current accepted range. Range: 0x{1:08x}".format(nAddr, self.ADDRESS_LIMIT))
            if (self.EXIT_ON_ERROR):
                exit(-1)
            else:
                return(-1)


        if  (sFile is None):
            sCmd = "/data/local/thortools/dauwrite -a 0x{0:08x} 0x{1:08x}".format(nAddr, nData)
            #lsCmd = ['dauwrite', nAddr, nData]
        else:
            sCmd = "/data/local/thortools/dauwrite -f {0} -a 0x{1:08x} ".format(sFile, nAddr)
            #lsCmd = ['dauwrite', nAddr, '-f', sFile]

        statusObj = self.adb_shell(sCmd)

        if (statusObj.returncode != 0):
            if (self.EXIT_ON_ERROR):
                exit(statusObj.returncode)

        if (self.DEBUG_LEVEL == 1):
            print("Write - Address: 0x{0:08x} Data: 0x{1:08x}".format(nAddr, nData))
            print(statusObj.stdout)

        return (statusObj.returncode)

    def writeTOP(self, nAddr, nData=0, sFile=None):
        return (self.writePCITOP(nAddr, nData, sFile))

    def writePCITOP(self, nAddr, nData, sFile=None):
        """ Write one word via Serial Port """
       
        if ((nAddr < 0) or (nAddr >= self.ADDRESS_LIMIT)):
            print("FPGAPci:writePCI: Error Address {0:08x} out of current accepted range. Range: 0x{1:08x}".format(nAddr, self.ADDRESS_LIMIT))
            if (self.EXIT_ON_ERROR):
                exit(-1)
            else:
                return(-1)


        if  (sFile is None):
            sCmd = "/data/local/thortools/dauwrite  0x{0:08x} 0x{1:08x}".format(nAddr, nData)
            #lsCmd = ['dauwrite', nAddr, nData]
        else:
            sCmd = "/data/local/thortools/dauwrite -f {0} 0x{1:08x} ".format(sFile, nAddr)
            #lsCmd = ['dauwrite', nAddr, '-f', sFile]

        statusObj = self.adb_shell(sCmd)

        if (statusObj.returncode != 0):
            if (self.EXIT_ON_ERROR):
                exit(statusObj.returncode)

        if (self.DEBUG_LEVEL == 1):
            print("Write - Address: 0x{0:08x} Data: 0x{1:08x}".format(nAddr, nData))
            print(statusObj.stdout)

        return (statusObj.returncode)

