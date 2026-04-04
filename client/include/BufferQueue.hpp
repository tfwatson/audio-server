#pragma once

#include <boost/lockfree/spsc_queue.hpp>

/// @brief Lock-free single-producer single-consumer queue of audio samples
using BufferQueue = boost::lockfree::spsc_queue<float>;