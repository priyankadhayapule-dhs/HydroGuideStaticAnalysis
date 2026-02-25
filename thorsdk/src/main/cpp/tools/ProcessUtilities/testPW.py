import os
import numpy
numpy.seterr(all='ignore')
import warnings
warnings.simplefilter("ignore", numpy.ComplexWarning)
from numpy.core import array,zeros,ones,conj,where,arange,sum,size
from numpy import mat, multiply, shape
from numpy.core import complex64,float32,int8
from numpy.core import pi,sin,cos,log10,floor,ceil,round,fmod
from numpy import linalg
from numpy.lib import vander,diag,angle,interp
from scipy.signal import hanning,lfilter,boxcar,hamming,butter,ellip,kaiser,blackmanharris,firwin, upfirdn, resample_poly
from numpy.fft import fft,fftshift
from scipy.special import i0
from matplotlib.pylab import imshow,figure,show, plot, title
from matplotlib.colors import LinearSegmentedColormap,Normalize
import array as array_builtin
import numpy as np
import scipy.signal as sci
import matplotlib.pyplot as plt
from matplotlib.pylab import imshow, subplot, plot, figure, title, show, xticks, ylabel
def pwimage(x, aspect='auto', interpolation='bilinear', origin='lower'):

    figure();
    prf=4238.55794270833
    BaselineShift = 0.
    PosMaxVelCmPerSec = (0.5 - BaselineShift) * 0.1 * prf * 1.54 / (2 * 2)
    NegMaxVelCmPerSec = (BaselineShift + 0.5) * 0.1 * prf * 1.54 / (2 * 2)
    imshow(array(x.transpose(),dtype=float),aspect=aspect,interpolation=interpolation,cmap='gray',norm=Normalize(vmin=0,vmax=np.max(x)),origin=origin,extent=[-1,1,-NegMaxVelCmPerSec, PosMaxVelCmPerSec]);

    h = ylabel('-        cm/s        +', rotation=90);
    xticks([]);

    #subplot(122);plot(x.T);
    show(False)

class pw:
    def __init__(self):
        self.NumSamplesPerLine = 52
        self.NumLinesPerFrame = 192
        self.NumFrames = 10
        self.WallFilterSpectralCutoff = 0.06
        self.WallFilterSpectralOrder = 4
        self.WallFilterAudioCutoff = 0.01 #Carotid setting
        self.WallFilterAudioCutoff = 0.02 #Carotid2goodSNR setting
        self.WallFilterAudioCutoff = 0.02 #LVgoodSNR setting
        self.DownsampleAudioFilterOrder = 5
        self.DownsampleSpectralFilterOrder = 5
        self.WallFilterAudioOrder = 5
        self.AudioHilbertFirLen = 47
        self.AudioHilbertFirBeta = 1.0
        self.AudioHilbertMinSampleRateHz = 0
        self.AudioHilbertOrder = 6
        self.AudioHilbertPassbandRippleDb = 1.0
        self.AudioHilbertStopbandRippleDb = 50.0
        self.AudioHilbertType = 1
        self.AudioSplitterOrder = 6
        self.AudioSplitterPassbandRippleDb = 1
        self.AudioSplitterStopbandRippleDb = 50
        self.AudioSplitterType = 1
        self.AudioUpsampleMaxSampleRateHz = 4238.55794270833
        self.AudioUpsampleFilterOrder = 8
        self.AudioTransitionBandwidth = 0.005
        self.FftAverageNum = 1
        self.AcqSpectralSampleRateHz = 4238.55794270833
        self.AcqAudioSampleRateHz = 4238.55794270833
        self.SampleRateHz = 4238.55794270833
        self.BaselineShift = 0.
        self.Invert = 1

    def loadIq(self,iqFilename=None):
        from scipy.signal import chirp
        x = np.arange(0,0.0451,1/4238.55794270833)
        x = np.arange(0,0.4528,1/4238.55794270833)
        iq=np.zeros([1,1920]).astype(np.complex64)
        for fi in range(self.NumFrames):
            iq[0,fi*192:192+fi*192] = np.cos(3000*x[fi*192:192+fi*192])+1.0j*np.sin(1000*x[fi*192:192+fi*192])+20*(np.cos(50*x[fi*192:192+fi*192])+1.0j*np.sin(50*x[fi*192:192+fi*192]))

        iq=np.tile(iq,(self.NumSamplesPerLine,1))

        self.iqIn = iq.astype(complex64)
        self.iqIn=self.iqIn.reshape(self.NumSamplesPerLine,self.NumFrames*self.NumLinesPerFrame).T
        plt.figure();plt.plot(self.iqIn.real);plt.plot(self.iqIn.imag);plt.show(False)

        # data re-arrange for multi-frame-testing
        iqInR1 = self.iqIn.reshape(self.NumFrames, self.NumLinesPerFrame, self.NumSamplesPerLine)
        #iqInR2 = np.swapaxes(iqInR1, 1, 2)
        iqInR2 = iqInR1
        #iqInR3 = iqInR2.reshape(self.NumLinesPerFrame * self.NumSamplesPerLine, self.NumFrames)
        iqInR4 = np.copy(iqInR2)
        np.save('pwIn.npy', iqInR4)
        self.iqIn=self.iqIn.reshape(-1)

    def sumSampleSpectral(self):
        self.gatingIn = self.iqIn
        numRangeSamples = self.NumSamplesPerLine
        self.gatingOut = self.gatingIn.reshape(self.NumFrames*self.NumLinesPerFrame, self.NumSamplesPerLine)
        self.gatingOut = self.gatingOut.reshape(-1)
        self.numRangeSamples = numRangeSamples
        self.numLines = self.NumLinesPerFrame

        self.downSampleIn = self.gatingOut

        self.downSampleIn = self.downSampleIn.reshape(-1,self.numRangeSamples)
        self.accWinSpectral=ones(self.numRangeSamples)

        self.accWinSpectral=hamming(self.numRangeSamples)


        self.accWinSpectral=self.accWinSpectral.reshape(-1)
        self.sumSampleSpectralOut=sum(self.downSampleIn,axis=1).reshape(-1)
        np.save('pwSumSampleSpectralOut.npy',self.sumSampleSpectralOut)
        self.spectralActualSampleRateHz = self.AcqSpectralSampleRateHz

    def wallFilterSpectral(self):
        self.wallFilterIn=self.sumSampleSpectralOut
        wfil=butter(self.WallFilterSpectralOrder,self.WallFilterSpectralCutoff,btype='high')
        coeff = firwin(127,self.WallFilterSpectralCutoff,scale=True,pass_zero=False,window=('kaiser',7.1))
        np.save('wallFilterWindow.npy', coeff.astype('float'))
        self.fftWinSize = 84
        self.fftWin=ones((self.fftWinSize,1)).reshape(-1)

        self.fftWin=hanning(self.fftWinSize).reshape(-1)

        self.wallFilterSpectralOut=lfilter(coeff,1,self.wallFilterIn)
        np.save('pwwallFilterSpectralOut.npy',self.wallFilterSpectralOut.astype('complex64'))

    def fft(self):
        #self.fftIn=self.wallFilterSpectralOut
        # insert 0s so that 1 sample in can produce output
        wfOut = self.wallFilterSpectralOut
        frontZeros = np.zeros(self.fftWinSize-1)
        inData = np.insert(wfOut, 0, frontZeros)
        self.fftIn=inData

        #fftSize
        self.fftSize=256

        #Actual FFT calcuation
        self.fftOut_beforeAvg = []
        self.fftOut = []
        pos=0

        #Calculation all FFTs
        indd=0
        self.fftDataShiftFloat=26041/6144
        self.fftDataShiftRemain = 0.0;

        while floor(pos+self.fftWinSize)<=len(self.fftIn):
            fftOut_beforeAvg = 0
            print('self.fftDataShiftFloat,self.fftDataShiftRemain,pos',self.fftDataShiftFloat,self.fftDataShiftRemain,pos)
            fftOut_beforeAvg=pow(abs(fftshift(fft(self.fftWin*self.fftIn[pos:(pos+self.fftWinSize)],self.fftSize)))/self.fftSize,2)
            self.fftOut_beforeAvg.append(fftOut_beforeAvg)

            if ((self.fftDataShiftFloat+self.fftDataShiftRemain) - floor(self.fftDataShiftFloat+self.fftDataShiftRemain))>=0.5:
               self.fftDataShift=ceil(self.fftDataShiftFloat+self.fftDataShiftRemain)
            else:
               self.fftDataShift=floor(self.fftDataShiftFloat+self.fftDataShiftRemain)

            pos=np.int(floor(pos+self.fftDataShift))
            self.fftDataShiftRemain += self.fftDataShiftFloat - self.fftDataShift
            print(pos,(pos+self.fftWinSize))
            indd=indd+1

        #Average FFTs
        pos1 = 0
        while (pos1 + (self.FftAverageNum-1)) <= len(self.fftOut_beforeAvg)-1:
            fftOut = 0
            for n in range(0, self.FftAverageNum):
                fftOut += self.fftOut_beforeAvg[pos1 + n]
            self.fftOut.append(fftOut)
            pos1 +=1

        self.numFftCols = len(self.fftOut)
        self.fftOut = array(self.fftOut).reshape(-1, self.fftSize)
        np.save('pwFftOut.npy',self.fftOut.astype('float32'))

    def postfft(self):

        self.postFftIn=self.fftOut
        interpIn = self.postFftIn
        numFftCols = self.postFftIn.shape[0]
        interpOut=zeros((numFftCols,self.fftSize))
        startIndex = 0
        if self.BaselineShift<0:
            startIndex = np.int(abs(self.BaselineShift)*doppler.fftSize)
        elif self.BaselineShift>0:
            startIndex = self.fftSize-np.int(abs(self.BaselineShift)*self.fftSize)
        displayIndex=np.concatenate([np.arange(startIndex,self.fftSize),np.arange(0,startIndex)])
        #print(displayIndex)
        for index in range(0,self.fftSize):
            interpOut[:,index]=interpIn[:,displayIndex[index]]
        self.fftpostOut=interpOut

    def audio(self):
        # Use the same Wall Filter output for the Spectral and Audio processing
        self.audioIn=self.wallFilterSpectralOut

        self.audioGainFwd=1.0
        self.audioGainRev=self.audioGainFwd
        self.bandwidthFwd = 0.5
        self.bandwidthRev=1-self.bandwidthFwd

        print(self.bandwidthFwd,self.audioGainFwd,self.bandwidthRev,self.audioGainRev)
        cutoffFwd=self.bandwidthFwd
        if self.bandwidthFwd>0.5:
            cutoffFwd=0.5
            self.audioGainFwd*=2.0/(1+2*self.bandwidthFwd)

        cutoffRev=self.bandwidthRev
        if self.bandwidthRev>0.5:
            cutoffRev=0.5
            self.audioGainRev*=2.0/(1+2*self.bandwidthRev)

        print(self.bandwidthFwd,self.audioGainFwd,self.bandwidthRev,self.audioGainRev)

        ## JJ HILBERT BEGIN
        nd=np.int((self.AudioHilbertFirLen-1)/2)
        n=array(range(0,self.AudioHilbertFirLen),dtype=float)-nd
        hn=2*(np.sin(np.pi*n/2)**2)/(np.pi*n)
        #hn=2/(np.pi*n)
        hn[nd]=0
        hq=lfilter(hn,1,self.audioIn.imag)

        ## pre LPF output
        preLPFwd = self.audioIn.real[0:len(hq)]-hq
        preLPRev = self.audioIn.real[0:len(hq)]+hq

        np.save('pwHilbertFwd.npy',preLPFwd.astype('float32'))
        np.save('pwHilbertRev.npy',preLPRev.astype('float32'))
        ##

        lpflen=np.int(1/self.WallFilterAudioCutoff)
        if np.mod(lpflen,2)==0:
            lpflen=lpflen-1
        #h = sci.firwin(lpflen,0.5,window='hanning')
        # filter length
        lpfFilterLength = 128
        upSampleRatio = 2
        lpfCutoff = 0.5/upSampleRatio
        h = sci.firwin(lpfFilterLength,lpfCutoff,window='hanning') #flowphantom setting

        lpfAudioFwd = upfirdn(h, preLPFwd, upSampleRatio)
        lpfAudioRev = upfirdn(h, preLPRev, upSampleRatio)

        np.save('pwHilbertUpLpfFwd.npy',lpfAudioFwd.astype('float32'))
        np.save('pwHilbertUpLpfRev.npy',lpfAudioRev.astype('float32'))

        audioFwd = sci.lfilter(h,1,self.audioIn.real[0:len(hq)]-hq)
        audioRev = sci.lfilter(h,1,self.audioIn.real[0:len(hq)]+hq)
        ## JJ HILBERT END

        #gain
        audioFwd=self.audioGainFwd*audioFwd
        audioRev=self.audioGainRev*audioRev

        #clipping
        audioLeft=where(audioFwd>2**23,2**23,audioFwd)
        self.audioLeftOut=where(audioLeft<=-2**23,-2**23,audioLeft)
        self.audioLeftOut = round(self.audioLeftOut)

        self.audioRevOut = audioRev

        #clipping
        audioRight=where(audioRev>2**23,2**23,audioRev)
        self.audioRightOut=where(audioRight<=-2**23,-2**23,audioRight)
        self.audioRightOut = round(self.audioRightOut)

        # ramp
        numSampleToRamp = int(0.01*self.SampleRateHz)
        for n in range(0, numSampleToRamp):
            self.audioLeftOut[n] =  self.audioLeftOut[n] * float(n)/ numSampleToRamp
            self.audioRightOut[n] =  self.audioRightOut[n] * float(n)/ numSampleToRamp



doppler=pw()
doppler.loadIq()
doppler.sumSampleSpectral()
doppler.wallFilterSpectral()

# audio
doppler.audio()

# fft
doppler.fft()
doppler.postfft()
pwimage(doppler.fftpostOut)
