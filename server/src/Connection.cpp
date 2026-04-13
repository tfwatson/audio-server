#include "Connection.hpp"

#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <atomic>
#include <cerrno>
#include <utility>
#include <vector>

#include "Buffer.hpp"
#include "PacketHeader.hpp"

Connection::Connection(std::atomic<bool>& connectionAlive,
					   int clientSocket,
					   struct sockaddr_in clientAddress,
					   BufferQueue& receiveQueue,
					   BufferQueue& transmitQueue)
	: mConnectionAlive(connectionAlive),
	  mClientSocket(clientSocket),
	  mClientAddress(std::move(clientAddress)),
	  mReceiveQueue(receiveQueue),
	  mTransmitQueue(transmitQueue)
{
	mReceiveBuffer.resize(512);
	mReceivePtr = reinterpret_cast<char*>(&mReceiveHeader);
	mBytesToReceive = sizeof(mReceiveHeader);
	mHeaderReceived = false;
	mSendNewPacket = true;
	mHeaderSent = false;
}

void Connection::Handle()
{
	struct pollfd fds[1];
	fds[0].fd = mClientSocket;
	fds[0].events = POLLIN;

	while (mConnectionAlive)
	{
		// Always be ready to receive
		fds[0].events = POLLIN;
		if (!mSendNewPacket || mTransmitQueue.read_available() > 0)
		{
			// Only be ready to send if already in the middle of it or buffer ready to start sending
			fds[0].events |= POLLOUT;
		}

		int ret = poll(fds, 1, 100);
		if (ret < 0 && errno != EINTR)
		{
			mConnectionAlive = false;
			return;
		}

		if (fds[0].revents & POLLIN)
		{
			HandleReceive();
		}

		if (fds[0].revents & POLLOUT)
		{
			HandleSend();
		}
	}
}

void Connection::HandleReceive()
{
	ssize_t bytesReceived = recv(mClientSocket, mReceivePtr, mBytesToReceive, 0);
	if (bytesReceived < 0)
	{
		// Fatal errors should end the connection while non-fatal should skip iteration
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
		{
			mConnectionAlive = false;
		}
		return;
	}
	mReceivePtr += bytesReceived;
	mBytesToReceive -= bytesReceived;

	if (!mHeaderReceived && mBytesToReceive == 0)
	{
		mHeaderReceived = true;
		mReceiveHeader.mSampleRate = ntohl(mReceiveHeader.mSampleRate);
		mReceiveHeader.mNumFrames = ntohl(mReceiveHeader.mNumFrames);

		if (mReceiveHeader.mNumFrames != mReceiveBuffer.size())
		{
			mReceiveBuffer.resize(mReceiveHeader.mNumFrames);
		}
		mReceivePtr = reinterpret_cast<char*>(mReceiveBuffer.data());
		mBytesToReceive = mReceiveHeader.mNumFrames * sizeof(float);
	}
	else if (mHeaderReceived && mBytesToReceive == 0)
	{
		mReceiveQueue.push(mReceiveBuffer);

		mHeaderReceived = false;
		mReceivePtr = reinterpret_cast<char*>(&mReceiveHeader);
		mBytesToReceive = sizeof(mReceiveHeader);
	}
}

void Connection::HandleSend()
{
	// If we're ready to send a new packet, set it up
	if (mSendNewPacket)
	{
		if (!mTransmitQueue.pop(mSendBuffer))
		{
			return;
		}

		mHeaderSent = false;
		mSendHeader.mNumFrames = htonl(mSendBuffer.size());
		mSendHeader.mSampleRate = htonl(44100);

		mBytesToSend = sizeof(mSendHeader);
		mSendPtr = reinterpret_cast<char*>(&mSendHeader);

		mSendNewPacket = false;
	}

	// Send packet
	ssize_t bytesSent = send(mClientSocket, mSendPtr, mBytesToSend, 0);
	if (bytesSent < 0)
	{
		// Fatal errors should end the connection while non-fatal should skip iteration
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
		{
			mConnectionAlive = false;
		}
		return;
	}
	mBytesToSend -= bytesSent;
	mSendPtr += bytesSent;

	// If we've sent the packet header transition to sending the packet body
	// If we've sent the full packet, mark that we're ready to send a new one
	if (mBytesToSend == 0 && !mHeaderSent)
	{
		mHeaderSent = true;
		mSendPtr = reinterpret_cast<char*>(mSendBuffer.data());
		mBytesToSend = sizeof(float) * mSendBuffer.size();
	}
	else if (mBytesToSend == 0 && mHeaderSent)
	{
		mSendNewPacket = true;
	}
}