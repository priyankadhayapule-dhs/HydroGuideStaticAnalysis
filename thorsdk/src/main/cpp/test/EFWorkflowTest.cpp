#include <gtest/gtest.h>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include <EFWorkflow.h>
#include <CinePlayer.h>
#include <AIManager.h>

struct CallbackEvent
{
	enum {
		FrameID,
		Segmentation,
		Statistics
	} type;
	EFWorkflow *workflow;
	CardiacView view;
	int frame;
};

struct CallbackData
{
	std::mutex mutex;
	std::condition_variable cv; // single cv to notify of any changes

	// records of how received callback events
	std::vector<CallbackEvent> records;
};

static void FrameIDCallback(void *user, EFWorkflow *workflow, CardiacView view)
{
	auto *data = static_cast<CallbackData*>(user);

	CallbackEvent event = {CallbackEvent::FrameID, workflow, view, 0};

	std::unique_lock<std::mutex> lock(data->mutex);
	data->records.push_back(event);
	data->cv.notify_all();
}

static void SegmentationCallback(void *user, EFWorkflow *workflow, CardiacView view, int frame)
{
	auto *data = static_cast<CallbackData*>(user);

	CallbackEvent event = {CallbackEvent::Segmentation, workflow, view, frame};

	std::unique_lock<std::mutex> lock{data->mutex};
	data->records.push_back(event);
	data->cv.notify_all();
}


static void StatsCallback(void *user, EFWorkflow *workflow)
{
	auto *data = static_cast<CallbackData*>(user);

	CallbackEvent event = {CallbackEvent::Statistics, workflow, CardiacView::A4C, 0};

	std::unique_lock<std::mutex> lock(data->mutex);
	data->records.push_back(event);
	data->cv.notify_all();
}

/*

TEST(EFWorkflow, AIFrameID)
{
	// Test frame id process
	auto workflow = EFWorkflow::GetGlobalWorkflow();
	workflow->reset();
	CallbackData data;
	AIManager manager;

	// Open CineBuffer, CinePlayer, and UpsReader, then open raw file
	auto ups = std::make_shared<UpsReader>();
	ASSERT_TRUE(ups->open("assets/thor.db"));
	auto cb = std::unique_ptr<CineBuffer>(new CineBuffer);
	auto player = std::unique_ptr<CinePlayer>(new CinePlayer(*cb));
	ASSERT_EQ(THOR_OK, player->init(ups));
	ASSERT_EQ(THOR_OK, player->openRawFile("assets/heart.thor"));

	//cb->setBoundary(0, 100);

	manager.setCineBuffer(cb.get());

	workflow->setFrameIDCallback(&FrameIDCallback);
	auto task = workflow->createFrameIDTask(&data, CardiacView::A4C);
	manager.push(std::move(task));

	{
		std::unique_lock<std::mutex> lock{data.mutex};
		if (data.records.empty())
		{
			// should be complete in less than 10 seconds (probably much less)
			data.cv.wait_for(lock, std::chrono::seconds(10));
		}
		ASSERT_EQ(1, data.records.size()); // use assert to fail instantly if false
		EXPECT_EQ(CallbackEvent::FrameID, data.records[0].type);
		EXPECT_EQ(workflow, data.records[0].workflow);
		EXPECT_EQ(CardiacView::A4C, data.records[0].view);
	}
	auto frameID = workflow->getAIFrames(CardiacView::A4C);
	EXPECT_NE(-1, frameID.esFrame);
	EXPECT_NE(-1, frameID.edFrame);

	return;

	// Load A2C data here

	task = workflow->createFrameIDTask(&data, CardiacView::A2C);
	manager.push(std::move(task));

	{
		std::unique_lock<std::mutex> lock{data.mutex};
		if (data.records.size() == 1)
		{
			// should be complete in less than 10 seconds (probably much less)
			data.cv.wait_for(lock, std::chrono::seconds(10));
		}
		ASSERT_EQ(2, data.records.size()); // use assert to fail instantly if false
		EXPECT_EQ(CallbackEvent::FrameID, data.records[1].type);
		EXPECT_EQ(workflow, data.records[1].workflow);
		EXPECT_EQ(CardiacView::A2C, data.records[1].view);
	}
	frameID = workflow->getAIFrames(CardiacView::A2C);
	EXPECT_NE(-1, frameID.esFrame);
	EXPECT_NE(-1, frameID.edFrame);
}


TEST(EFWorkflow, AISegmentation)
{
	auto workflow = EFWorkflow::GetGlobalWorkflow();
	workflow->reset();
	CallbackData data;
	AIManager manager;

	// Open CineBuffer, CinePlayer, and UpsReader, then open raw file
	auto ups = std::make_shared<UpsReader>();
	ASSERT_TRUE(ups->open("assets/thor.db"));
	auto cb = std::unique_ptr<CineBuffer>(new CineBuffer);
	auto player = std::unique_ptr<CinePlayer>(new CinePlayer(*cb));
	ASSERT_EQ(THOR_OK, player->init(ups));
	ASSERT_EQ(THOR_OK, player->openRawFile("assets/heart.thor"));

	//cb->setBoundary(0, 100);

	manager.setCineBuffer(cb.get());

	workflow->setSegmentationCallback(&SegmentationCallback);
	auto task = workflow->createSegmentationTask(&data, CardiacView::A4C, 40);
	manager.push(std::move(task));
	{
		std::unique_lock<std::mutex> lock{data.mutex};
		if (data.records.empty())
			data.cv.wait_for(lock, std::chrono::seconds(10));
		ASSERT_EQ(1, data.records.size());
		EXPECT_EQ(CallbackEvent::Segmentation, data.records[0].type);
		EXPECT_EQ(workflow, data.records[0].workflow);
		EXPECT_EQ(CardiacView::A4C, data.records[0].view);
		EXPECT_EQ(40, data.records[0].frame);
	}

}

/*

TEST(EFWorkflow, StatisticsDirect)
{
	// Test statistics (directly, not using threads)

	auto workflow = EFWorkflow::GetGlobalWorkflow();
	workflow->reset();
	CallbackData data;
	AIManager manager;

	// dummy user segmentations
	CardiacSegmentation segSystole;
	CardiacSegmentation segDiastole;

	// Generate some points in a semicircle (actual volume is 2/3*pi*r^3)
	float pi = 3.14159265f;
	float totalAngle = 1.0f*pi;

	int numPoints = 13;
	for (int i=0; i < numPoints; ++i)
	{
		float theta = -static_cast<float>(i)/(numPoints-1) * totalAngle + pi/2.0f - (2.0f*pi - totalAngle) / 2.0f;
		vec2 p{1.0f*std::cos(theta), 1.0f*std::sin(theta)};
		segSystole.coords.push_back(p*0.7f); // systole is scaled so that r = 0.7
		segDiastole.coords.push_back(p);
	}

	workflow->setUserFrames(CardiacView::A4C, {1,2});
	workflow->setUserSegmentation(CardiacView::A4C, 1, segSystole);
	workflow->setUserSegmentation(CardiacView::A4C, 2, segDiastole);

	workflow->computeStats();
	auto stats = workflow->getStatistics();

	// All AI driven results should be 0, as we have not run any AI algorithms
	EXPECT_NEAR(0.0f, stats.heartRate, 0.00001f);
	EXPECT_NEAR(0.0f, stats.originalEF, 0.00001f);
	EXPECT_NEAR(0.0f, stats.originalESVolume, 0.00001f);
	EXPECT_NEAR(0.0f, stats.originalEDVolume, 0.00001f);
	EXPECT_NEAR(0.0f, stats.originalStrokeVolume, 0.00001f);
	EXPECT_NEAR(0.0f, stats.originalCardiacOutput, 0.00001f);

	// "perfect" volumes for each
	float sysVolume = 2.0f/3.0f * pi * 0.7f*0.7f*0.7f;
	float diasVolume = 2.0f/3.0f * pi;

	EXPECT_LT(0.0f, stats.editedEF);
	EXPECT_NEAR(sysVolume, stats.editedESVolume, 0.01f);
	EXPECT_NEAR(diasVolume, stats.editedEDVolume, 0.01f);
	EXPECT_LT(0.0f, stats.editedStrokeVolume);
	EXPECT_LT(0.0f, stats.editedCardiacOutput);
}
*/