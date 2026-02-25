#define LOG_TAG "ImageUtils"
#include <ThorUtils.h>
#include <ImageUtils.h>
#include <opencv2/imgproc.hpp>

void WriteImg(const char *path, cv::InputArray _input, bool normalize)
{
	FILE *stream = fopen(path, "wb");
	if (!stream)
	{
		LOGE("Failed to open path %s", path);
		return;
	}

	cv::Mat input = _input.getMat();
	CV_Assert(input.depth() == CV_8U || input.depth() == CV_32F);
	CV_Assert(input.channels() == 1 || input.channels() == 3);

	cv::Mat img;
	if (normalize)
	{
		// normalize and possibly convert type
		cv::Mat normalized;
		cv::normalize(input, normalized, 0.0, 255.0, cv::NORM_MINMAX, CV_8U);
		img = normalized;
	}
	else if (input.depth() == CV_32F)
	{
		// convert type
		input.convertTo(img, CV_8U, 255.0);
	}
	else
	{
		// no change, img can reference input (avoids copy)
		img = input;
	}

	fprintf(stream, "P%c\n%d %d\n255\n", img.channels() == 1 ? '5' : '6', img.size().width, img.size().height);
	fwrite(img.ptr<uint8_t>(), img.channels(), img.total(), stream);
	fclose(stream);
}

void ChannelMax(cv::InputArray _input, cv::OutputArray _output)
{
	cv::Mat input = _input.getMat();
	// TODO expand this to allow more input types?
	CV_Assert(input.depth() == CV_32F);

	_output.create(input.size(), CV_8U);
	cv::Mat output = _output.getMat();

	if (input.channels() == 2)
	{
		input.forEach<cv::Point2f>([&](cv::Point2f pixel, const int loc[]) {
			output.at<uint8_t>(loc[0], loc[1]) = pixel.x > pixel.y ? 0 : 1;
		});
	}
}

void LargestComponent(cv::InputArray _src, cv::OutputArray _dst)
{
	cv::Mat src = _src.getMat();

	cv::Mat labels;
	cv::Mat stats;
	cv::Mat centroids;

	int numLabels = cv::connectedComponentsWithStats(src, labels, stats, centroids, 8, CV_16U);
	LOGD("Found %d connected components", numLabels);

	// Because we are selecting only 1 component, we know it'll fit in an 8U type
	_dst.create(labels.size(), CV_8UC1);
	cv::Mat dst = _dst.getMat();

	if (numLabels < 2)
	{
		// if only 1 label, don't bother with the stats, just convert (and max out the value)
		labels.convertTo(dst, CV_8U, 255.0);
		return;
	}

	// find largest
	int largest = 0;
	int maxArea = 0;
	for (int label=1; label < numLabels; ++label)
	{
		int area = stats.at<int>(label, cv::CC_STAT_AREA);
		LOGD("  Label %d has area %d", label, area);
		if (area > maxArea)
		{
			largest = label;
			maxArea = area;
		}
	}

	// mask out everything but the largest label (this also maxes out the dst)	
	cv::inRange(labels, cv::Scalar(largest), cv::Scalar(largest), dst);
}