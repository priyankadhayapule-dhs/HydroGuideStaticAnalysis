INC=../../libUltrasound/include \
	../../libUltrasound/include/dau \
	../../libUltrasound/include/Rendering \
	../../libUltrasound/include/ImageProcessing \
	../../libThorCommon/include \
	../../libThorHardware/include \
	../../prebuilts/SpeckleReduction/include \
	../../libThorCommon/include/jsoncpp \
	/usr/local/opt/opencv/include

SRCS=coloracqtest.cpp \
	../../libUltrasound/dau/ColorAcq.cpp \
	../../libUltrasound/UpsReader.cpp \
	../../libUltrasound/DBWrappers.cpp \
        ../../libThorCommon/Crc32.cpp \
	../../libThorCommon/jsoncpp/json_value.cpp \
	../../libThorCommon/jsoncpp/json_writer.cpp \
	../../libThorCommon/jsoncpp/json_reader.cpp

DOPACQSRCS=doppleracqtest.cpp \
	../../libUltrasound/dau/DopplerAcq.cpp \
	../../libUltrasound/UpsReader.cpp \
	../../libUltrasound/dau/DauSequenceBuilder.cpp \
	../../libThorHardware/DauHw.cpp \
	../../libUltrasound/DBWrappers.cpp \
    ../../libThorCommon/Crc32.cpp \
	../../libThorCommon/jsoncpp/json_value.cpp \
	../../libThorCommon/jsoncpp/json_writer.cpp \
	../../libThorCommon/jsoncpp/json_reader.cpp

coloracqtest: $(SRCS)
	g++ -std=c++11 -DMACOS -o coloracqtest $(foreach d, $(INC), -I$d) $(SRCS) -lsqlite3

doppleracqtest: $(DOPACQSRCS)
	g++ -std=c++11 -DMACOS -o doppleracqtest $(foreach d, $(INC), -I$d) $(DOPACQSRCS) -lsqlite3
