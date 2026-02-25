# Copyright 2020 Echonous Inc
@0xc05542744f0a8763;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("echonous::capnp");

# Top level Thor Clip structure, represents a replacement for
# the binary .thor raw file
struct ThorClip {
	timestamp @0 :UInt64;
	probeType @1 :UInt8;

	# Could get the implied number of frames from the size of
	# the CineData list, but the interface is nicer if it's here
	numFrames @2 :UInt32;

	# Single top level CineParams instance
	# We could try to determine which parts of CineParams go with
	# which data types, but the entire CineParams data structure is
	# less than 200 bytes. Therefore, it seems better to keep our
	# on disk representation close to our in memory representation,
	# which is a single instance of cineParams.
	cineParams @3 :CineParams;

	# Mode specific data sections
	bmode @4 :CineData;
	cmode @5 :CineData;
	mmode @6 :ScrollData;
	da @7 :ScrollData;
	ecg @8 :ScrollData;

	pwmode @9 :ScrollData;
	cwmode @10 :ScrollData;
	pwaudio @11 :ScrollData;
	cwaudio @12 :ScrollData;

	transitionframe @13 :ScrollData;

	dopplerpeakmean @14 :ScrollData;
	guidance @15 :Guidance;
    autolabel @16 :Autolabel;
    fastexam @17 :FastExam;
}

# CineParams representation, taken directly from CineBuffer.h
# Only difference is the ups is simply a version number, since
# we are not going to store the entire UPS in this file
struct CineParams {
	bmodeParams @0 :ScanConverterParams;
	cmodeParams @1 :ScanConverterParams;
	imagingCaseId @2 :UInt32;
	organId @3 :UInt32;
	dauDataTypes @4 :UInt32;
	imagingMode @5 :UInt32;
	frameInterval @6 :UInt32;
	frameRate @7 :Float32;
	upsVersionMajor @8 :UInt32;
	upsVersionMinor @9 :UInt32;
	scrollSpeed @10 :Float32;
	targetFrameRate @11 :UInt32;
	mlinesPerFrame @12 :UInt32;
	stillTimeShift @13 :Float32;
	stillMLineTime @14 :Float32;

	colorMapIndex @15 :UInt32;
	isColorMapInverted @16 :Bool;

	ecgSamplesPerFrame @17 :UInt32;
	ecgSamplesPerSecond @18 :Float32;
	daEcgScrollSpeedIndex @19 :UInt32 = 1;

	daSamplesPerFrame @20 :UInt32;
	daSamplesPerSecond @21 :Float32;

	ecgSingleFrameWidth @22 :Int32;
	ecgTraceIndex @23 :Int32;

	daSingleFrameWidth @24 :Int32;
	daTraceIndex @25 :Int32;

	isEcgExternal @26 :Bool;
	ecgLeadNumExternal @27 :UInt32;

	mSingleFrameWidth @28 :UInt32;

	pwPrfIndex @29 :UInt32;
	pwGateIndex @30 :UInt32;

	cwPrfIndex @31 :UInt32;
	cwDecimationFactor @32 :UInt32;

	dopplerBaselineShiftIndex @33 :UInt32;
	dopplerBaselineInvert @34 :Bool;
	dopplerSamplesPerFrame @35 :UInt32;
	dopplerFFTSize @36 :UInt32;
	dopplerNumLinesPerFrame @37 :UInt32;
	dopplerNumLinesPerFrameFloat @56 :Float32 = 0.0;
	dopplerSweepSpeedIndex @38 :UInt32;
	dopplerFFTNumLinesToAvg @39 :UInt32;
	dopplerAudioUpsampleRatio @40 :UInt32;
	dopplerVelocityScale @41 :Float32;
	dopplerFftSmoothingNum @42 :UInt32;

	colorBThreshold @43 :Int32;
	colorVelThreshold @44 :Int32;
	isTransitionRenderingReady @45 :Bool;
	transitionImagingMode @46 :Int32;
	transitionAltImagingMode @47 :Int32;
	renderedTransitionImgMode @48 :Int32;

	zoomPanParams @49 :ZoomPanParams;
	traceIndex @50 :Int32;

	dopplerWFGain @51 :Float32 = 15.0;

	dopplerPeakDraw @52 :Bool = false;
	dopplerMeanDraw @53 :Bool = false;
	dopplerPeakMax  @54 :Float32 = 0.0;

	# Default Probe Type is 0x3 which is the first released probe or Torso-3.
	# Torso-1 uses the same UPS as Torso-3 so should be OK to default to
	# this value.
	probeType @55 :UInt32 = 3;

    isPWTDI @57 :Bool = false;

    mModeSweepSpeedIndex @58 :Int32 = -1;

    colorDopplerMode @59 :Int32 = 0;
}

# ScanConverterParams, exactly as in native code
struct ScanConverterParams {
	numSampleMesh @0 :UInt32;
	numRayMesh @1 :UInt32;
	numSamples @2 :UInt32;
	numRays @3 :UInt32;
	startSampleMm @4 :Float32;
	sampleSpacingMm @5 :Float32;
	raySpacing @6 :Float32;
	startRayRadian @7 :Float32;

	# Additional params for Linear
	probeShape @8 :UInt32 = 0;
	startRayMm @9 :Float32 = 0.0;
	steeringAngleRad @10 :Float32 = 0.0;
}

# Zoom Pan Params - fixed float array
struct ZoomPanParams {
    zoomPanParams0 @0 : Float32;
    zoomPanParams1 @1 : Float32;
    zoomPanParams2 @2 : Float32;
    zoomPanParams3 @3 : Float32;
    zoomPanParams4 @4 : Float32;
}

# Notice: this is a structure so that we could change this
# to be some more compressed representation. If instead we put
# a List(CineFrame) directly in the ThorClip structure, then
# we could also be stuck as representing the clip data as a
# list of frames (although we could change how each frame is
# represented).
#
# The tradeoff is a single extra pointer per CineData struct
struct CineData {
	rawFrames @0 :List(CineFrame);
}

# Scrolling data is either a list of frames/samples from the
# cinebuffer, or a single texture used in a screenshot
struct ScrollData {
	union {
		texture @0 :Data;
		rawFrames @1 :List(CineFrame);
	}
}

# A single frame from the CineBuffer
# Frame Header information is stored here as well,
# and may be extended if desired.
struct CineFrame {
	header :group {
		frameNum @0 :UInt32; 
		timeStamp @1 :UInt64;
		numSamples @2 :UInt32;
		heartRate @3 :Float32;
		statusCode @4 :UInt32;
	}
	frame @5 :Data;
}

struct Guidance {
    modelVersion @0 :Text;
    targetView   @1 :View;
    predictions  @2 :List(Prediction);

    struct Prediction {
        # Quality score normally ranges from 1-5. A score of 0 means no information for that frame
        qualityScore @0 :UInt8;
        subview @1 :Subview;
    }

    enum View {
        noInformation @0; # Make sure no info is index 0 so it is the default
        a4c @1;
        a2c @2;
        a5c @3;
        plax @4;
    }

    # Note: The numeric value of these subviews does NOT match the numeric value from the ML model
    enum Subview {
        noInformation @0; # Make sure no info is index 0 so it is the default
        a4c @1;
        tiltDown @2;
        tiltUp @3;
        counterClk @4;
        clockwise @5;
        angleLateral @6;
        angleMedial @7;
        ribSpaceUp @8;
        slideMedial @9;
        slideLateral @10;
        a2c @11;
        noise @12;
        a3c @13;
        tiltDownA2c @14;
        tiltUpA2c @15;
        angleMedialA2c @16;
        angleLateralA2c @17;
        uncertain @18;
        flippedA4c @19;

        # PLAX Subviews
        plaxApical @20;
        plaxPsaxAv @21;
        plaxPsaxMv @22;
        plaxPsaxPm @23;
        plaxPsaxAp @24;
        plaxOther @25;
        plaxPlaxBest @26;
        plaxPlaxAlat @27;
        plaxPlaxAmed @28;
        plaxPlaxSlat @29;
        plaxPlaxSmed @30;
        plaxPlaxSup @31;
        plaxPlaxSdn @32;
        plaxPlaxClk @33;
        plaxPlaxCtr @34;
        plaxPlaxTdn @35;
        plaxRvit @36;
        plaxPlaxTup @37;
        plaxRvot @38;
        plaxPlaxUnsure @39;
        plaxFlip @40;
    }
}

struct Autolabel {
    modelVersion @0 :Text;
    predictions  @1 :List(LabeledFrame);

    struct LabeledFrame {
        frameNum          @0 :UInt32;
        realLabels        @2 :List(Label); # Physical coordinates, before smoothing
        realLabelsSmooth  @1 :List(Label); # Physical coordinates, after smoothing
        pixelLabels       @3 :List(Label); # Fractional pixel coordinates, before smoothing
        pixelLabelsSmooth @4 :List(Label); # Fractional pixel coordinates, after smoothing

        viewIndex @5 :UInt32;
        viewName @6 :Text;
        viewConf @7 :Float32;
    }

    struct Label {
        # Positions may be in physical coordinates or fractional pixel coordinates
        x @0 :Float32;
        y @1 :Float32;
        w @2 :Float32;
        h @3 :Float32;
        labelId @4 :Text;
        displayText @5 :Text;
        conf @6 :Float32;
    }
}

struct FastExam {
    modelVersion @0 :Text;
    frames @1 :List(LabeledFrame);

    struct LabeledFrame {
        frameNum @0 :UInt32;
        rawLabelsPhysical @1 :List(Label);
        rawLabelsNormalized @2 :List(Label);
        smoothLabelsPhysical @3 :List(Label);
        smoothLabelsNormalized @4 :List(Label);
        viewIndex @5 :UInt32;
        viewName @6 :Text;
        viewConf @7 :Float32;
    }
    struct Label {
        x @0 :Float32;
        y @1 :Float32;
        w @2 :Float32;
        h @3 :Float32;
        labelId @4 :Text;
        displayText @5 :Text;
        conf @6 :Float32;
    }
}