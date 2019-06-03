/*
 * SocketMgr.cpp
 *
 *  Created on: May 24, 2019
 *      Author: rboyer
 */

#include "SocketMgr.h"

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define MAX_BUFFER_SIZE 80000 	// Large enough for .bmp formatted 320x240 image.

using namespace std;
using namespace cv;


SocketMgr::SocketMgr() :
	m_monfd(-1), m_cmdfd(-1), m_connected(false), m_exited(false)
{
}

SocketMgr::~SocketMgr()
{
	Disconnect();
}

bool SocketMgr::Connect(const char * hostname)
{
	if (!HostnameToIP(hostname))
	{
		cerr << "Error: Could not find server '" << hostname << "'." << endl;
		return false;
	}

	if ( (m_monfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
		cerr << "Error: Could not create socket." << endl;
		return false;
	}

	if ( (m_cmdfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
		cerr << "Error: Could not create socket." << endl;
		close(m_monfd);
		return false;
	}

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(m_serverIP);
	serverAddr.sin_port = htons(SM_MONITOR_PORT);

	if ( connect(m_monfd, (const sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
	{
		cerr << "Error: Could not connect to monitor port." << endl;
		close(m_monfd);
		close(m_cmdfd);
		return false;
	}

	serverAddr.sin_port = htons(SM_COMMAND_PORT);
	if ( connect(m_cmdfd, (const sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
	{
		cerr << "Error: Could not connect to command port." << endl;
		close(m_monfd);
		close(m_cmdfd);
		return false;
	}

	m_threadMon = boost::thread(&SocketMgr::ManageMonitorStream, this);

	m_connected = true;
	return true;
}

void SocketMgr::Disconnect()
{
	if (m_connected)
	{
		if (m_threadMon.joinable())
		{
			m_threadMon.interrupt();
			m_threadMon.join();
		}

		close(m_monfd);
		close(m_cmdfd);
		m_connected = false;
	}
}

unique_ptr<Mat> SocketMgr::GetCurrentFrame()
{
	boost::mutex::scoped_lock lock(m_frameMutex);
	return move(m_currFrame);
}

void SocketMgr::SendCommand(const char * command)
{
	int ret;
	if ((ret = send(m_cmdfd, command, strlen(command), 0)) < 0)
	{
		cerr << "Error: send() returned " << ret << endl;
	}
}

bool SocketMgr::HostnameToIP(const char * hostname)
{
	struct addrinfo hints, *addrs;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status;
	if ( (status = getaddrinfo(hostname, nullptr, &hints, &addrs)) != 0 )
	{
		cerr << "Error: getaddrinfo() returned " << gai_strerror(status) << endl;
		return false;
	}

	struct sockaddr_in * addr_in = (struct sockaddr_in *)addrs->ai_addr;
	inet_ntop(addrs->ai_family, &(addr_in->sin_addr), m_serverIP, sizeof(m_serverIP));

	freeaddrinfo(addrs);
	return true;
}

void SocketMgr::ManageMonitorStream()
{
	// Enter monitor loop.
	char * pBuffer = new char[MAX_BUFFER_SIZE];
	int size = 0;
	while (1)
	{
		// Get a frame from the server.
		if (!RecvFrame(pBuffer, size))
		{
			cerr << "Error: Failed to receive frame." << endl;
			break;
		}

		// Decode into cv:Mat.
		vector<char> buf(pBuffer, pBuffer + size);
		{
			boost::mutex::scoped_lock lock(m_frameMutex);
			m_currFrame = make_unique<Mat>(imdecode(buf, 1));
		}

		// Give main thread an opportunity to shut this thread down.
		try
		{
			boost::this_thread::interruption_point();
		}
		catch (boost::thread_interrupted &)
        {
			break;
        }
	}

	// Clean up.
	delete [] pBuffer;
	m_exited = true;
}

bool SocketMgr::RecvFrame(char * pRawData, int & size)
{
	if ( recv(m_monfd, (char *)(&size), sizeof(size), 0) < 0 )
		return false;

	if (size > MAX_BUFFER_SIZE)
	{
		cerr << "Error: Received frame too large for buffer." << endl;
		return false;
	}

	int sizeRemaining = size;
	while (sizeRemaining > 0)
	{
		int bytesRecv = recv(m_monfd, pRawData, sizeRemaining, 0);
		if (bytesRecv > 0)
		{
			pRawData += bytesRecv;
			sizeRemaining -= bytesRecv;
			continue;
		}
		if (bytesRecv == -1)
		{
			if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
				continue;
		}

		// Something bad happened.
		break;
	}

	return (sizeRemaining == 0);
}
