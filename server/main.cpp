#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "Buffer.hpp"
#include "BufferQueue.hpp"
#include "Connection.hpp"
#include "PacketHeader.hpp"

std::atomic<bool> running = true;

void signalHandler(int signal)
{
	running = false;
}

int main()
{
	struct sigaction sigIntHandler = {};
	sigIntHandler.sa_handler = signalHandler;
	sigaction(SIGINT, &sigIntHandler, nullptr);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(42069);

	int error = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
	error = listen(sockfd, 5);

	struct sockaddr_in clientAddr = {};
	socklen_t clientLen = sizeof(clientAddr);
	int clientfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);

	BufferQueue receiveQueue(5);
	BufferQueue transmitQueue(5);
	Connection client(running, clientfd, addr, receiveQueue, transmitQueue);
	std::thread clientThread(&Connection::Handle, &client);

	Buffer intermittent;
	while (running)
	{
		if (receiveQueue.read_available() > 0 && receiveQueue.pop(intermittent)) {
			transmitQueue.push(intermittent);
			std::cout << "Processed full packet!\n";
		}
	}

	clientThread.join();
}