#include <gtest/gtest.h>

#include "PersistenceProcess.h"
#include "ThorRawReader.h"
#include "ThorRawWriter.h"
#include "CineBuffer.h"


class PersistenceProcessTest : public ::testing::Test
{
protected: // Methods
	void SetUp() override
	{
		ups = std::make_shared<UpsReader>();
		ups->open("assets");
		p.reset(new PersistenceProcess(ups));
	}

	void TearDown() override
	{
		p->terminate();
	}

protected: // Instance data
	std::unique_ptr<PersistenceProcess> p;

	std::shared_ptr<UpsReader> ups;
};

TEST_F(PersistenceProcessTest, Init)
{
	ThorStatus status = p->init();
	EXPECT_FALSE(IS_THOR_ERROR(status));
}

TEST_F(PersistenceProcessTest, SetParams)
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

TEST_F(PersistenceProcessTest, UniformImage)
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

TEST_F(PersistenceProcessTest, Stream)
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
		"b1006_heart",
		//"heart",
		//"liver_30cm",
		//"liver",
		//"moving_phantom_fr",
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
		snprintf(rpath, sizeof(rpath), "results/%s_pfilt.thor", file);
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