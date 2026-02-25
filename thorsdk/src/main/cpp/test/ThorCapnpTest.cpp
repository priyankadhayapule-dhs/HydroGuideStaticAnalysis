#include <UltrasoundManager.h>
#include <CineBuffer.h>
#include <CinePlayer.h>
#include <CineRecorder.h>
#include <BitfieldMacros.h>
#include <dirent.h>
#include <cstdio>
#include <mutex>
#include <condition_variable>

#ifndef THOR_TRY
#define THOR_TRY(expr) ThorAssertOk(expr, #expr, __FILE__, __LINE__)
#endif
static void ThorAssertOk(ThorStatus result, const char *expr, const char *file, int line)
{
    if (IS_THOR_ERROR(result))
    {
        fprintf(stderr, "Thor Error code 0x%08x in \"%s\" (%s:%d)\n", static_cast<uint32_t>(result), expr, file, line);
        fflush(stdout);
        std::abort();
    }
}

static std::mutex saveCompleteMutex;
static std::condition_variable saveCompleteCV;
static bool saveComplete;
void SaveFileCompletionHandler(ThorStatus result)
{
	ThorAssertOk(result, "cinerecorder.saveVideoFromCineFrames(path, 0, end)", __FILE__, __LINE__);

	auto l = std::unique_lock<std::mutex>(saveCompleteMutex);
	saveComplete = true;
	l.unlock();
	saveCompleteCV.notify_one();
}

static void SaveFile(UltrasoundManager &manager, const char *path)
{
	auto &cinerecorder = manager.getCineRecorder();
	cinerecorder.attachCompletionHandler(SaveFileCompletionHandler);
	uint32_t end = manager.getCineBuffer().getMaxFrameNum();

	// Set save status to not yet complete
	{
		auto l = std::unique_lock<std::mutex>(saveCompleteMutex);
		saveComplete = false;
	}

	// Begin save
	THOR_TRY(cinerecorder.saveVideoFromCineFrames(path, 0, end));

	// Wait until save is complete
	{
		auto l = std::unique_lock<std::mutex>(saveCompleteMutex);
		saveCompleteCV.wait(l, [](){ return saveComplete; });
	}

	cinerecorder.detachCompletionHandler();
}

static void CompareCineParams(CineBuffer &cbOld, CineBuffer &cbNew)
{
	auto oldParams = cbOld.getParams();
	auto newParams = cbNew.getParams();

	if (0 != memcmp(&oldParams.converterParams, &newParams.converterParams, sizeof(ScanConverterParams)))
	{
		fprintf(stderr, "Comparison failure in CineParams.converterParams\n");
		std::exit(EXIT_FAILURE);
	}
}

static void CompareFrameHeaders(CineBuffer &cbOld, CineBuffer &cbNew, uint32_t dataType, uint32_t frameNum)
{
	auto *oldHeader = (CineBuffer::CineFrameHeader*) cbOld.getFrame(frameNum, dataType, CineBuffer::FrameContents::IncludeHeader);
	auto *newHeader = (CineBuffer::CineFrameHeader*) cbNew.getFrame(frameNum, dataType, CineBuffer::FrameContents::IncludeHeader);

	//printf("Comparing frame headers for frame %u, dataType %u\n", frameNum, dataType);
	//printf("Old header at %p, new header at %p\n", oldHeader, newHeader);
	//fflush(stdout);

	if (0 != memcmp(oldHeader, newHeader, sizeof(CineBuffer::CineFrameHeader)))
	{
		fprintf(stderr, "Comparison failure in frame header on frame %u, data type %u\n", frameNum, dataType);
		std::exit(EXIT_FAILURE);
	}
}

static void CompareFrameContents(CineBuffer &cbOld, CineBuffer &cbNew, uint32_t dataType, uint32_t frameNum, size_t size)
{
	auto *oldData = cbOld.getFrame(frameNum, dataType);
	auto *newData = cbNew.getFrame(frameNum, dataType);

	//printf("Comparing frame data for frame %u, dataType %u\n", frameNum, dataType);
	//printf("Old data at %p, new data at %p\n", oldData, newData);
	//fflush(stdout);

	if (0 != memcmp(oldData, newData, size))
	{
		fprintf(stderr, "Comparison failure in frame data on frame %u, data type %u\n", frameNum, dataType);
		std::exit(EXIT_FAILURE);
	}
}

static void CompareDataType(CineBuffer &cbOld, CineBuffer &cbNew, uint32_t dataType, size_t size)
{
	for (uint32_t frameNum = cbOld.getMinFrameNum(); frameNum != cbOld.getMaxFrameNum(); ++frameNum)
	{
		//printf("Comparing frame %u\n", frameNum); fflush(stdout);
		CompareFrameHeaders(cbOld, cbNew, dataType, frameNum);
		CompareFrameContents(cbOld, cbNew, dataType, frameNum, size);
	}
}

static void CompareCineBuffers(CineBuffer &cbOld, CineBuffer &cbNew)
{
	CompareCineParams(cbOld, cbNew);
	uint32_t dataTypes = cbOld.getParams().dauDataTypes;
	if (BF_GET(dataTypes, DAU_DATA_TYPE_B, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_B, MAX_B_FRAME_SIZE);
	if (BF_GET(dataTypes, DAU_DATA_TYPE_COLOR, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_COLOR, MAX_COLOR_FRAME_SIZE);

	if (BF_GET(dataTypes, DAU_DATA_TYPE_M, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_M, MAX_M_FRAME_SIZE);
	if (BF_GET(dataTypes, DAU_DATA_TYPE_DA, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_DA, MAX_DA_FRAME_SIZE);
	if (BF_GET(dataTypes, DAU_DATA_TYPE_ECG, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_ECG, MAX_ECG_FRAME_SIZE);

	if (BF_GET(dataTypes, DAU_DATA_TYPE_TEX, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_TEX, MAX_TEX_FRAME_SIZE);
	if (BF_GET(dataTypes, DAU_DATA_TYPE_DA_SCR, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_DA_SCR, MAX_DA_SCR_FRAME_SIZE);
	if (BF_GET(dataTypes, DAU_DATA_TYPE_ECG_SCR, 1))
		CompareDataType(cbOld, cbNew, DAU_DATA_TYPE_ECG_SCR, MAX_ECG_FRAME_SIZE);
}

static void CheckFile(UltrasoundManager &managerOld, UltrasoundManager &managerNew, const char *oldFile)
{
	char oldPath[256];
	char newPath[256];

	snprintf(oldPath, sizeof(oldPath), "capnp-verify/old-thor/%s", oldFile);
	snprintf(newPath, sizeof(newPath), "capnp-verify/new-thor/%s", oldFile);

	// Open old style Thor file in managerOld
	auto &cineplayerOld = managerOld.getCinePlayer();
	THOR_TRY(cineplayerOld.openRawFile(oldPath));
	printf("Load old style file %s\n", oldPath); fflush(stdout);

	// Save to new file
	SaveFile(managerOld, newPath);
	printf("Saved as Capnp file to path %s\n", newPath); fflush(stdout);

	// Open in managerNew
	auto &cineplayerNew = managerNew.getCinePlayer();
	THOR_TRY(cineplayerNew.openRawFile(newPath));
	printf("Loaded Capnp file, comparing...\n"); fflush(stdout);

	// Compare old and new cinebuffer data
	CompareCineBuffers(managerOld.getCineBuffer(), managerNew.getCineBuffer());
	printf("Comparison complete, no differences found.\n"); fflush(stdout);
}

int main()
{
	// Use two ultrasound managers:
	//  one for loading the old style Thor file
	//  one for loading the new capnp style files
	// compare the loaded results of both

	auto managerOld = std::unique_ptr<UltrasoundManager>(new UltrasoundManager);
	THOR_TRY(managerOld->openDirect("assets"));

	auto managerNew = std::unique_ptr<UltrasoundManager>(new UltrasoundManager);
	THOR_TRY(managerNew->openDirect("assets"));

	DIR* dir = opendir("capnp-verify/old-thor");
    if (!dir)
    {
        fprintf(stderr, "Failed to open directory, errno = %d\n", errno);
        return 1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (!strstr(entry->d_name, ".thor"))
            continue;

        CheckFile(*managerOld, *managerNew, entry->d_name);
    }
    closedir(dir);

	return 0;
}