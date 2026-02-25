#pragma once

/*
For each frame:

await spot in queue
await worker thread
for each stage:
	await stage ready (sequential only)
	perform process
await spot in queue 2
await notifier thread
perform notification

*/

#include <array>
#include <chrono>
#include <vector>
#include <string>

class ParallelPipelineProfiler
{
public:
	// Set the maximum number of frames in the circular buffer
	void setMaxFrames(uint32_t maxFrames);
	void addSection(const char *name); // todo add importance?

	// Begins the next section for a given frame.
	// Starting section N finishes section N-1 for the frame
	void startNextSection(uint32_t frame);

	// Mark a frame as complete (finishing its last section)
	void frameComplete(uint32_t frame);

private:
	typedef std::chrono::high_resolution_clock clock;
	typedef clock::time_point time_point;

	std::vector<std::string> mSectionNames;
	std::vector<int> mSections;
	std::vector<std::array<time_point, 32>> mFrameTimes; // up to 31 sections plus an ending time
};