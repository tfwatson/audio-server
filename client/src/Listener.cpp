#include "Listener.hpp"

#include <iostream>
#include <stdexcept>

Listener::Listener(unsigned int sampleRate, unsigned int framesPerBuffer, BufferQueue& bufferQueue)
	: mSampleRate(sampleRate),
	  mFramesPerBuffer(framesPerBuffer),
	  mInputParameters(),
	  mBufferQueue(bufferQueue)
{
	std::cout << "Listener initializing...\n";

	mInputParameters.device = Pa_GetDefaultInputDevice();
	if (mInputParameters.device == paNoDevice)
	{
		throw std::runtime_error("Error in Listener initialization: No input devices exist!");
	}

	mInputParameters.channelCount = 1;
	mInputParameters.sampleFormat = paFloat32;
	mInputParameters.suggestedLatency =
		Pa_GetDeviceInfo(mInputParameters.device)->defaultLowInputLatency;

	mInputParameters.hostApiSpecificStreamInfo = nullptr;

	PaError error = Pa_OpenStream(&mStream,
								  &mInputParameters,
								  nullptr,
								  mSampleRate,
								  mFramesPerBuffer,
								  paClipOff,
								  RecordCallback,
								  &mBufferQueue);
	if (error)
	{
		throw std::runtime_error("Error in Listener initialization: " +
								 std::string(Pa_GetErrorText(error)));
	}

	error = Pa_StartStream(mStream);
	if (error)
	{
		Pa_CloseStream(mStream);

		throw std::runtime_error("Error in Listener initialization: " +
								 std::string(Pa_GetErrorText(error)));
	}

	std::cout << "Listener successfully initialized. Now listening to audio!\n";
}

Listener::~Listener()
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

	std::cout << "Listener successfully destroyed. No longer listening to audio!\n";
}

int Listener::RecordCallback(const void* input,
							 void* output,
							 unsigned long frameCount,
							 const PaStreamCallbackTimeInfo* timeInfo,
							 const PaStreamCallbackFlags statusFlags,
							 void* userData)
{
	BufferQueue& bufferQueue = *(static_cast<BufferQueue*>(userData));

	if (input != nullptr)
	{
		const float* rptr = static_cast<const float*>(input);
		for (unsigned int i = 0; i < frameCount; ++i)
		{
			bufferQueue.push(*rptr++);
		}
	}
	else
	{
		for (unsigned int i = 0; i < frameCount; ++i)
		{
			bufferQueue.push(0.0f);
		}
	}

	return paContinue;
}