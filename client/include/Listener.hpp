#pragma once

#include <portaudio.h>

#include "BufferQueue.hpp"

/**
 * @brief RAII Wrapper class for PortAudio recording
 *
 */
class Listener
{
public:
	/**
	 * @brief Construct a new Listener object
	 *
	 * @param sampleRate (Hz) How many times we sample a frame per second
	 * @param framesPerBuffer How many frames are in a buffer
	 * @param bufferQueue Queue to push recorded audio samples into
	 */
	Listener(unsigned int sampleRate, unsigned int framesPerBuffer, BufferQueue& bufferQueue);

	/**
	 * @brief Destroy the Listener object, cleaning up any underlying PortAudio
	 * processes
	 *
	 */
	~Listener();

	// Delete copy operations
	Listener(const Listener&) = delete;
	Listener& operator=(const Listener&) = delete;

private:
	// Data for recording
	const unsigned int mSampleRate;
	const unsigned int mFramesPerBuffer;

	// PortAudio variables needed for audio recording
	PaStreamParameters mInputParameters;
	PaStream* mStream = nullptr;

	BufferQueue& mBufferQueue;

	/**
	 * @brief Function that's called every time PortAudio fills a buffer
	 *
	 * @param input Buffer for audio input
	 * @param output Buffer for audio output (not used)
	 * @param framesPerBuffer How many frames are in each buffer
	 * @param timeInfo Time information about buffer (see PortAudio docs for
	 * description)
	 * @param statusFlags Flags to modify audio input process (see PortAudio
	 * docs for full list of flags)
	 * @param userData Application data that developer can pass in (can be
	 * anything, just cast it inside the function body)
	 * @return int PortAudio Callback Result that instructs what stream should
	 * do after finishing function body (see PortAudio docs for full list of
	 * results)
	 */
	static int RecordCallback(const void* input,
							  void* output,
							  unsigned long framesPerBuffer,
							  const PaStreamCallbackTimeInfo* timeInfo,
							  const PaStreamCallbackFlags statusFlags,
							  void* userData);
};