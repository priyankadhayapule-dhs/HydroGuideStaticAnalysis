//
// Copyright 2019 EchoNous, Inc.
//
#include <ThorError.h>
#include <ThorUtils.h>
#include <opencv2/core.hpp>
#include <vector>
#include <iterator>
#include <numeric>
#include <algorithm>


// Forward declarations
class CineBuffer;
class EFNativeJNI;

namespace echonous
{
namespace phase
{

/**
 Scan convert the entire CineBuffer, writing the outputs into the given
 buffer. Frames are written contiguously, plus cv::Mat headers are (optionally)
 written to the mats parameter.
 */
ThorStatus ScanConvert(CineBuffer *cine,
                       cv::Size size,
                       std::vector<float> &buffer,
                       float flipX,
                       std::vector<cv::Mat> *frames = nullptr);

/**
 Run the Phase detection model on all frames, writing the result to output
 */
ThorStatus RunModel(const std::vector<cv::Mat> &frames,
                    std::vector<float> &output, EFNativeJNI* jni);

/**
 Run the Phase detection model on all frames, with a user-supplied java environment, writing the result to output
 */
ThorStatus RunModel(const std::vector<cv::Mat> &frames,
                        std::vector<float> &output, EFNativeJNI* jni, void *jenv);

/**
 Run a minmax filter on input, using the given window size. Output[i] is incremented once for each
 time input[i] is the max value in the window, and decremented once for each time input[i] is the
 min value in the window
 */
ThorStatus MinMax(const std::vector<float> &input, int window, std::vector<int> &output);


// Template functions operating on iterator pairs (similar to std lib)


// Normalize a signal
template <typename InputIt, typename OutputIt>
OutputIt normalize(InputIt first, InputIt last, OutputIt out)
{
    typedef typename std::iterator_traits<InputIt>::value_type value_type; 
    auto minmax = std::minmax_element(first, last);
    return std::transform(first, last, out, [=](const value_type &in) { return (in-*minmax.first)/(*minmax.second-*minmax.first); });
}

template <typename T>
T squared_error(T v1, T v2)
{
    auto d = v1-v2;
    return d*d;
}

// Computes the sum squared error of a signal by itself, offset by a given amount
template <typename InputIt>
float sse_offset(InputIt first, InputIt last, InputIt offset)
{
    typedef typename std::iterator_traits<InputIt>::value_type value_type;
    float error = std::inner_product(offset, last, first, 0.0f, std::plus<value_type>(), squared_error<value_type>);
    return std::inner_product(first, offset, first+std::distance(offset, last), error, std::plus<value_type>(), squared_error<value_type>);
}

// Identify local minima and maxima of a signal
// a value is a local min/max if it is larger/smaller than all other values in the window
template <typename InputIt, typename OutputIt>
void local_minmax(InputIt first, InputIt last, size_t window, OutputIt peaks, OutputIt troughs)
{
    // This algorithm is runtime O(N*window), while it could be runtime O(N) with additional memory usage
    // Could keep a running deque of min/max candidates for the sliding window, but given that window sizes here will
    // be small (<20) it does not make sense to add the extra complexity

    InputIt winStart = first;
    InputIt winMid = first+window/2;
    InputIt winEnd = first+window;
    while (winEnd != last)
    {
        auto minmax = std::minmax_element(winStart, winEnd);
        if (winMid == minmax.first)
            *troughs++ = winMid;
        else if (winMid == minmax.second)
            *peaks++ = winMid;
        ++winStart;
        ++winMid;
        ++winEnd;
    }
}

template <typename InputIt>
size_t period(InputIt first, InputIt last)
{
    typedef typename std::iterator_traits<InputIt>::value_type value_type;
    std::vector<value_type> error;
    for (InputIt it=first; it != last; ++it)
    {
        error.push_back(sse_offset(first, last, it));
    }

    std::vector<typename std::vector<value_type>::iterator> min, max;
    local_minmax(error.begin(), error.end(), 20, std::back_inserter(max), std::back_inserter(min));

    if (min.empty())
        return 0; // no local minima found
    return min.front() - error.begin();
}

}
}