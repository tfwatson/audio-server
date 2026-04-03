#include "Player.hpp"

#include <portaudio.h>

#include <iostream>
#include <stdexcept>

Player::Player(unsigned int sampleRate, unsigned int framesPerBuffer,
                   unsigned int numChannels)
    : mSampleRate(sampleRate),
      mFramesPerBuffer(framesPerBuffer),
      mNumChannels(numChannels)
{
    std::cout << "Player initializing...\n";

    // Obtain default output device and set it to be used
    mOutputParameters.device = Pa_GetDefaultOutputDevice();
    if (mOutputParameters.device == paNoDevice)
    {
        throw std::runtime_error(
            "Error in Player initialization: No output devices exist!");
    }

    // Set the number of output channels to whatever was passed in
    mOutputParameters.channelCount = mNumChannels;

    // Set our samples to be represented by 32-bit floats
    mOutputParameters.sampleFormat = paFloat32;

    // Set the suggested latency to be whatever the output device is set to be
    mOutputParameters.suggestedLatency =
        Pa_GetDeviceInfo(mOutputParameters.device)->defaultLowOutputLatency;

    // No host API specific stream info needed
    mOutputParameters.hostApiSpecificStreamInfo = nullptr;

    // Open stream using constructor parameters
    mError =
        Pa_OpenStream(&mStream, nullptr, &mOutputParameters, mSampleRate,
                      mFramesPerBuffer, paClipOff, mPlayCallback, nullptr);
    if (mError)
    {
        throw std::runtime_error("Error in Player initialization: " +
                                 std::string(Pa_GetErrorText(mError)));
    }

    // Start stream to output audio to user
    mError = Pa_StartStream(mStream);
    if (mError)
    {
        // Cleanup
        Pa_CloseStream(mStream);

        throw std::runtime_error("Error in Player initialization: " +
                                 std::string(Pa_GetErrorText(mError)));
    }

    std::cout << "Player successfully initialized. Now outputting audio!\n";
}

Player::~Player()
{
    // Check if stream is initialized before trying to close it
    if (mStream)
    {
        // Check if stream is active before aborting it
        if (Pa_IsStreamActive(mStream))
        {
            // Abort stream for immediate end to audio playback
            Pa_AbortStream(mStream);
        }

        // Close stream
        Pa_CloseStream(mStream);
    }

    std::cout
        << "Player successfully destroyed. No longer outputting audio!\n";
}

int Player::mPlayCallback(const void* input, void* output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              const PaStreamCallbackFlags statusFlags,
                              void* userData)
{
    return paContinue;
}