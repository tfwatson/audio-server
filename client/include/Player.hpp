#pragma once

#include <portaudio.h>

#include "BufferQueue.hpp"

/**
 * @brief RAII Wrapper class for PortAudio playback
 *
 */
class Player
{
public:
	/**
	 * @brief Construct a new Player object
	 *
	 * @param sampleRate (Hz) How many times we sample a frame per second
	 * @param framesPerBuffer How many frames are in a buffer
	 * @param bufferQueue Queue to pull audio samples from for playback
	 */
	Player(unsigned int sampleRate, unsigned int framesPerBuffer, BufferQueue& bufferQueue);

	/**
	 * @brief Destroy the Player object, cleaning up any underlying PortAudio
	 * processes
	 *
	 */
	~Player();

	// Delete copy operations
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;

private:
	// Constants for playback
	const unsigned int mSampleRate;
	const unsigned int mFramesPerBuffer;

	// PortAudio variables needed for audio playback
	PaStreamParameters mOutputParameters;
	PaStream* mStream = nullptr;

	BufferQueue& mBufferQueue;

	/**
	 * @brief Function that's called every time PortAudio receives a buffer
	 *
	 * @param input Buffer for audio input (not used)
	 * @param output Buffer for audio output
	 * @param framesPerBuffer How many frames are in each buffer
	 * @param timeInfo Time information about buffer (see PortAudio docs for
	 * description)
	 * @param statusFlags Flags to modify audio output process (see PortAudio
	 * docs for full list of flags)
	 * @param userData Application data that developer can pass in (can be
	 * anything, just cast it inside the function body)
	 * @return int PortAudio Callback Result that instructs what stream should
	 * do after finishing function body (see PortAudio docs for full list of
	 * results)
	 */
	static int PlaybackCallback(const void* input,
								void* output,
								unsigned long framesPerBuffer,
								const PaStreamCallbackTimeInfo* timeInfo,
								const PaStreamCallbackFlags statusFlags,
								void* userData);
};