#include <chrono>
#include <thread>
#include <gtest/gtest.h>

#include "ParallelImageProcessor.h"
#include "AutoGainProcess.h"
#include "SpeckleReductionProcess.h"

// time a function
template <typename Fn, typename ...Args>
std::chrono::microseconds timeit(Fn&& fn, Args&&... args)
{
	auto start = std::chrono::high_resolution_clock::now();
	fn(std::forward<Args>(args)...);
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	return std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
}

// Builds a bmode pipeline processor, and tests how long it takes to process each frame
void Test_BModeProcess(unsigned int rays, unsigned int samples, unsigned int concurrency)
{
	CineBuffer *cb = new CineBuffer();
	ParallelImageProcessor proc(concurrency);

	auto upsReader = std::make_shared<UpsReader>();
	upsReader->open(); // uses defaults

	proc.setCineBuffer(cb);
	// Skipping the deinterleave processor, as that requires a DauInputManager, which in turn requires the full Dau + DMA buffers...

	proc.addSequentialStage<AutoGainProcess>(upsReader);
	proc.addParallelStage<SpeckleReductionProcess>(upsReader);

	proc.init();

	ImageProcess::ProcessParams params{0, DAU_DATA_TYPE_B, samples, rays};
	proc.setProcessParams(params);

	// Assuming 30fps incoming, test with 10 secs of data
	int fps = 30;
	int secsData = 10;
	int frames = fps*secsData;

	cb->setBoundary(0, frames);

	uint8_t frame[MAX_B_FRAME_SIZE];
	for (int i=0; i < MAX_B_FRAME_SIZE; ++i)
	{
		frame[i] = static_cast<uint8_t>(i % 255);
	}

	auto elapsed = timeit([](int frames, ParallelImageProcessor &proc, uint8_t *input) {
		for (int i=0; i != frames; ++i)
		{
			proc.processImageAsync(input, i, DAU_DATA_TYPE_B);
		}
		proc.flush();

	}, frames, std::ref(proc), frame);

	printf("%u\t%u\t%u\t%d\t%llu\n", rays, samples, concurrency, frames, elapsed.count());
	delete cb;
}

/*
TEST(ParallelTimingTest, BMode)
{
	printf("rays\tsamples\tthreads\tframes\tmicrosecs\n");
	Test_BModeProcess(MAX_B_LINES_PER_FRAME, MAX_B_SAMPLES_PER_LINE, 1);
	Test_BModeProcess(MAX_B_LINES_PER_FRAME, MAX_B_SAMPLES_PER_LINE, 4);
	Test_BModeProcess(MAX_B_LINES_PER_FRAME, MAX_B_SAMPLES_PER_LINE, 8);
}
*/

class ParallelImageProcessorTest : public ::testing::TestWithParam<unsigned int>
{
protected: // Methods
	void SetUp() override
	{
		p.reset(new ParallelImageProcessor(GetParam()));
		cb.reset(new CineBuffer);
		ups = std::make_shared<UpsReader>();
		ups->open("assets");

		p->setCineBuffer(cb.get());
	}

	void TearDown() override
	{
		p->terminate();

		// Check for memory leaks or open threads?
	}

protected: // Instance data
	std::unique_ptr<ParallelImageProcessor> p;

	std::unique_ptr<CineBuffer> cb;
	std::shared_ptr<UpsReader> ups;
};

struct ProcessStats
{
	ProcessStats() :
		numInstanciated(0), numInitialized(0), numFramesProcessed(0),
		currentlyProcessing(0), maxConcurrentProcess(0),
		numParamsSet(0), numTerminated(0)
	{}

	std::mutex mutex; // mutex protecting all the below values
	int numInstanciated; // number of instances created
	int numInitialized; // number of times init has been called
	int numFramesProcessed; // number of frames processed
	int currentlyProcessing; // current number of instances in the middle of process()
	int maxConcurrentProcess; // The maximum number of instances processing at once
	int numParamsSet; // total number of times setProcessParams has been called
	int numTerminated; // total number of times terminate has been called
};

// Mock Process used for testing
class MockProcess : public ImageProcess
{
public:
	static const char *name()
	{
		return "MockProcess";
	}

	MockProcess(const std::shared_ptr<UpsReader> &upsReader, ProcessStats &stats, int processTimeMs) :
		ImageProcess(upsReader), mStats(stats), mProcessTime(processTimeMs)
	{
		std::unique_lock<std::mutex> lock(mStats.mutex);
		mStats.numInstanciated++;
	}

	ThorStatus init() override
	{
		std::unique_lock<std::mutex> lock(mStats.mutex);
		mStats.numInitialized++;
		return THOR_OK;
	}

	ThorStatus setProcessParams(ProcessParams &params) override
	{
		std::unique_lock<std::mutex> lock(mStats.mutex);
		mStats.numParamsSet++;
		return THOR_OK;
	}

	ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override
	{
		{
			std::unique_lock<std::mutex> lock(mStats.mutex);
			mStats.currentlyProcessing++;
			if (mStats.currentlyProcessing > mStats.maxConcurrentProcess)
				mStats.maxConcurrentProcess = mStats.currentlyProcessing;
		}

		// sleep for a short amount of time to allow multithreading to occur
		std::this_thread::sleep_for(mProcessTime);
		memcpy(inputPtr, outputPtr, MAX_B_FRAME_SIZE);

		{
			std::unique_lock<std::mutex> lock(mStats.mutex);
			mStats.currentlyProcessing--;
			mStats.numFramesProcessed++;
		}
		return THOR_OK;
	}

	void terminate() override
	{
		std::unique_lock<std::mutex> lock(mStats.mutex);
		mStats.numTerminated++;
	}

	ProcessStats &mStats;
	std::chrono::milliseconds mProcessTime;
};


TEST_P(ParallelImageProcessorTest, Empty)
{
	// This test case is empty to test just the constructor/destructor
}

TEST_P(ParallelImageProcessorTest, InitEmpty)
{
	// Initialize with no stages
	p->init();
}

TEST_P(ParallelImageProcessorTest, SequentialSetup)
{
	// Test if a sequential stage can be added and initialized properly
	ProcessStats stats;
	p->addSequentialStage<MockProcess>(ups, std::ref(stats), 1);
	p->init();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		EXPECT_EQ(1, stats.numInstanciated);
		EXPECT_EQ(1, stats.numInitialized);
		EXPECT_EQ(0, stats.numFramesProcessed);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_EQ(0, stats.maxConcurrentProcess);
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(0, stats.numTerminated);
	}

	p->terminate();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		EXPECT_EQ(1, stats.numInstanciated);
		EXPECT_EQ(1, stats.numInitialized);
		EXPECT_EQ(0, stats.numFramesProcessed);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_EQ(0, stats.maxConcurrentProcess);
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(1, stats.numTerminated);
	}
}

TEST_P(ParallelImageProcessorTest, ParallelSetup)
{
	// Test if a parallel stage can be added and initialized properly
	ProcessStats stats;
	p->addParallelStage<MockProcess>(ups, std::ref(stats), 1);
	p->init();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		// Parallel stage should create no more than the number of worker threads
		EXPECT_GE(GetParam(), stats.numInstanciated);
		EXPECT_LT(0, stats.numInstanciated);
		EXPECT_EQ(stats.numInstanciated, stats.numInitialized);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_EQ(0, stats.maxConcurrentProcess);
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(0, stats.numTerminated);
	}

	p->terminate();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		EXPECT_GE(GetParam(), stats.numInstanciated);
		EXPECT_LT(0, stats.numInstanciated);
		EXPECT_EQ(stats.numInstanciated, stats.numInitialized);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_EQ(0, stats.maxConcurrentProcess);
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(stats.numInstanciated, stats.numTerminated);
	}
}

TEST_P(ParallelImageProcessorTest, SequentialProcess)
{
	// Test if a sequential stage can process a series of frames properly
	ProcessStats stats;
	p->addSequentialStage<MockProcess>(ups, std::ref(stats), 1);
	p->init();

	uint8_t input[MAX_B_FRAME_SIZE];

	for (int frame = 0; frame != 100; ++frame)
	{
		input[0] = frame;
		p->processImageAsync(input, frame, DAU_DATA_TYPE_B);
	}

	p->flush();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		EXPECT_EQ(1, stats.numInstanciated);
		EXPECT_EQ(1, stats.numInitialized);
		EXPECT_EQ(100, stats.numFramesProcessed);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_EQ(1, stats.maxConcurrentProcess);
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(0, stats.numTerminated);
	}

	p->terminate();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		EXPECT_EQ(1, stats.numInstanciated);
		EXPECT_EQ(1, stats.numInitialized);
		EXPECT_EQ(100, stats.numFramesProcessed);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_EQ(1, stats.maxConcurrentProcess);
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(1, stats.numTerminated);
	}
}

TEST_P(ParallelImageProcessorTest, ParallelProcess)
{
	// Test if a parallel stage can process a series of frames properly
	ProcessStats stats;
	p->addParallelStage<MockProcess>(ups, std::ref(stats), 1);
	p->init();

	uint8_t input[MAX_B_FRAME_SIZE];

	for (int frame = 0; frame != 100; ++frame)
	{
		input[0] = frame;
		p->processImageAsync(input, frame, DAU_DATA_TYPE_B);
	}

	p->flush();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		EXPECT_GE(GetParam(), stats.numInstanciated);
		EXPECT_LT(0, stats.numInstanciated);
		EXPECT_EQ(stats.numInstanciated, stats.numInitialized);
		EXPECT_EQ(100, stats.numFramesProcessed);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_GE(GetParam(), stats.maxConcurrentProcess);
		if (GetParam() > 1)
		{
			// If the concurrency is larger than 1, we should have had multiple stages going at once
			EXPECT_LT(1, stats.maxConcurrentProcess);
		}
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(0, stats.numTerminated);
	}

	p->terminate();

	{
		std::unique_lock<std::mutex> lock(stats.mutex);
		EXPECT_GE(GetParam(), stats.numInstanciated);
		EXPECT_LT(0, stats.numInstanciated);
		EXPECT_EQ(stats.numInstanciated, stats.numInitialized);
		EXPECT_EQ(100, stats.numFramesProcessed);
		EXPECT_EQ(0, stats.currentlyProcessing);
		EXPECT_GE(GetParam(), stats.maxConcurrentProcess);
		if (GetParam() > 1)
		{
			// If the concurrency is larger than 1, we should have had multiple stages going at once
			EXPECT_LT(1, stats.maxConcurrentProcess);
		}
		EXPECT_EQ(0, stats.numParamsSet);
		EXPECT_EQ(stats.numInstanciated, stats.numTerminated);
	}
}

TEST_P(ParallelImageProcessorTest, MockPipeline)
{
	// Test a mock processing pipeline similar to B Mode processing
	ProcessStats stats[3];
	// Add a parallel stage (mock deinterlace), a sequential stage (mock autogain) and a parallel stage (mock speckle reduction)
	p->addParallelStage<MockProcess>(ups, std::ref(stats[0]), 1);
	p->addSequentialStage<MockProcess>(ups, std::ref(stats[1]), 1);
	p->addParallelStage<MockProcess>(ups, std::ref(stats[2]), 20);
	p->init();

	uint8_t input[MAX_B_FRAME_SIZE];

	for (int frame = 0; frame != 100; ++frame)
	{
		input[0] = frame;
		p->processImageAsync(input, frame, DAU_DATA_TYPE_B);
	}

	p->flush();

	// Check the first parallel stage stats
	{
		std::unique_lock<std::mutex> lock(stats[0].mutex);
		EXPECT_GE(GetParam(), stats[0].numInstanciated);
		EXPECT_LT(0, stats[0].numInstanciated);
		EXPECT_EQ(stats[0].numInstanciated, stats[0].numInitialized);
		EXPECT_EQ(100, stats[0].numFramesProcessed);
		EXPECT_EQ(0, stats[0].currentlyProcessing);
		EXPECT_GE(GetParam(), stats[0].maxConcurrentProcess);
		if (GetParam() > 1)
		{
			// If the concurrency is larger than 1, we should have had multiple stages going at once
			EXPECT_LT(1, stats[0].maxConcurrentProcess);
		}
		EXPECT_EQ(0, stats[0].numParamsSet);
		EXPECT_EQ(0, stats[0].numTerminated);
	}

	// Second stage is sequential
	{
		std::unique_lock<std::mutex> lock(stats[1].mutex);
		EXPECT_EQ(1, stats[1].numInstanciated);
		EXPECT_EQ(1, stats[1].numInitialized);
		EXPECT_EQ(100, stats[1].numFramesProcessed);
		EXPECT_EQ(0, stats[1].currentlyProcessing);
		EXPECT_EQ(1, stats[1].maxConcurrentProcess);
		EXPECT_EQ(0, stats[1].numParamsSet);
		EXPECT_EQ(0, stats[1].numTerminated);
	}

	// Third stage is parallel
	{
		std::unique_lock<std::mutex> lock(stats[2].mutex);
		EXPECT_GE(GetParam(), stats[2].numInstanciated);
		EXPECT_LT(0, stats[2].numInstanciated);
		EXPECT_EQ(stats[2].numInstanciated, stats[2].numInitialized);
		EXPECT_EQ(100, stats[2].numFramesProcessed);
		EXPECT_EQ(0, stats[2].currentlyProcessing);
		EXPECT_GE(GetParam(), stats[2].maxConcurrentProcess);
		if (GetParam() > 1)
		{
			// If the concurrency is larger than 1, we should have had multiple stages going at once
			EXPECT_LT(1, stats[2].maxConcurrentProcess);
		}
		EXPECT_EQ(0, stats[2].numParamsSet);
		EXPECT_EQ(0, stats[2].numTerminated);
	}

	p->terminate();
}

TEST_P(ParallelImageProcessorTest, ActualClip)
{}

INSTANTIATE_TEST_CASE_P(Instance0, ParallelImageProcessorTest, ::testing::Values(1,2,4,8,16));