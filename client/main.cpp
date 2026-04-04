#include <signal.h>

#include <iostream>
#include <memory>
#include <thread>

#include "Listener.hpp"
#include "Player.hpp"
#include "Transmitter.hpp"

std::atomic<bool> running = true;

constexpr unsigned int SAMPLE_RATE = 44100;
constexpr unsigned int FRAMES_PER_BUFFER = 512;
constexpr unsigned int QUEUE_CAPACITY = FRAMES_PER_BUFFER * 2;

void signalHandler(int signal)
{
	running = false;
}

int main()
{
	struct sigaction sigIntHandler = {};
	sigIntHandler.sa_handler = signalHandler;
	sigaction(SIGINT, &sigIntHandler, nullptr);

	std::cout << "PortAudio initializing...\n";
	int error = Pa_Initialize();

	if (error)
	{
		std::cerr << "Error initializing PortAudio!\n";
		return 1;
	}
	std::cout << "PortAudio successfully initialized!\n";

	// Scope for listener and players to destruct prior to terminating PortAudio
	{
		BufferQueue inputBufferQueue(QUEUE_CAPACITY);
		// TODO: will be fed by data received from the server
		BufferQueue outputBufferQueue(QUEUE_CAPACITY);

		std::unique_ptr<Listener> listener;
		std::unique_ptr<Player> player;
		try
		{
			listener = std::make_unique<Listener>(SAMPLE_RATE, FRAMES_PER_BUFFER, inputBufferQueue);
			player = std::make_unique<Player>(SAMPLE_RATE, FRAMES_PER_BUFFER, outputBufferQueue);
		}
		catch (const std::runtime_error& error)
		{
			Pa_Terminate();

			std::cerr << error.what() << '\n';
			return 1;
		}

		Transmitter transmitter(inputBufferQueue, running);
		std::thread transmitterThread(&Transmitter::Run, &transmitter);

		while (running)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		std::cout << "Interrupt signal received. Exiting...\n";

		transmitterThread.join();
	}

	Pa_Terminate();
	std::cout << "PortAudio successfully terminated!\n";

	return 0;
}
