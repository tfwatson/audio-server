#pragma once

#include "portaudio.h"

class Player
{
   public:
    /**
     * @brief Construct a new Player object
     *
     * @param sampleRate
     * @param framesPerBuffer
     * @param numChannels
     */
    Player(unsigned int sampleRate, unsigned int framesPerBuffer,
           unsigned int numChannels);

    /**
     * @brief Destroy the Player object, cleaning up any underlying PortAudio
     * processes
     *
     */
    ~Player();

   private:
    // Constants for playback
    const unsigned int mSampleRate;
    const unsigned int mFramesPerBuffer;
    const unsigned int mNumChannels;

    // PortAudio variables needed for audio playback
    PaStreamParameters mOutputParameters;
    PaStream* mStream = nullptr;
    PaError mError = paNoError;

    /**
     * @brief Function that's called everytime PortAudio receives a buffer
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
    static int mPlayCallback(const void* input, void* output,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             const PaStreamCallbackFlags statusFlags,
                             void* userData);
};