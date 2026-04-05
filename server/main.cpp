#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <vector>

#include "PacketHeader.hpp"

std::atomic<bool> running = true;

bool ReceivePacket(int fd, PacketHeader& packetHeader, std::vector<float>& packetBody)
{
	char* headerPtr = reinterpret_cast<char*>(&packetHeader);
	size_t remaining = sizeof(packetHeader);
	while (remaining > 0)
	{
		ssize_t bytesReceived = recv(fd, headerPtr, remaining, 0);
		if (bytesReceived == -1)
		{
			return false;
		}
		headerPtr += bytesReceived;
		remaining -= bytesReceived;
	}
	packetHeader.mNumFrames = ntohl(packetHeader.mNumFrames);
	packetHeader.mSampleRate = ntohl(packetHeader.mSampleRate);

	char* bodyPtr = reinterpret_cast<char*>(packetBody.data());
	remaining = packetHeader.mNumFrames * sizeof(float);
	while (remaining > 0)
	{
		ssize_t bytesReceived = recv(fd, bodyPtr, remaining, 0);
		if (bytesReceived == -1)
		{
			return false;
		}
		bodyPtr += bytesReceived;
		remaining -= bytesReceived;
	}

	return true;
}

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

	std::vector<float> buffer;
	buffer.resize(512, 0.0f);
	PacketHeader header{};
	while (running)
	{
		ReceivePacket(clientfd, header, buffer);
		std::cout << "Received packet with " << header.mNumFrames << " frames!\n";
		buffer.clear();
		header.mNumFrames = 0;
		header.mSampleRate = 0;
	}
}