#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "boost/lockfree/spsc_queue.hpp"

const std::string SERVER_PORT = "42069";
constexpr int BACKLOG = 10;
constexpr unsigned int SAMPLE_RATE = 44100;
constexpr unsigned int SAMPLES_PER_BUFFER = 512;
constexpr unsigned int NUM_CHANNELS = 1;
constexpr unsigned int QUEUE_CAPACITY = SAMPLES_PER_BUFFER * 8;

using SampleQueue = boost::lockfree::spsc_queue<float>;
using Buffer = std::array<float, SAMPLES_PER_BUFFER>;

std::atomic<bool> running{true};
std::vector<std::thread> clientThreads;

void signalHandler(int signal)
{
	running = false;
}

void handleClient(int sockfd)
{
	bool connected = true;
	Buffer incomingBuffer{0.0f};
	Buffer outgoingBuffer{0.0f};
	while (running && connected)
	{
		unsigned long bytesToReceive = SAMPLES_PER_BUFFER * sizeof(float);
		char* incomingData = reinterpret_cast<char*>(incomingBuffer.data());
		while (running && connected && bytesToReceive > 0)
		{
			const ssize_t bytesReceived = recv(sockfd, incomingData, bytesToReceive, 0);
			if (bytesReceived == 0)	 // peer closed
			{
				connected = false;
				break;
			}
			if (bytesReceived < 0)
			{
				if (errno == EINTR)
					continue;
				connected = false;
				break;
			}
			incomingData += bytesReceived;
			bytesToReceive -= static_cast<size_t>(bytesReceived);
		}

		outgoingBuffer = incomingBuffer;

		const char* outgoingData = reinterpret_cast<const char*>(outgoingBuffer.data());
		unsigned long bytesToSend = SAMPLES_PER_BUFFER * sizeof(float);
		while (running && connected && bytesToSend > 0)
		{
			const ssize_t bytesSent = send(sockfd, outgoingData, bytesToSend, 0);
			if (bytesSent < 0)
			{
				if (errno == EINTR)
					continue;
				connected = false;
				break;
			}
			outgoingData += bytesSent;
			bytesToSend -= static_cast<size_t>(bytesSent);
		}
	}

	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
}

int main()
{
	// Set up signal handling for graceful exit
	struct sigaction sa{};
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, nullptr) == -1)
	{
		std::cout << "sigaction SIGINT: " << strerror(errno) << std::endl;
		exit(1);
	}

	// SIGPIPE: ignore, so send() returns EPIPE instead of killing the process
	struct sigaction sp{};
	sp.sa_handler = SIG_IGN;
	sigemptyset(&sp.sa_mask);
	sp.sa_flags = 0;
	sigaction(SIGPIPE, &sp, nullptr);

	// Setup initial network variables
	int listenFd{-1};
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
	int error{};
	if ((error = getaddrinfo(nullptr, SERVER_PORT.c_str(), &hints, &res)) != 0)
	{
		std::cout << gai_strerror(error) << std::endl;
		exit(1);
	}

	// Look through results and bind to first we can
	for (p = res; p != nullptr; p = p->ai_next)
	{
		if ((listenFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			continue;
		}

		if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			freeaddrinfo(res);
			exit(1);
		}

		if (bind(listenFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			continue;
		}

		break;
	}

	if (p == nullptr)
	{
		freeaddrinfo(res);
		exit(1);
	}

	if (listen(listenFd, BACKLOG) == -1)
	{
		freeaddrinfo(res);
		exit(1);
	}

	// Free dynamically allocated information needed for initial connection
	freeaddrinfo(res);

	while (running)
	{
		// Accept incoming client connections, ensuring proper error handling
		incomingAddrSize = sizeof(incomingAddr);
		int clientFd =
			accept(listenFd, reinterpret_cast<struct sockaddr*>(&incomingAddr), &incomingAddrSize);
		if (clientFd == -1)
		{
			if (errno == EINTR || errno == ECONNABORTED)
			{
				// Not an actual error, continue
				continue;
			}
			else if (errno == EMFILE || errno == ENFILE)
			{
				// Too many open files, wait for system to clear some then continue
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			else
			{
				// Server failure, set flag to shut down connection for all clients
				running = false;
				break;
			}
		}

		// If connection to client is successful, spawn a thread to handle that client
		clientThreads.emplace_back(handleClient, clientFd);
	}

	// On shutdown, join all client threads
	for (auto& thread : clientThreads)
	{
		thread.join();
	}

	// Close out listening FD
	shutdown(listenFd, SHUT_RDWR);
	close(listenFd);
}