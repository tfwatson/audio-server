#include "Player.hpp"

#include <iostream>
#include <stdexcept>

Player::Player(unsigned int sampleRate, unsigned int framesPerBuffer, BufferQueue& bufferQueue)
	: mSampleRate(sampleRate),
	  mFramesPerBuffer(framesPerBuffer),
	  mOutputParameters(),
	  mBufferQueue(bufferQueue)
{
	std::cout << "Player initializing...\n";

	mOutputParameters.device = Pa_GetDefaultOutputDevice();
	if (mOutputParameters.device == paNoDevice)
	{
		throw std::runtime_error("Error in Player initialization: No output devices exist!");
	}

	mOutputParameters.channelCount = 1;
	mOutputParameters.sampleFormat = paFloat32;
	mOutputParameters.suggestedLatency =
		Pa_GetDeviceInfo(mOutputParameters.device)->defaultLowOutputLatency;

	mOutputParameters.hostApiSpecificStreamInfo = nullptr;

	PaError error = Pa_OpenStream(&mStream,
								  nullptr,
								  &mOutputParameters,
								  mSampleRate,
								  mFramesPerBuffer,
								  paClipOff,
								  PlaybackCallback,
								  &mBufferQueue);
	if (error)
	{
		throw std::runtime_error("Error in Player initialization: " +
								 std::string(Pa_GetErrorText(error)));
	}

	error = Pa_StartStream(mStream);
	if (error)
	{
		Pa_CloseStream(mStream);

		throw std::runtime_error("Error in Player initialization: " +
								 std::string(Pa_GetErrorText(error)));
	}

	std::cout << "Player successfully initialized. Now outputting audio!\n";
}

Player::~Player()
{
	if (mStream)
	{
		if (Pa_IsStreamActive(mStream))
		{
			// Abort rather than Stop to halt immediately without waiting for
			// remaining buffers to finish
			Pa_AbortStream(mStream);
		}
		Pa_CloseStream(mStream);
	}

	std::cout << "Player successfully destroyed. No longer outputting audio!\n";
}

int Player::PlaybackCallback(const void* input,
							 void* output,
							 unsigned long frameCount,
							 const PaStreamCallbackTimeInfo* timeInfo,
							 const PaStreamCallbackFlags statusFlags,
							 void* userData)
{
	float* wptr = static_cast<float*>(output);
	BufferQueue& bufferQueue = *(static_cast<BufferQueue*>(userData));

	for (unsigned int i = 0; i < frameCount; ++i)
	{
		if (bufferQueue.read_available())
		{
			*wptr++ = bufferQueue.front();
			bufferQueue.pop();
		}
		else
		{
			*wptr++ = 0.0f;
		}
	}

	return paContinue;
}