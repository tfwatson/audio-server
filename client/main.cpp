#include <cstdio>
#include "portaudio.h"

int main()
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::fprintf(stderr, "PortAudio init failed: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    std::printf("PortAudio initialized — version: %s\n", Pa_GetVersionText());

    Pa_Terminate();
    return 0;
}
