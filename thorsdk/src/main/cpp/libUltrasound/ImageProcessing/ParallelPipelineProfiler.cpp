#define LOG_TAG "PipelineProfiler"

#include <ParallelPipelineProfiler.h>
#include <ThorUtils.h>

void ParallelPipelineProfiler::setMaxFrames(uint32_t maxFrames)
{
	mFrameTimes.resize(maxFrames);
	mSections.resize(maxFrames);
}

void ParallelPipelineProfiler::addSection(const char *name)
{
	mSectionNames.emplace_back(name);
}

void ParallelPipelineProfiler::startNextSection(uint32_t frame)
{
	auto now = clock::now();
	frame = frame % mFrameTimes.size();
	int section = mSections[frame]++;

	//ALOGD("frame %u section %d", frame, section);

	mFrameTimes[frame][section] = now;
}

void ParallelPipelineProfiler::frameComplete(uint32_t frame)
{
	auto now = clock::now();
	frame = frame % mFrameTimes.size();
	int lastSection = mSections[frame];
	mSections[frame] = 0;

	mFrameTimes[frame][lastSection] = now;

	char buffer[256];
	char *s = buffer;

	for (int i=0; i != lastSection; ++i)
	{
		// Section i
		auto duration = mFrameTimes[frame][i+1] - mFrameTimes[frame][i];
		uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
		s += snprintf(s, sizeof(buffer)-(s-buffer), "\t%lu", ns);
	}
	//ALOGI("Frame %u%s", frame, buffer);
}