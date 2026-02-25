#support
#V4 files
import numpy as np
import sqlite3

class ThorReader:

    def __init__(self):
        self.initVar()

    def __init__(self, file_name):
        self.initVar()
        self.mFileName = file_name

    def initVar(self):
        self.mNumRays = 112
        self.mNumSamples = 512
        self.mImagingCase = -1
        self.mImagingMode = -1
        self.mImagingDepth = -1
        self.mFrameCount = -1
        self.mBLoc = -1
        self.mCLoc = -1
        self.mMLoc = -1
        self.mDALoc = -1
        self.mECGLoc = -1
        self.mPWLoc = -1
        self.mFrameHeaderSize = 0
        self.mBmodeHeaderSize = 0
        self.mCmodeHeaderSize = 0
        self.mMmodeHeaderSize = 0
        self.mDAmodeHeaderSize = 0
        self.mECGmodeHeaderSize = 0
        self.mMetaDataSize = 0
        self.mFileVersion = -1
        #
        self.mBMaxFrameSize = 512 * 256
        self.mCMaxFrameSize = 90 * 224
        self.mMMaxFrameSize = 512 * 32
        self.mDAMaxFrameSize = 2048 * 4
        self.mECGMaxFrameSize = 512 * 4

    def setFileName(self, file_name):
        self.mFileName = file_name
        self.mImagingCase = -1

    def setParams(self, numSamples, numRays):
        self.mNumSamples = numSamples
        self.mNumRays = numRays

    def readHeaderSize(self, version):
        if (version == 5):
            self.mFrameHeaderSize = 24;
            self.mBmodeHeaderSize = 32; # SC
            self.mCmodeHeaderSize = 37; # SC + 5 bit
            self.mMmodeHeaderSize = 18;
            self.mDAmodeHeaderSize = 33;
            self.mECGmodeHeaderSize = 33;
            self.mMetaDataSize = 41;
        elif (version == 4):
            self.mFrameHeaderSize = 24;
            self.mBmodeHeaderSize = 0;
            self.mCmodeHeaderSize = 37; # SC + 5 bit
            self.mMmodeHeaderSize = 18;
            self.mDAmodeHeaderSize = 33;
            self.mECGmodeHeaderSize = 33;
            self.mMetaDataSize = 41;
        elif (version == 3):
            self.mFrameHeaderSize = 16;
            self.mBmodeHeaderSize = 0;
            self.mCmodeHeaderSize = 32;
            self.mMmodeHeaderSize = 18;
            self.mDAmodeHeaderSize = 25;
            self.mECGmodeHeaderSize = 25;
        return

    def readFile(self):
        #
        frame_cnt_loc = 29
        imaging_case_loc = 17
        version_loc = 4
        datatype_loc = 37
        #
        fd = open(self.mFileName, "r")
        fd.seek(version_loc)
        file_version = np.fromfile(fd, dtype=np.uint8, count=1)[0]
        self.mFileVersion = file_version
        print("file version: ", file_version)
        self.readHeaderSize(file_version);
        fd.seek(frame_cnt_loc)
        frame_cnt = np.fromfile(fd, dtype=np.int32, count=1)[0]
        self.mFrameCount = frame_cnt
        fd.seek(imaging_case_loc)
        imaging_case = np.fromfile(fd, dtype=np.int32, count=1)[0]
        self.mImagingCase = imaging_case
        print("imaging case ID: ", imaging_case)
        print("frame count: ", frame_cnt)
        fd.seek(datatype_loc)
        data_type = np.fromfile(fd, dtype=np.int32, count=1)[0]
        print("data_type: ", data_type);
        # Loc Offset
        loc_offset = self.mMetaDataSize;
        # B-Mode
        if (data_type >> 0 & 1):
            print("This file has Bmode data")
            self.mBLoc = loc_offset     # header location
            loc_offset += self.mBmodeHeaderSize + (self.mBMaxFrameSize + self.mFrameHeaderSize) * self.mFrameCount
        # C
        if (data_type >> 1 & 1):
            print("This file has Cmode data")
            self.mCLoc = loc_offset     # header location
            loc_offset += self.mCmodeHeaderSize + (self.mCMaxFrameSize + self.mFrameHeaderSize) * self.mFrameCount
        # PW
        if (data_type >> 2 & 1):
            print("This file has PW data")
            self.mPWLoc = 0
        # M
        if (data_type >> 3 & 1):
            print("This file has Mmode data")
            self.mMLoc = loc_offset     # header location
            loc_offset += self.mMmodeHeaderSize + (self.mMMaxFrameSize + self.mFrameHeaderSize) * self.mFrameCount
        # DA
        if (data_type >> 7 & 1):
            print("This file has DA data")
            self.mDALoc = loc_offset     # headr location
            loc_offset += self.mDAmodeHeaderSize + (self.mDAMaxFrameSize + self.mFrameHeaderSize) * self.mFrameCount
        # ECG
        if (data_type >> 8 & 1):
            print("This file has ECG data")
            self.mECGLoc = loc_offset     # headr location
            loc_offset += self.mECGmodeHeaderSize + (self.mECGMaxFrameSize + self.mFrameHeaderSize) * self.mFrameCount
        #
        #
        fd.close()
        #
        return

    def readThorBFrames(self):
        # file headersize: 41
        # cineFrameHeader: 16
        #
        # frame_cnt loc : 33
        #
        self.readBSCParams()
        #
        fd = open(self.mFileName, "r")
        #
        out_data = np.zeros([self.mFrameCount, self.mNumRays, self.mNumSamples], dtype = np.uint8)
        #
        for i in range(self.mFrameCount):
            start_loc = self.mBLoc + self.mBmodeHeaderSize + self.mFrameHeaderSize + (self.mBMaxFrameSize + self.mFrameHeaderSize) * i
            fd.seek(start_loc)
            # read
            data_array = np.fromfile(fd, dtype=np.uint8, count=self.mNumSamples * self.mNumRays)
            cur_frame = data_array.reshape((self.mNumRays, self.mNumSamples))
            #
            out_data[i, :, :] = cur_frame
        fd.close()
        #
        return out_data

    def readThorECGFrames(self):
        #
        #
        ecg_header_loc = self.mECGLoc
        #
        cineheader_size = self.mFrameHeaderSize
        ecg_max_frame_size = self.mECGMaxFrameSize
        #
        ecg_data_loc = ecg_header_loc + self.mECGmodeHeaderSize
        ecg_samples_frame_loc = ecg_header_loc + 20;
        ecg_frame_width_loc = ecg_header_loc + 4;
        ecg_scr_frame_loc = ecg_header_loc + 24;
        fd = open(self.mFileName, "r")
        fd.seek(ecg_samples_frame_loc)
        samplesPerFrame = np.fromfile(fd, dtype=np.int32, count=1)[0]
        print("samples Per Frame: ", samplesPerFrame)
        #
        fd.seek(ecg_frame_width_loc)
        frameWidth = np.fromfile(fd, dtype=np.int32, count=1)[0]
        print("frame width: ", frameWidth)
        #
        fd.seek(ecg_scr_frame_loc)
        isScrFrame = np.fromfile(fd, dtype=np.bool, count=1)[0]
        print("isScrFrame: ", isScrFrame)
        #
        if (~isScrFrame):
            # non screen frame
            out_data = np.zeros([self.mFrameCount * samplesPerFrame], dtype = np.float32)
            idx = 0;
            for i in range(self.mFrameCount):
                data_loc = ecg_data_loc + cineheader_size + (ecg_max_frame_size + cineheader_size) * i
                fd.seek(data_loc)
                #read
                data_array = np.fromfile(fd, dtype=np.float32, count=samplesPerFrame)
                out_data[idx:idx+samplesPerFrame] = data_array
                idx = idx + samplesPerFrame
        else:
            # ScreenFrame:
            out_data = 0
            print("not yet implemented");
        #
        fd.close()
        #
        return out_data;

    def readThorDAFrames(self):
        #
        #
        da_header_loc = self.mDALoc
        #
        cineheader_size = self.mFrameHeaderSize
        da_max_frame_size = self.mDAMaxFrameSize
        #
        da_data_loc = da_header_loc + self.mDAmodeHeaderSize
        da_samples_frame_loc = da_header_loc + 20;
        da_frame_width_loc = da_header_loc + 4;
        da_scr_frame_loc = da_header_loc + 24;
        fd = open(self.mFileName, "r")
        fd.seek(da_samples_frame_loc)
        samplesPerFrame = np.fromfile(fd, dtype=np.int32, count=1)[0]
        print("samples Per Frame: ", samplesPerFrame)
        #
        fd.seek(da_frame_width_loc)
        frameWidth = np.fromfile(fd, dtype=np.int32, count=1)[0]
        print("frame width: ", frameWidth)
        #
        fd.seek(da_scr_frame_loc)
        isScrFrame = np.fromfile(fd, dtype=np.bool, count=1)[0]
        print("isScrFrame: ", isScrFrame)
        #
        if (~isScrFrame):
            # non screen frame
            out_data = np.zeros([self.mFrameCount * samplesPerFrame], dtype = np.float32)
            idx = 0;
            for i in range(self.mFrameCount):
                data_loc = da_data_loc + cineheader_size + (da_max_frame_size + cineheader_size) * i
                fd.seek(data_loc)
                #read
                data_array = np.fromfile(fd, dtype=np.float32, count=samplesPerFrame)
                out_data[idx:idx+samplesPerFrame] = data_array
                idx = idx + samplesPerFrame
        else:
            # ScreenFrame:
            out_data = 0
            print("not yet implemented");
        #
        fd.close()
        #
        return out_data;

    def readBSCParams(self):
        #
        if (self.mFileVersion == 5):
            sc_params = self.readBSCParamsFromFileHeader()
        else:
            sc_params = self.readBSCParamsFromDB()

        return sc_params

    def readBSCParamsFromFileHeader(self):
        #
        fd = open(self.mFileName, "r")
        start_loc = self.mBLoc
        fd.seek(start_loc)
        #
        num_samplemesh = np.fromfile(fd, dtype=np.int32, count=1)[0]
        num_raymesh = np.fromfile(fd, dtype=np.int32, count=1)[0]
        numSamples = np.fromfile(fd, dtype=np.int32, count=1)[0]
        numRays = np.fromfile(fd, dtype=np.int32, count=1)[0]
        startSampleMm = np.fromfile(fd, dtype=np.float32, count=1)[0]
        sampleSpaingMm = np.fromfile(fd, dtype=np.float32, count=1)[0]
        raySpacing = np.fromfile(fd, dtype=np.float32, count=1)[0]
        fovStart = np.fromfile(fd, dtype=np.float32, count=1)[0]
        #
        self.mNumSamples = numSamples
        self.mNumRays = numRays
        #
        sc_params = {
            'numSamples': numSamples,
            'numRays': numRays,
            'startSampleMm': startSampleMm,
            'sampleSpacingMm': sampleSpaingMm,
            'startRayRadian': fovStart,
            'raySpacingRadian': raySpacing
        }
        #
        print("----- read B Scan Converter Params from fileHeader-----")
        print("numSamples: ", sc_params['numSamples'])
        print("numRays: ", sc_params['numRays'])
        print("startSample: ", sc_params['startSampleMm'])
        print("sampleSpacing: ", sc_params['sampleSpacingMm'])
        print("startRay: ", sc_params['startRayRadian'])
        print("raySpacing: ", sc_params['raySpacingRadian'])

        fd.close()
        #
        return sc_params

    def readBSCParamsFromDB(self):
        #
        if (self.mImagingCase < 0):
            self.getImagingCase()

        conn = sqlite3.connect('thor.db')
        c = conn.cursor()
        #
        ICaseID = self.mImagingCase
        #
        c.execute("SELECT Depth, Mode FROM ImagingCases WHERE ID = '%s'" % ICaseID)
        [depthId, mode] = c.fetchone()
        self.mImagingMode = mode
        #
        c.execute("SELECT Depth, FovAzimuthSpan, FovAzimuthStart, NumRxBeamsB, NumRxBeamsBC, RxPitchB, RxPitchBC FROM Depths WHERE ID = '%s'" % depthId)
        [depth, fovSpan, fovStart, numRxB, numRxBC, raySpacingB, raySpacingBC] = c.fetchone()
        # numSamples = 512 -> From Global
        self.mImagingDepth = depth
        numSamples = 512
        sampleSpaingMm = depth * 1.0 / (numSamples - 1)
        startSampleMm = 0.0
        # close connection
        c.close()
        conn.close()
        #
        numRays = numRxB
        if (self.mImagingMode == 1):
            numRays = numRxBC
        #
        raySpacing = raySpacingB
        if (self.mImagingMode == 1):
            raySpacing = raySpacingBC
        #
        self.mNumSamples = numSamples
        self.mNumRays = numRays
        #
        sc_params = {
            'numSamples': numSamples,
            'numRays': numRays,
            'startSampleMm': startSampleMm,
            'sampleSpacingMm': sampleSpaingMm,
            'startRayRadian': fovStart,
            'raySpacingRadian': raySpacing
        }
        #
        print("----- read B Scan Converter Params from dB -----")
        print("numSamples: ", sc_params['numSamples'])
        print("numRays: ", sc_params['numRays'])
        print("startSample: ", sc_params['startSampleMm'])
        print("sampleSpacing: ", sc_params['sampleSpacingMm'])
        print("startRay: ", sc_params['startRayRadian'])
        print("raySpacing: ", sc_params['raySpacingRadian'])
        #
        return sc_params

    def getImagingDepth(self):
        return self.mImagingDepth

    def getImagingCase(self):
        # imaging case location
        imaging_case_loc = 17
        #
        fd = open(self.mFileName, "r")
        fd.seek(imaging_case_loc)
        imaging_case = np.fromfile(fd, dtype=np.int32, count=1)[0]
        self.mImagingCase = imaging_case
        fd.close()
        return imaging_case

    def readCSCParams(self):
        # file headersize: 41
        # cineFrameHeader: 16
        #
        # frame_cnt loc : 33
        metadata_size = 41
        frame_cnt_loc = 29
        imaging_case_loc = 17
        cineheader_size = 24
        b_max_samples = 512
        b_max_rays = 256
        c_max_samples = 256
        c_max_rays = 128
        #
        #
        fd = open(self.mFileName, "r")
        fd.seek(frame_cnt_loc)
        frame_cnt = np.fromfile(fd, dtype=np.int32, count=1)[0]
        fd.seek(imaging_case_loc)
        imaging_case = np.fromfile(fd, dtype=np.int32, count=1)[0]
        #print("imaging case ID: ", imaging_case)
        #print("frame count: ", frame_cnt)
        # color header location
        start_loc = metadata_size + (b_max_samples * b_max_rays + cineheader_size) * frame_cnt
        fd.seek(start_loc)
        # colorParams
        color_sc_size = 32
        num_samplemesh = np.fromfile(fd, dtype=np.int32, count=1)[0]
        num_raymesh = np.fromfile(fd, dtype=np.int32, count=1)[0]
        c_num_sample = np.fromfile(fd, dtype=np.int32, count=1)[0]
        c_num_rays = np.fromfile(fd, dtype=np.int32, count=1)[0]
        c_startSampleMm = np.fromfile(fd, dtype=np.float32, count=1)[0]
        c_sampleSpacingMm = np.fromfile(fd, dtype=np.float32, count=1)[0]
        c_raySpacing = np.fromfile(fd, dtype=np.float32, count=1)[0]
        c_startRayRadian = np.fromfile(fd, dtype=np.float32, count=1)[0]
        print("----- read C Scan Converter Params -----")
        print("numSamples: ", c_num_sample)
        print("numRays: ", c_num_rays)
        print("startSample: ", c_startSampleMm)
        print("sampleSpacing: ", c_sampleSpacingMm)
        print("startRay: ", c_startRayRadian)
        print("raySpacing: ", c_raySpacing)
        sc_params = {
            'numSamples': c_num_sample,
            'numRays': c_num_rays,
            'startSampleMm': c_startSampleMm,
            'sampleSpacingMm': c_sampleSpacingMm,
            'startRayRadian': c_startRayRadian,
            'raySpacingRadian': c_raySpacing
        }
        fd.close()
        #
        return sc_params

    def readThorCFrames(self):
        # file headersize: 41
        # cineFrameHeader: 16
        #
        # frame_cnt loc : 33
        metadata_size = 41
        frame_cnt_loc = 29
        imaging_case_loc = 17
        cineheader_size = 24
        b_max_samples = 512
        b_max_rays = 256
        c_max_samples = 256
        c_max_rays = 128
        #
        #
        fd = open(self.mFileName, "r")
        fd.seek(frame_cnt_loc)
        frame_cnt = np.fromfile(fd, dtype=np.int32, count=1)[0]
        fd.seek(imaging_case_loc)
        imaging_case = np.fromfile(fd, dtype=np.int32, count=1)[0]
        #print("imaging case ID: ", imaging_case)
        #print("frame count: ", frame_cnt)
        # color header location
        start_loc = metadata_size + (b_max_samples * b_max_rays + cineheader_size) * frame_cnt
        fd.seek(start_loc)
        # colorParams
        color_sc_size = 32
        num_samplemesh = np.fromfile(fd, dtype=np.int32, count=1)[0]
        num_raymesh = np.fromfile(fd, dtype=np.int32, count=1)[0]
        c_num_sample = np.fromfile(fd, dtype=np.int32, count=1)[0]
        c_num_rays = np.fromfile(fd, dtype=np.int32, count=1)[0]
        c_startSampleMm = np.fromfile(fd, dtype=np.float32, count=1)[0]
        c_sampleSpacingMm = np.fromfile(fd, dtype=np.float32, count=1)[0]
        c_raySpacing = np.fromfile(fd, dtype=np.float32, count=1)[0]
        c_startRayRadian = np.fromfile(fd, dtype=np.float32, count=1)[0]
        #print("numSamples: ", c_num_sample)
        #print("numRays: ", c_num_rays)
        #print("startSample: ", c_startSampleMm)
        #print("sampleSpacing: ", c_sampleSpacingMm)
        #print("startRay: ", c_startRayRadian)
        #print("raySpacing: ", c_raySpacing)
        #
        color_offset = metadata_size + (b_max_samples * b_max_rays + cineheader_size) * frame_cnt + color_sc_size
        #
        out_data = np.zeros([frame_cnt, c_num_rays, c_num_sample], dtype = np.uint8)
        #
        for i in range(frame_cnt):
            start_loc = color_offset + cineheader_size + (c_max_samples * c_max_rays + cineheader_size) * i
            fd.seek(start_loc)
            # read
            data_array = np.fromfile(fd, dtype=np.uint8, count=c_num_sample * c_num_rays)
            cur_frame = data_array.reshape((c_num_rays, c_num_sample))
            #
            out_data[i, :, :] = cur_frame
        fd.close()
        #
        return out_data
