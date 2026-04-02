#pragma once

#include "portaudio.h"

class Listener
{
   public:
    Listener(unsigned int sampleRate, unsigned int framesPerBuffer,
             unsigned int numChannels);

    ~Listener();

   private:
    // Constants for recording
    const unsigned int mSampleRate;
    const unsigned int mFramesPerBuffer;
    const unsigned int mNumChannels;

    PaStreamParameters mInputParameters;
    PaStream* mStream = nullptr;
    PaError mError = paNoError;

    bool mInitialized = false;

    // Recording callback function used by PortAudio
    static int mRecordCallback(const void* input, void* output,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               const PaStreamCallbackFlags statusFlags,
                               void* userData);
};