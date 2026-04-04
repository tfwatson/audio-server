#include "Transmitter.hpp"

#include <chrono>
#include <thread>

Transmitter::Transmitter(BufferQueue& bufferQueue, std::atomic<bool>& running)
	: mBufferQueue(bufferQueue), mRunning(running)
{
}

void Transmitter::Run()
{
	while (mRunning)
	{
		// TODO: drain queue, batch into buffers, encode and send
		(void)mBufferQueue.pop();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}