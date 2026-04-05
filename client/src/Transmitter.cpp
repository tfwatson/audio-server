#include "Transmitter.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

#include "PacketHeader.hpp"

Transmitter::Transmitter(BufferQueue& bufferQueue,
						 std::atomic<bool>& running,
						 const unsigned int sampleRate,
						 const unsigned int bufferSize,
						 std::string serverAddress,
						 uint16_t serverPort)
	: mBufferQueue(bufferQueue),
	  mRunning(running),
	  mSampleRate(sampleRate),
	  mBufferSize(bufferSize),
	  mServerAddress(std::move(serverAddress)),
	  mServerPort(serverPort),
	  mSocket(-1)
{
}

void Transmitter::Run()
{
	mSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mSocket == -1)
	{
		std::cerr << "Transmitter: failed to obtain socket: " << strerror(errno) << '\n';
		return;
	}

	struct sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(mServerPort);
	inet_pton(AF_INET, mServerAddress.c_str(), &serverAddr.sin_addr);
	if (connect(mSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0)
	{
		std::cerr << "Transmitter: failed to connect: " << strerror(errno) << '\n';
		close(mSocket);
		mSocket = -1;
		return;
	}

	std::vector<float> buffer;
	buffer.reserve(mBufferSize);
	PacketHeader packetHeader{htonl(mSampleRate), htonl(mBufferSize)};

	// Prefill packet header into packet since it's constant
	std::vector<char> packet(sizeof(PacketHeader) + mBufferSize * sizeof(float));
	memcpy(packet.data(), &packetHeader, sizeof(PacketHeader));

	while (mRunning)
	{
		float sample = 0.0f;
		if (buffer.size() < mBufferSize && mBufferQueue.pop(sample))
		{
			buffer.push_back(sample);
		}
		else if (buffer.size() == mBufferSize)
		{
			memcpy(
				packet.data() + sizeof(PacketHeader), buffer.data(), mBufferSize * sizeof(float));
			if (!SendPacket(packet.data(), sizeof(PacketHeader) + mBufferSize * sizeof(float)))
			{
				std::cerr << "Transmitter: failed to send packet: " << strerror(errno) << '\n';
				close(mSocket);
				mSocket = -1;
				return;
			}
			buffer.clear();
		}
		else
		{
			// Avoid CPU overuse on non-busy cycles
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	close(mSocket);
}

bool Transmitter::SendPacket(const char* packet, size_t packetSize) const
{
	size_t remainingBytes = packetSize;
	while (remainingBytes > 0)
	{
		ssize_t bytesSent = send(mSocket, packet, remainingBytes, 0);
		if (bytesSent < 0)
		{
			return false;
		}
		remainingBytes -= bytesSent;
		packet += bytesSent;
	}
	return true;
}