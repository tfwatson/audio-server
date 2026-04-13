#pragma once

#include <netinet/in.h>
#include <sys/poll.h>

#include <atomic>
#include <cstddef>

#include "Buffer.hpp"
#include "BufferQueue.hpp"
#include "PacketHeader.hpp"

class Connection
{
public:
	Connection(std::atomic<bool>& connectionAlive,
			   int clientSocket,
			   struct sockaddr_in clientAddress,
			   BufferQueue& receiveQueue,
			   BufferQueue& transmitQueue);

	void Handle();

private:
	void HandleReceive();

	void HandleSend();

	std::atomic<bool>& mConnectionAlive;
	int mClientSocket;
	struct sockaddr_in mClientAddress;
	BufferQueue& mReceiveQueue;
	BufferQueue& mTransmitQueue;

	PacketHeader mReceiveHeader;
	Buffer mReceiveBuffer;
	char* mReceivePtr;
	std::size_t mBytesToReceive;
	bool mHeaderReceived;

	PacketHeader mSendHeader;
	Buffer mSendBuffer;
	const char* mSendPtr;
	std::size_t mBytesToSend;
	bool mHeaderSent;
	bool mSendNewPacket;
};