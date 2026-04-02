#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include "Listener.hpp"

std::atomic<bool> running = true;

void signalHandler(int signal)
{
    running = false;
}

int main()
{
    // Register interrupt signal to end the program gracefully
    signal(SIGINT, signalHandler);

    // Setup Listener instance
    std::unique_ptr<Listener> listener;
    try
    {
        listener = std::make_unique<Listener>(44100, 512, 2);
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    // Wait for interrupt signal to gracefully exit
    while(running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Exit successfully
    return 0;
}
