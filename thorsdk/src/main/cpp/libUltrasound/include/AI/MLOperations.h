//
// Copyright 2019 EchoNous Inc.
//
#pragma once

#include <algorithm>
#include <numeric>
#include <cmath>

template <typename InputIterator>
typename std::iterator_traits<InputIterator>::difference_type
argmax(InputIterator first, InputIterator last)
{
    return std::distance(first, std::max_element(first, last));
}

template <typename InputIterator>
typename std::iterator_traits<InputIterator>::difference_type
argmin(InputIterator first, InputIterator last)
{
    return std::distance(first, std::min_element(first, last));
}

// Perform a softmax operation, writing the results to output
template <typename InputIterator, typename InputOutputIterator>
InputOutputIterator softmax(InputIterator first, InputIterator last, InputOutputIterator output)
{
    // Softmax is invariant under translation,
    // Therefore, subtract the maximum value from all input values,
    // so that values range from (-inf,0]
    // This ensures that std::exp will not overflow (results range from 0 to 1)
    double max_value = *std::max_element(first, last);
    auto out_last = std::transform(first, last, output, [=](double v) {return v-max_value;});
    std::transform(output, out_last, output, (double (*)(double))std::exp);
    double sum = std::accumulate(output, out_last, 0.0);
    std::transform(output, out_last, output, [=](double v){ return v/sum; });

    return out_last;
}

// Perform a softmax operation in place
template <typename InputOutputIterator>
InputOutputIterator softmax(InputOutputIterator first, InputOutputIterator last)
{
    // Same as above: invariant under translation.
    double max_value = *std::max_element(first, last);
    std::transform(first, last, first, [=](double v) {return v-max_value;});
    std::transform(first, last, first, (double (*)(double))std::exp);
    double sum = std::accumulate(first, last, 0.0);
    std::transform(first, last, first, [=](double v){ return v/sum; });
    return last;
}

// Returns ln(sum(exp(val[i]))), where val[i] is in [first, last)
// This calculation is more numerically stable than summing all the
// exponents (as that may overflow a float/double)
template <typename InputIterator>
double logsumexp(InputIterator first, InputIterator last)
{
    if (first == last)
        return 0.0;

    // Subtract the max_value from all values. This ensures all the exponentials are in range (-inf, 0]
    // and therefore, if the value underflows, then at least a sensible result (max_value) is returned
    double max_value = *std::max_element(first, last);
    double sum = std::accumulate(first, last, 0.0,
        [=](double acc, double value) { return acc+std::exp(value-max_value); });
    return max_value+std::log(sum);
}

// Perform a log softmax operation in place
template <typename InputOutputIterator>
InputOutputIterator log_softmax(InputOutputIterator first, InputOutputIterator last)
{
    using Scalar = typename std::iterator_traits<InputOutputIterator>::value_type;
    
    if (first == last)
        return first;
    double sum = logsumexp(first, last);
    return std::transform(first, last, first, [=](Scalar val){return val-sum;});
}

// Write the indices which would sort the input range into `indices`
// The input array is left untouched
template <typename RandomAccessIterator, typename OutputIterator>
OutputIterator argsort(RandomAccessIterator first, RandomAccessIterator last, OutputIterator indices)
{
    // Fill indices with [0,N]
    auto length = std::distance(first, last);
    auto indicesLast = std::next(indices, length);
    std::iota(indices, indicesLast, 0);

    using IndexType = typename std::iterator_traits<OutputIterator>::value_type;

    // Sort indices according to values in RandomAccessIterator
    std::sort(indices, indicesLast,
        [=](IndexType i1, IndexType i2) { return *std::next(first, i1) < *std::next(first, i2); });

    return indicesLast;
}

struct mode_info
{
    int mode;
    size_t count;
};

// Find the mode (most common value) from a list of values,
// and the occurance count. To avoid heap allocation, this
// function requires that the range of possible values is
// between [0,N)
template <size_t N, typename InputIterator>
mode_info mode_count(InputIterator first, InputIterator last)
{
    size_t counts[N];
    for (auto it = first; it != last; ++it)
        ++counts[*it];

    auto mode = std::max_element(std::begin(counts), std::end(counts));
    return {static_cast<int>(std::distance(std::begin(counts), mode)), *mode};
}