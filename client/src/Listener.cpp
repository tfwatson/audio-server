#include "Listener.hpp"

#include <portaudio.h>

#include <iostream>
#include <stdexcept>

Listener::Listener(unsigned int sampleRate, unsigned int framesPerBuffer,
                   unsigned int numChannels)
    : mSampleRate(sampleRate),
      mFramesPerBuffer(framesPerBuffer),
      mNumChannels(numChannels)
{
    std::cout << "Listener initializing...\n";

    // Suppress ALSA/JACK warnings
    FILE* devNull = fopen("/dev/null", "w");
    FILE* origStderr = stderr;
    stderr = devNull;

    // Initialize PortAudio
    mError = Pa_Initialize();

    // Restore stderr
    stderr = origStderr;
    fclose(devNull);

    // Handler initialization errors if any
    if (mError)
    {
        throw std::runtime_error("Error in Listener initialization: " +
                                 std::string(Pa_GetErrorText(mError)));
    }
    mInitialized = true;

    // Obtain default input device and set it to be used
    mInputParameters.device = Pa_GetDefaultInputDevice();
    if (mInputParameters.device == paNoDevice)
    {
        // Cleanup
        Pa_Terminate();

        throw std::runtime_error(
            "Error in Listener initialization: No input devices exist!");
    }

    // Set the number of input channels to whatever was passed in
    mInputParameters.channelCount = mNumChannels;

    // Set our samples to be represented by 32-bit floats
    mInputParameters.sampleFormat = paFloat32;

    // Set the suggested latency to be whatever the input device is set to be
    mInputParameters.suggestedLatency =
        Pa_GetDeviceInfo(mInputParameters.device)->defaultLowInputLatency;

    // No host API specific stream info needed
    mInputParameters.hostApiSpecificStreamInfo = nullptr;

    // Open stream using constructor parameters
    mError =
        Pa_OpenStream(&mStream, &mInputParameters, nullptr, mSampleRate,
                      mFramesPerBuffer, paClipOff, mRecordCallback, nullptr);
    if (mError)
    {
        // Cleanup
        Pa_Terminate();

        throw std::runtime_error("Error in Listener initialization: " +
                                 std::string(Pa_GetErrorText(mError)));
    }

    // Start stream to collect audio from user
    mError = Pa_StartStream(mStream);
    if (mError)
    {
        // Cleanup
        Pa_CloseStream(mStream);
        Pa_Terminate();

        throw std::runtime_error("Error in Listener initialization: " +
                                 std::string(Pa_GetErrorText(mError)));
    }

    std::cout << "Listener successfully initialized. Now listening to audio!\n";
}

Listener::~Listener()
{
    // Check if stream is initialized before trying to close it
    if (mStream)
    {
        // Check if stream is active before aborting it
        if (Pa_IsStreamActive(mStream))
        {
            // Abort stream for immediate end to audio collection
            Pa_AbortStream(mStream);
        }

        // Close stream
        Pa_CloseStream(mStream);
    }

    // Check if we initialized PortAudio before trying to terminate it
    if (mInitialized)
    {
        // Terminate PortAudio instance
        Pa_Terminate();
    }

    std::cout
        << "Listener successfully destroyed. No longer listening to audio!\n";
}

int Listener::mRecordCallback(const void* input, void* output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              const PaStreamCallbackFlags statusFlags,
                              void* userData)
{
    return paContinue;
}