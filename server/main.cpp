#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "boost/lockfree/spsc_queue.hpp"
#include "portaudio.h"

const std::string SERVER_PORT = "42069";
constexpr int BACKLOG = 10;
constexpr unsigned int SAMPLE_RATE = 44100;
constexpr unsigned int SAMPLES_PER_BUFFER = 512;
constexpr unsigned int NUM_CHANNELS = 1;
constexpr unsigned int QUEUE_CAPACITY = SAMPLES_PER_BUFFER * 8;

using SampleQueue = boost::lockfree::spsc_queue<float>;
using Buffer = std::array<float, SAMPLES_PER_BUFFER>;

std::atomic<bool> running{true};

void signalHandler(int signal)
{
	running = false;
}

void handleClient(int sockfd)
{
	Buffer incomingBuffer{0.0f};
	Buffer outgoingBuffer{0.0f};
	while (running)
	{
		unsigned long bytesToReceive = SAMPLES_PER_BUFFER * sizeof(float);
		char* incomingData = reinterpret_cast<char*>(incomingBuffer.data());
		while (bytesToReceive > 0 && running)
		{
			const ssize_t bytesReceived = recv(sockfd, incomingData, bytesToReceive, 0);
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
			incomingData += bytesReceived;
			bytesToReceive -= static_cast<size_t>(bytesReceived);
		}

		outgoingBuffer = incomingBuffer;
		const char* outgoingData = reinterpret_cast<const char*>(outgoingBuffer.data());
		unsigned long bytesToSend = SAMPLES_PER_BUFFER * sizeof(float);
		while (bytesToSend > 0 && running)
		{
			const ssize_t bytesSent = send(sockfd, outgoingData, bytesToSend, 0);
			if (bytesSent < 0)
			{
				if (errno == EINTR)
					continue;
				running = false;
				break;
			}
			outgoingData += bytesSent;
			bytesToSend -= static_cast<size_t>(bytesSent);
		}
	}
}

int main()
{
	// Set up signal handling for graceful exit
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	// Setup initial network variables
	int sockfd{-1};
	struct addrinfo hints{};
	struct addrinfo* res{};
	struct addrinfo* p{};
	int yes = 1;
	struct sockaddr_storage incomingAddr{};
	socklen_t incomingAddrSize{};

	// Load network parameter structs with info needed to connect to user specified server
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(nullptr, SERVER_PORT.c_str(), &hints, &res);

	// Look through results and bind to first we can
	for (p = res; p != nullptr; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			continue;
		}

		break;
	}

	if (p == nullptr)
	{
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1)
	{
		exit(1);
	}

	// Free dynamically allocated information needed for initial connection
	freeaddrinfo(res);

	incomingAddrSize = sizeof(incomingAddr);
	int newFd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&incomingAddr), &incomingAddrSize);

	handleClient(newFd);
}