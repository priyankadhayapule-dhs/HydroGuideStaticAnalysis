INC=../../libUltrasound/include \
	../../libUltrasound/include/dau \
	../../libUltrasound/include/Rendering \
	../../libUltrasound/include/ImageProcessing \
	../../libThorCommon/include \
	../../libThorHardware/include \
	../../prebuilts/SpeckleReduction/include \
	../../libThorCommon/include/jsoncpp \
	/usr/local/opt/opencv/include

SRCS=daprocesstest.cpp \
    ../../libUltrasound/ImageProcessing/DaProcess.cpp \
    ../../libUltrasound/CineBuffer.cpp \
    ../../libUltrasound/UpsReader.cpp \
    ../../libUltrasound/DBWrappers.cpp \
    ../../libThorCommon/CriticalSection.cpp \
    ../../libThorCommon/Crc32.cpp \
    ../../libThorCommon/ThorUtils.cpp \
	../../libThorHardware/PciDauMemory.cpp

SRCS2=ecgprocesstest.cpp \
    ../../libUltrasound/ImageProcessing/EcgProcess.cpp \
    ../../libUltrasound/CineBuffer.cpp \
    ../../libUltrasound/UpsReader.cpp \
    ../../libUltrasound/DBWrappers.cpp \
    ../../libThorCommon/CriticalSection.cpp \
    ../../libThorCommon/Crc32.cpp \
    ../../libThorCommon/ThorUtils.cpp

SRCS3=ecgprocesshrtest.cpp \
    ../../libUltrasound/ImageProcessing/EcgProcess.cpp \
    ../../libUltrasound/CineBuffer.cpp \
    ../../libUltrasound/UpsReader.cpp \
    ../../libUltrasound/DBWrappers.cpp \
    ../../libThorCommon/CriticalSection.cpp \
    ../../libThorCommon/Crc32.cpp \
    ../../libThorCommon/ThorUtils.cpp

SRCS4=pwprocesstest.cpp \
    ../../libUltrasound/ImageProcessing/PwLowPassDecimationFilterProcess.cpp \
    ../../libUltrasound/ImageProcessing/DopplerResamplingBuffer.cpp \
    ../../libUltrasound/ImageProcessing/DopplerSpectralProcess.cpp \
    ../../libUltrasound/ImageProcessing/DopplerWallFilterProcess.cpp \
    ../../libUltrasound/ImageProcessing/DopplerHilbertProcess.cpp \
    ../../libUltrasound/ImageProcessing/DopplerAudioUpsampleProcess.cpp \
    ../../libUltrasound/CineBuffer.cpp \
    ../../libUltrasound/UpsReader.cpp \
    ../../libUltrasound/DBWrappers.cpp \
    ../../libThorCommon/CriticalSection.cpp \
    ../../libThorCommon/Crc32.cpp \
    ../../libThorCommon/ThorUtils.cpp \
		../../libThorCommon/pffft.c \
    cnpy.cpp

all: daprocesstest ecgprocesstest ecgprocesshrtest pwprocesstest

daprocesstest: $(SRCS)
	g++ -std=c++11 -DMACOS -o daprocesstest $(foreach d, $(INC), -I$d) $(SRCS) -lsqlite3

ecgprocesstest: $(SRCS2)
	g++ -std=c++11 -DMACOS -o ecgprocesstest $(foreach d, $(INC), -I$d) $(SRCS2) -lsqlite3

ecgprocesshrtest: $(SRCS3)
	g++ -std=c++11 -DMACOS -o ecgprocesshrtest $(foreach d, $(INC), -I$d) $(SRCS3) -lsqlite3

pwprocesstest: $(SRCS4)
	g++ -std=c++11 -DMACOS -o pwprocesstest $(foreach d, $(INC), -I$d) $(SRCS4) -lsqlite3 -lz
