#include <portaudio.h>

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include "Listener.hpp"
#include "Player.hpp"

// Flag for program running state
std::atomic<bool> running = true;

void signalHandler(int signal)
{
    // Set running state to false so program terminates
    running = false;
}

int main()
{
    // Register interrupt signal to end the program gracefully
    signal(SIGINT, signalHandler);

    // Suppress ALSA/JACK warnings
    FILE* devNull = fopen("/dev/null", "w");
    FILE* origStderr = stderr;
    stderr = devNull;

    // Initialize PortAudio instance
    std::cout << "PortAudio initializing...\n";
    int error = Pa_Initialize();

    // Restore stderr
    stderr = origStderr;
    fclose(devNull);

    // Handler initialization errors if any
    if (error)
    {
        std::cerr << "Error initializing PortAudio!\n";
        return 1;
    }
    std::cout << "PortAudio successfully initialized!\n";

    // Scope for listener and players to destruct prior to terminating PortAudio
    {
        // Setup Listener instance
        std::unique_ptr<Listener> listener;
        std::unique_ptr<Player> player;
        try
        {
            listener = std::make_unique<Listener>(44100, 512, 2);
            player = std::make_unique<Player>(44100, 512, 2);
        }
        catch (const std::runtime_error& error)
        {
            // Cleanup
            Pa_Terminate();

            std::cerr << error.what() << '\n';
            return 1;
        }

        // Wait for interrupt signal to gracefully exit
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "Interrupt signal received. Exiting...\n";
    }

    // Terminate PortAudio instance
    Pa_Terminate();
    std::cout << "PortAudio successfully terminated!\n";

    // Exit successfully
    return 0;
}
