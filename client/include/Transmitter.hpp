#pragma once

#include <atomic>

#include "BufferQueue.hpp"

/**
 * @brief Consumes audio samples from a queue, batches them into buffers,
 * and transmits them to the server
 *
 */
class Transmitter
{
public:
	/**
	 * @brief Construct a new Transmitter object
	 *
	 * @param bufferQueue Queue to consume audio samples from
	 * @param running Shared flag to signal when to stop
	 */
	Transmitter(BufferQueue& bufferQueue, std::atomic<bool>& running);

	/**
	 * @brief Main loop that drains the queue and transmits audio.
	 * Intended to be run on its own thread.
	 *
	 */
	void Run();

	// Delete copy operations
	Transmitter(const Transmitter&) = delete;
	Transmitter& operator=(const Transmitter&) = delete;

private:
	BufferQueue& mBufferQueue;
	std::atomic<bool>& mRunning;
};