#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "boost/lockfree/spsc_queue.hpp"
#include "portaudio.h"

constexpr unsigned int SAMPLE_RATE = 44100;
constexpr unsigned int SAMPLES_PER_BUFFER = 512;
constexpr unsigned int NUM_CHANNELS = 1;
constexpr unsigned int QUEUE_CAPACITY = SAMPLES_PER_BUFFER * 8;
const std::string SERVER_ADDRESS = "127.0.0.1";
const std::string SERVER_PORT = "42069";

using SampleQueue = boost::lockfree::spsc_queue<float>;
using Buffer = std::array<float, SAMPLES_PER_BUFFER>;

std::atomic<bool> running = true;

void signalHandler(int sig)
{
	running = false;
}

int inputCallback(const void* input,
				  void* output,
				  unsigned long frameCount,
				  const PaStreamCallbackTimeInfo* timeInfo,
				  const PaStreamCallbackFlags statusFlags,
				  void* userData)
{
	// Turn passed in void ptr to SampleQueue reference shared across client components
	SampleQueue& sampleQueue = *(static_cast<SampleQueue*>(userData));

	// If user audio detected write it to the output
	if (input != nullptr)
	{
		const float* rptr = static_cast<const float*>(input);
		for (unsigned int i = 0; i < frameCount; ++i)
		{
			sampleQueue.push(*rptr++);
		}
	}
	// If no user audio input was detected write silence to the output
	else
	{
		for (unsigned int i = 0; i < frameCount; ++i)
		{
			sampleQueue.push(0.0f);
		}
	}

	// Return continue signal for stream to know it should stay open
	return paContinue;
}

int outputCallback(const void* input,
				   void* output,
				   unsigned long frameCount,
				   const PaStreamCallbackTimeInfo* timeInfo,
				   const PaStreamCallbackFlags statusFlags,
				   void* userData)
{
	float* wptr = static_cast<float*>(output);
	SampleQueue& sampleQueue = *(static_cast<SampleQueue*>(userData));

	for (unsigned int i = 0; i < frameCount; ++i)
	{
		if (sampleQueue.read_available())
		{
			*wptr++ = sampleQueue.front();
			sampleQueue.pop();
		}
		else
		{
			*wptr++ = 0.0f;
		}
	}

	return paContinue;
}

void receiveCallback(int sockfd, SampleQueue& outputSampleQueue)
{
	Buffer buffer{0.0f};
	while (running)
	{
		unsigned long bytesToReceive = SAMPLES_PER_BUFFER * sizeof(float);
		char* data = reinterpret_cast<char*>(buffer.data());
		while (bytesToReceive > 0 && running)
		{
			const ssize_t bytesReceived = recv(sockfd, data, bytesToReceive, 0);
			if (bytesReceived == 0)	 // peer closed
			{
				running = false;
				return;
			}
			if (bytesReceived < 0)
			{
				if (errno == EINTR)
					continue;
				running = false;
				return;
			}
			data += bytesReceived;
			bytesToReceive -= static_cast<size_t>(bytesReceived);
		}
		outputSampleQueue.push(buffer.data(), SAMPLES_PER_BUFFER);
	}
}

int main()
{
	// Set up signal handler for graceful exit
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	// Error variable to be reused
	int error{};

	// Initialize PortAudio instance
	error = Pa_Initialize();
	if (error != 0)
	{
		std::cout << "Error initializing PA: " << Pa_GetErrorText(error) << std::endl;
		running = false;
	}

	// Setup I/O sample queues
	SampleQueue inputSampleQueue(QUEUE_CAPACITY);
	SampleQueue outputSampleQueue(QUEUE_CAPACITY);

	// Setup PA input parameters
	PaStreamParameters inputParams{};
	inputParams.channelCount = NUM_CHANNELS;
	inputParams.sampleFormat = paFloat32;
	inputParams.device = Pa_GetDefaultInputDevice();
	inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
	inputParams.hostApiSpecificStreamInfo = nullptr;

	// Setup PA input stream and open it
	PaStream* inputStream{};
	error = Pa_OpenStream(&inputStream,
						  &inputParams,
						  nullptr,
						  SAMPLE_RATE,
						  SAMPLES_PER_BUFFER,
						  paClipOff,
						  inputCallback,
						  &inputSampleQueue);
	if (error != 0)
	{
		std::cout << "Error opening input stream: " << Pa_GetErrorText(error) << std::endl;
		running = false;
	}

	// Setup PA output parameters
	PaStreamParameters outputParameters{};
	outputParameters.device = Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = NUM_CHANNELS;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency =
		Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	// Setup PA output stream and open it
	PaStream* outputStream{};
	error = Pa_OpenStream(&outputStream,
						  nullptr,
						  &outputParameters,
						  SAMPLE_RATE,
						  SAMPLES_PER_BUFFER,
						  paClipOff,
						  outputCallback,
						  &outputSampleQueue);
	if (error != 0)
	{
		std::cout << "Error opening output stream: " << Pa_GetErrorText(error) << std::endl;
		running = false;
	}

	// Start PA streams
	error = Pa_StartStream(inputStream);
	if (error != 0)
	{
		std::cout << "Error starting input stream: " << Pa_GetErrorText(error) << std::endl;
		running = false;
	}
	error = Pa_StartStream(outputStream);
	if (error != 0)
	{
		std::cout << "Error starting output stream: " << Pa_GetErrorText(error) << std::endl;
		running = false;
	}

	// ==============================================================

	// Setup initial network variables
	int sockfd{-1};
	struct addrinfo hints{};
	struct addrinfo* res{};

	// Load network parameter structs with info needed to connect to user specified server
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(SERVER_ADDRESS.c_str(), SERVER_PORT.c_str(), &hints, &res);

	// Open a socket for our specified connection type and connect to server over it
	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
	{
		std::cout << "Failed to open socket: " << strerror(errno) << std::endl;
		running = false;
	}
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
	{
		std::cout << "Failed to connect to server: " << strerror(errno) << std::endl;
		running = false;
	}

	// Free dynamically allocated information needed for initial connection
	freeaddrinfo(res);

	// ==============================================================

	auto receiveThread = std::thread(receiveCallback, sockfd, std::ref(outputSampleQueue));

	// Run until user exits
	Buffer buffer{0.0f};
	while (running)
	{
		if (inputSampleQueue.read_available() < SAMPLES_PER_BUFFER)
		{
			Pa_Sleep(2);
			continue;
		}

		inputSampleQueue.pop(buffer.data(), SAMPLES_PER_BUFFER);

		unsigned long bytesToSend = SAMPLES_PER_BUFFER * sizeof(float);
		const char* data = reinterpret_cast<const char*>(buffer.data());
		while (bytesToSend > 0)
		{
			const ssize_t bytesSent = send(sockfd, data, bytesToSend, 0);
			if (bytesSent < 0)
			{
				if (errno == EINTR)
					continue;
				running = false;
				break;
			}
			data += bytesSent;
			bytesToSend -= static_cast<size_t>(bytesSent);
		}
	}

	// Close connection to server
	shutdown(sockfd, SHUT_RDWR);
	receiveThread.join();
	close(sockfd);

	// Clean up PA streams
	Pa_AbortStream(inputStream);
	Pa_AbortStream(outputStream);
	Pa_CloseStream(inputStream);
	Pa_CloseStream(outputStream);
	Pa_Terminate();

	return 0;
}
