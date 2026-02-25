#include <gtest/gtest.h>

#include "BSmoothProcess.h"
#include "ThorRawReader.h"
#include "ThorRawWriter.h"
#include "CineBuffer.h"


class BSmoothProcessTest : public ::testing::Test
{
protected: // Methods
	void SetUp() override
	{
		ups = std::make_shared<UpsReader>();
		ups->open("assets");
		p.reset(new BSmoothProcess(ups));
	}

	void TearDown() override
	{
		p->terminate();
	}

protected: // Instance data
	std::unique_ptr<BSmoothProcess> p;

	std::shared_ptr<UpsReader> ups;
};

TEST_F(BSmoothProcessTest, Init)
{
	ThorStatus status = p->init();
	EXPECT_FALSE(IS_THOR_ERROR(status));
}

TEST_F(BSmoothProcessTest, SetParams)
{
	ThorStatus status = p->init();
	ASSERT_FALSE(IS_THOR_ERROR(status));

	ImageProcess::ProcessParams params;

	params = ImageProcess::ProcessParams{1, IMAGING_MODE_B, MAX_B_SAMPLES_PER_LINE, MAX_B_LINES_PER_FRAME};
	status = p->setProcessParams(params);
	EXPECT_FALSE(IS_THOR_ERROR(status));

	params = ImageProcess::ProcessParams{2, IMAGING_MODE_B, 512, 224};
	status = p->setProcessParams(params);
	EXPECT_FALSE(IS_THOR_ERROR(status));

	params = ImageProcess::ProcessParams{3, IMAGING_MODE_B, 256, 128};
	status = p->setProcessParams(params);
	EXPECT_FALSE(IS_THOR_ERROR(status));

	params = ImageProcess::ProcessParams{4, IMAGING_MODE_B, 128, 64};
	status = p->setProcessParams(params);
	EXPECT_FALSE(IS_THOR_ERROR(status));
}

TEST_F(BSmoothProcessTest, DISABLED_UniformImage)
{
	ThorStatus status = p->init();
	ASSERT_FALSE(IS_THOR_ERROR(status));

	ImageProcess::ProcessParams params = ImageProcess::ProcessParams{1, IMAGING_MODE_B, MAX_B_SAMPLES_PER_LINE, MAX_B_LINES_PER_FRAME};
	status = p->setProcessParams(params);
	ASSERT_FALSE(IS_THOR_ERROR(status));

	// uniform grey input
	std::vector<uint8_t> input(MAX_B_SAMPLES_PER_LINE*MAX_B_LINES_PER_FRAME, 128u);
	std::vector<uint8_t> orig_input = input; // copy to ensure the original is unchanged
	std::vector<uint8_t> output(MAX_B_SAMPLES_PER_LINE*MAX_B_LINES_PER_FRAME, 0u);

	for (int i=0; i < 20; ++i)
	{
		status = p->process(&input.front(), &output.front());
		EXPECT_FALSE(IS_THOR_ERROR(status));

		// Two vectors should be idential
		EXPECT_EQ(orig_input, input);
		EXPECT_EQ(orig_input, output);
	}
}

TEST_F(BSmoothProcessTest, SingleRow)
{
	ThorStatus status = p->init();
	ASSERT_FALSE(IS_THOR_ERROR(status));

	uint8_t input[] = {
   		127, 138, 130, 121, 132, 144, 135, 126, 137, 149, 140, 131, 142,
       154, 145, 136, 147, 159, 150, 141, 152, 164, 155, 146, 157, 169,
       160, 151, 162, 174, 165, 156, 167, 178, 169, 160, 172, 183, 174,
       165, 176, 187, 178, 169, 181, 192, 183, 174, 185, 196, 187, 178,
       189, 200, 191, 182, 193, 204, 195, 186, 197, 208, 199, 189, 200,
       212, 202, 193, 204, 215, 206, 196, 207, 218, 209, 199, 210, 221,
       212, 202, 213, 224, 214, 205, 216, 227, 217, 207, 218, 229, 219,
       210, 220, 231, 221, 212, 222, 233, 223, 213, 224, 235, 225, 215,
       225, 236, 226, 216, 227, 237, 227, 217, 228, 238, 228, 218, 228,
       239, 229, 219, 229, 239, 229, 219, 229, 239, 229, 219, 229, 239,
       229, 219, 229, 239, 228, 218, 228, 238, 228, 217, 227, 237, 227,
       216, 226, 236, 225, 215, 225, 235, 224, 213, 223, 233, 222, 212,
       221, 231, 220, 210, 219, 229, 218, 207, 217, 227, 216, 205, 214,
       224, 213, 202, 212, 221, 210, 199, 209, 218, 207, 196, 206, 215,
       204, 193, 202, 212, 200, 189, 199, 208, 197, 186, 195, 204, 193,
       182, 191, 200, 189, 178, 187, 196, 185, 174, 183, 192, 181, 169,
       178, 187, 176, 165, 174, 183, 172, 160, 169, 178, 167, 156, 165,
       174, 162, 151, 160, 169, 157, 146, 155, 164, 152, 141, 150, 159,
       147, 136, 145, 154, 142, 131, 140, 149, 137, 126, 135, 144, 132,
       121, 130, 138, 127, 116, 124, 133, 122, 110, 119, 128, 117, 105,
       114, 123, 112, 100, 109, 118, 107,  95, 104, 113, 102,  90,  99,
       108,  97,  85,  94, 103,  92,  80,  89,  98,  87,  76,  85,  94,
        82,  71,  80,  89,  78,  67,  76,  85,  73,  62,  71,  80,  69,
        58,  67,  76,  65,  54,  63,  72,  61,  50,  59,  68,  57,  46,
        55,  65,  54,  42,  52,  61,  50,  39,  48,  58,  47,  36,  45,
        55,  44,  33,  42,  52,  41,  30,  40,  49,  38,  27,  37,  47,
        36,  25,  35,  44,  34,  23,  33,  42,  32,  21,  31,  41,  30,
        19,  29,  39,  29,  18,  28,  38,  27,  17,  27,  37,  26,  16,
        26,  36,  26,  15,  25,  35,  25,  15,  25,  35,  25,  15,  25,
        35,  25,  15,  25,  35,  25,  15,  26,  36,  26,  16,  26,  37,
        27,  17,  27,  38,  28,  18,  29,  39,  29,  19,  30,  41,  31,
        21,  32,  42,  33,  23,  34,  44,  35,  25,  36,  47,  37,  27,
        38,  49,  40,  30,  41,  52,  42,  33,  44,  55,  45,  36,  47,
        58,  48,  39,  50,  61,  52,  42,  54,  65,  55,  46,  57,  68,
        59,  50,  61,  72,  63,  54,  65,  76,  67,  58,  69,  80,  71,
        62,  73,  85,  76,  67,  78,  89,  80,  71,  82,  94,  85,  76,
        87,  98,  89,  80,  92, 103,  94,  85,  97, 108,  99,  90, 102,
       113, 104,  95, 107, 118, 109, 100, 112, 123, 114, 105, 117, 128,
       119, 110, 122, 133, 124, 116
	};

	uint8_t expected[] = {
		127, 127, 129, 130, 131, 133, 134, 135, 136, 138, 139, 140, 141,
       143, 144, 145, 146, 148, 149, 150, 151, 153, 154, 155, 156, 158,
       160, 160, 161, 163, 165, 165, 166, 167, 168, 169, 171, 172, 174,
       174, 175, 176, 178, 178, 180, 181, 182, 183, 184, 185, 186, 187,
       188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 198, 199,
       201, 201, 202, 203, 204, 205, 205, 206, 207, 208, 208, 209, 210,
       212, 211, 212, 213, 214, 214, 215, 216, 217, 216, 217, 218, 219,
       219, 219, 220, 221, 221, 221, 222, 223, 222, 223, 224, 225, 224,
       224, 225, 226, 225, 226, 226, 227, 226, 227, 227, 227, 227, 227,
       228, 229, 228, 228, 228, 229, 228, 228, 228, 229, 228, 228, 228,
       229, 228, 228, 228, 227, 227, 227, 227, 227, 226, 226, 226, 227,
       225, 225, 225, 225, 224, 224, 224, 223, 222, 222, 222, 221, 221,
       220, 220, 219, 219, 218, 218, 217, 216, 216, 216, 215, 214, 213,
       213, 213, 211, 210, 210, 210, 208, 208, 207, 206, 205, 205, 204,
       203, 202, 201, 202, 199, 198, 198, 198, 196, 195, 194, 194, 192,
       191, 190, 190, 188, 187, 186, 186, 184, 183, 182, 182, 180, 178,
       177, 177, 175, 174, 173, 173, 171, 169, 168, 168, 166, 165, 164,
       164, 161, 160, 159, 159, 156, 155, 154, 154, 151, 150, 149, 149,
       146, 145, 144, 144, 141, 140, 139, 139, 136, 135, 134, 134, 131,
       130, 129, 128, 126, 125, 123, 123, 121, 119, 118, 118, 116, 114,
       113, 113, 111, 109, 108, 108, 106, 104, 103, 103, 101,  99,  98,
        98,  96,  94,  93,  93,  91,  89,  89,  88,  86,  85,  85,  84,
        81,  80,  79,  79,  77,  76,  75,  75,  72,  71,  70,  70,  68,
        67,  66,  66,  64,  63,  62,  62,  60,  59,  58,  58,  56,  55,
        54,  54,  53,  51,  52,  50,  49,  48,  48,  47,  46,  45,  45,
        44,  43,  42,  42,  41,  40,  39,  40,  38,  37,  36,  37,  36,
        35,  34,  35,  33,  33,  32,  33,  31,  31,  30,  31,  30,  29,
        28,  29,  28,  28,  27,  28,  27,  26,  26,  27,  26,  25,  25,
        25,  25,  25,  24,  25,  24,  24,  24,  25,  24,  24,  24,  25,
        24,  24,  24,  25,  24,  24,  24,  25,  25,  25,  25,  25,  26,
        26,  26,  27,  27,  27,  27,  29,  28,  28,  28,  29,  30,  30,
        30,  31,  31,  32,  32,  33,  33,  34,  34,  35,  36,  36,  36,
        37,  38,  39,  39,  41,  41,  41,  42,  43,  44,  44,  45,  46,
        47,  47,  48,  49,  50,  51,  51,  53,  54,  54,  55,  56,  57,
        58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
        71,  72,  74,  75,  76,  77,  78,  79,  80,  81,  83,  84,  85,
        86,  87,  87,  89,  91,  92,  92,  94,  96,  97,  98,  99, 101,
       102, 103, 104, 106, 107, 108, 109, 111, 112, 113, 114, 116, 117,
       118, 119, 121, 122, 123, 125
	};

	ImageProcess::ProcessParams params;
	params.imagingCaseId = 0;
	params.imagingMode = IMAGING_MODE_B;
	params.numSamples = 1;
	params.numRays = 500;

	uint8_t result[500];
	status = p->setProcessParams(params);
	EXPECT_FALSE(IS_THOR_ERROR(status));

	status = p->process(input, result);
	EXPECT_FALSE(IS_THOR_ERROR(status));

	// for (int i=0; i < 500; ++i)
	// {
	// 	if (i % 13 == 0)
	// 		printf("\n");
	// 	printf("%hhu ", result[i]);
	// }

	for (unsigned int i=0; i < 500; ++i)
	{
		uint8_t diff = expected[i] > result[i] ? expected[i]-result[i] : result[i]-expected[i];
		EXPECT_GE(1, diff) << "expected[" << i << "] = " << static_cast<uint32_t>(expected[i]) << " vs result[" << i << "] = " << static_cast<uint32_t>(result[i]);
	}

	//EXPECT_TRUE(std::equal(std::begin(expected), std::end(expected), std::begin(result)));
}

TEST_F(BSmoothProcessTest, Stream)
{
	ThorStatus status = p->init();
	ASSERT_FALSE(IS_THOR_ERROR(status));

	ImageProcess::ProcessParams params = ImageProcess::ProcessParams{1, IMAGING_MODE_B, MAX_B_SAMPLES_PER_LINE, MAX_B_LINES_PER_FRAME};
	status = p->setProcessParams(params);
	ASSERT_FALSE(IS_THOR_ERROR(status));

	// 
	const char *files[] = {
		//"ab_fr",
		//"b1005_kidney",
		//"b1005_liver",
		//"b1006_heart",
		//"heart",
		//"liver_30cm",
		//"liver",
		"moving_phantom_fr",
		//"speckle_pers_lat",
		//"speckle",
		//"testclip_pers_lat",
		//"testclip"
	};

	for (const char *file : files)
	{
		ThorRawReader reader;
		ThorRawDataFile::Metadata meta;

		char apath[255];
		snprintf(apath, sizeof(apath), "assets/%s.thor", file);
		status = reader.open(apath, meta);
		ASSERT_FALSE(IS_THOR_ERROR(status));

		char rpath[255];
		snprintf(rpath, sizeof(rpath), "results/%s_notch.thor", file);
		ThorRawWriter writer;
		status = writer.create(rpath, meta);
		ASSERT_FALSE(IS_THOR_ERROR(status));

		uint8_t *inputPtr = reader.getDataPtr();
		ASSERT_NE(nullptr, inputPtr);
		uint8_t *outputPtr = writer.getDataPtr(meta.frameCount*
			(sizeof(CineBuffer::CineFrameHeader) + MAX_B_FRAME_SIZE));
		ASSERT_NE(nullptr, outputPtr);
		for (int frame=0; frame < meta.frameCount; ++frame)
		{
			// copy CineFrameHeader
			std::copy(inputPtr, inputPtr+sizeof(CineBuffer::CineFrameHeader), outputPtr);

			inputPtr += sizeof(CineBuffer::CineFrameHeader); // advance past header
			outputPtr += sizeof(CineBuffer::CineFrameHeader);
			status = p->process(inputPtr, outputPtr);
			EXPECT_FALSE(IS_THOR_ERROR(status));

			inputPtr += MAX_B_FRAME_SIZE;
			outputPtr += MAX_B_FRAME_SIZE;
		}
		reader.close();
		writer.close();
	}
}