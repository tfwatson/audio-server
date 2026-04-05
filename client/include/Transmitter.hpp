#pragma once

#include <atomic>
#include <string>

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
	 * @param sampleRate Sample rate of audio input
	 * @param bufferSize Number of samples to batch before sending
	 * @param serverAddress Address of server we send audio to
	 * @param serverPort Port of server we send audio to
	 */
	Transmitter(BufferQueue& bufferQueue,
				std::atomic<bool>& running,
				const unsigned int sampleRate,
				const unsigned int bufferSize,
				std::string serverAddress,
				uint16_t serverPort);

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
	const unsigned int mSampleRate;
	const unsigned int mBufferSize;
	std::string mServerAddress;
	uint16_t mServerPort;
	int mSocket;

	/**
	 * @brief Helper function that wraps send() to ensure all data is sent to server and not just
	 * partial bytes
	 *
	 * @param packet Packet to be sent to server
	 * @param packetSize Size of packet
	 * @return true Packet was fully sent
	 * @return false Packet was not fully sent
	 */
	bool SendPacket(const char* packet, size_t packetSize) const;
};