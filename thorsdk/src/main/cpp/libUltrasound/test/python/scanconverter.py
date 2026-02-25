# Scan Converter
# based-on 4x2 cubic interpolation

import numpy as np
#import matplotlib.pyplot as plt
import math

class ScanConverter:

    # constructor/initializer setting up variables
    def __init__(self):
        self.mNumRays = 112
        self.mNumSamples = 512
        self.mScreenWidth = 256
        self.mScreenHeight = 256
        self.mTopStrip = 0
        self.mBottomStrip = 0
        self.mImagingDepth = 180
        self.mRayStartDegree = 55
        self.mRaySpacing = -1
        self.mStartSampleMm = 0;
        self.mSampleSpacing = 0.1;

    # setting scan converter parameters
    def setParams(self, imagingDepth, numSamples, numRays, startSampleMM, sampleSpacing, rayStartDegree, raySpacingDegree, screenWidth, screenHeight, topStrip, bottomeStrip):
        self.mImagingDepth = imagingDepth
        self.mNumSamples = numSamples
        self.mNumRays = numRays
        self.mScreenWidth = screenWidth
        self.mScreenHeight = screenHeight
        self.mTopStrip = topStrip
        self.mBottomStrip = bottomeStrip
        self.mRayStartDegree = rayStartDegree
        self.mRaySpacing = raySpacingDegree
        self.mStartSampleMm = startSampleMM;
        self.mSampleSpacing = sampleSpacing;

    # function define - cubic interpolate
    def cubicInterpolate(self, y0, y1, y2, y3, mu):
        mu2 = mu*mu
        a0 = y3 - y2 - y0 + y1
        a1 = y0 - y1 - a0
        a2 = y2 - y0
        a3 = y1
        rst = a0*mu*mu2+a1*mu2+a2*mu+a3
        return rst

    # inData -> input data in np Int array in [numRays, numSamples]
    #
    def convert(self, inData):
        # calculate scale and offsets
        x_offset = self.mScreenWidth/2
        y_offset = self.mTopStrip
        screen_y_size = self.mScreenHeight - self.mTopStrip - self.mBottomStrip
        imaging_depth = self.mImagingDepth
        scale = screen_y_size/imaging_depth
        #
        # physical to normalize screen
        m_m2p = np.zeros([4,4], dtype = float)
        m_m2p[0,0] = scale
        m_m2p[1,1] = scale
        m_m2p[2,2] = 1
        m_m2p[3,3] = 1
        m_m2p[0,3] = x_offset
        m_m2p[1,3] = y_offset
        #
        apex_loc_mm = np.array([[0],
                     [0],
                     [0],
                     [1]])
        #
        apex_pix = m_m2p@apex_loc_mm
        # inverse
        m_p2m = np.linalg.inv(m_m2p)
        # FoV
        start_degree = self.mRayStartDegree
        ray_spacing = self.mRaySpacing
        end_degree = start_degree + ray_spacing * (self.mNumRays - 1)
        # conversion from x-axis = 0 -a-axis = -180
        if (ray_spacing < 0):
            start_degree = 90 - start_degree
            end_degree = 90 - end_degree
        else:
            org_start = start_degree
            start_degree = 90 - end_degree
            end_degree = 90 - org_start
        #
        scan_out = np.zeros([self.mScreenHeight, self.mScreenWidth], dtype = np.uint8)
        out_pix = 0
        pix_loc = np.array([[0],[0],[0],[1]])
        #input_x_scale = imaging_depth/(self.mNumSamples-1)
        input_x_scale = self.mSampleSpacing
        start_depth = self.mStartSampleMm
        end_depth = self.mStartSampleMm + self.mSampleSpacing * (self.mNumSamples - 1)
        #
        for j in range(self.mScreenHeight):
            pix_loc[1] = j
            for i in range(self.mScreenWidth):
                pix_loc[0] = i
                mm_loc = m_p2m@pix_loc
                x_dis = mm_loc[0] - apex_loc_mm[0]
                y_dis = mm_loc[1] - apex_loc_mm[1]
                xy_dis = math.hypot(x_dis, y_dis)
                deg = math.atan2(y_dis,x_dis)*180/math.pi
                #print(x_dis, y_dis, xy_dis, deg)
                if y_dis < start_depth or y_dis > end_depth:
                    out_pix = 0
                else:
                    if deg < start_degree or deg > end_degree:
                        out_pix = 0
                    else:
                        #interpolation here
                        #linear interpolation
                        x_loc = (xy_dis - start_depth)/input_x_scale
                        r_loc = (deg - start_degree)/abs(ray_spacing)
                        x_loc_int = math.floor(x_loc)
                        x_frac = x_loc - x_loc_int
                        r_loc_int = math.floor(r_loc)
                        r_frac = r_loc - r_loc_int
                        if x_loc >=0 and x_loc < self.mNumSamples - 1 and r_loc >=0 and r_loc < self.mNumRays - 1:
                            y0_loc = r_loc_int - 1
                            y1_loc = r_loc_int
                            y2_loc = r_loc_int + 1
                            y3_loc = r_loc_int + 2
                            if r_loc_int == 0:
                                y0_loc = 0
                            elif r_loc_int == self.mNumRays - 2:
                                y3_loc = r_loc_int + 1
                            ####
                            y0q0 = inData[y0_loc, x_loc_int]
                            y1q0 = inData[y1_loc, x_loc_int]
                            y2q0 = inData[y2_loc, x_loc_int]
                            y3q0 = inData[y3_loc, x_loc_int]
                            yyq0 = self.cubicInterpolate(y0q0*1.0, y1q0*1.0, y2q0*1.0, y3q0*1.0, r_frac)
                            ####
                            y0q1 = inData[y0_loc, x_loc_int+1]
                            y1q1 = inData[y1_loc, x_loc_int+1]
                            y2q1 = inData[y2_loc, x_loc_int+1]
                            y3q1 = inData[y3_loc, x_loc_int+1]
                            yyq1 = self.cubicInterpolate(y0q1*1.0, y1q1*1.0, y2q1*1.0, y3q1*1.0, r_frac)
                            ####
                            out_pix = yyq0*(1-x_frac)+yyq1*x_frac
                        else:
                            out_pix = 0
                    #
                    if out_pix > 255:
                        out_pix = 255
                    if out_pix < 0:
                        out_pix = 0
                    scan_out[j,i] = out_pix
                    #
        return scan_out;

######### end of class ScanConverter ############
