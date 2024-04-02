#include "rtp_transmitter_v4.h"

#include <errno.h>
#include <string.h>

#ifdef _WIN32
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include "common_logger.h"
#include "common_port_manager.h"

static uint16_t get_available_port()
{
	uint16_t port;
	bool ret = PortManager::get_instance()->get_udp_port(port);

	return ret ? port : 0;
}

RTPTransmitterV4::RTPTransmitterV4()
{
	m_initialize = false;
	m_bind_ip = 0;
	m_bind_port = 0;
#ifdef _WIN32
	m_bind_socket = INVALID_SOCKET;
#else
	m_bind_socket = -1;
	pthread_mutex_init(&m_mutex, NULL);
#endif
}

RTPTransmitterV4::~RTPTransmitterV4()
{
#ifdef _WIN32
	if (m_bind_socket != INVALID_SOCKET)
	{
		closesocket(m_bind_socket);
	}
#else
	if (m_bind_socket != -1)
	{
		close(m_bind_socket);
	}
#endif

	std::vector<IPAddrV4 *>::iterator it = m_destinations.begin();
	for (; it != m_destinations.end(); it++)
	{
		delete *it;
	}
	m_destinations.clear();
#ifdef _WIN32
#else
	pthread_mutex_destroy(&m_mutex);
#endif
}

bool RTPTransmitterV4::init(RTPTransParamsV4 *params)
{
	if (m_initialize)
	{
		return true;
	}

	uint8_t ttl;
	if (params)
	{
		this->m_bind_ip = params->bindIP;
		this->m_bind_port = params->bindPort;
		ttl = params->ttl;
	}
	else
	{
		this->m_bind_ip = 0;
		this->m_bind_port = get_available_port();
		ttl = 128;
	}

	m_bind_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
	if (m_bind_socket == INVALID_SOCKET)
	{
		return false;
	}
#else
	if (m_bind_socket == -1)
	{
		return false;
	}
#endif

	/*
	int one = 1;

#ifdef _WIN32
	if (setsockopt(m_bind_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one)) != 0)
	{
		LOG_ERROR("fail to reuse address");

		closesocket(m_bind_socket);
		m_bind_socket = INVALID_SOCKET;
		return false;
	}
#else
	if (setsockopt(m_bind_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&one, sizeof(one)) != 0)
	{
		LOG_ERROR("fail to reuse address");

		close(m_bind_socket);
		m_bind_socket = -1;
		return false;
	}

	if (setsockopt(m_bind_socket, IPPROTO_IP, IP_TTL, (const void *)&ttl, sizeof(ttl)) != 0)
	{
		LOG_WARNING("fail to set IP_TTL: %d; error was (%d)", ttl, errno);
	}
#endif
	*/
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(this->m_bind_port);
	addr.sin_addr.s_addr = htonl(this->m_bind_ip);

	if (::bind(m_bind_socket, (struct sockaddr *)&addr, sizeof(sockaddr_in)) != 0)
	{
		LOG_ERROR("fail to bind socket address, %d", errno);

#ifdef _WIN32
		closesocket(m_bind_socket);
		m_bind_socket = INVALID_SOCKET;
#else
		close(m_bind_socket);
		m_bind_socket = -1;
#endif
		return false;
	}

	m_initialize = true;
	return true;
}

bool RTPTransmitterV4::add_destination(const char *ip, const uint16_t &port)
{
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(m_mutex);
#else
	pthread_mutex_lock(&m_mutex);
#endif

	IPAddrV4 *addr = new IPAddrV4(ip, port);
	m_destinations.push_back(addr);

#ifdef _WIN32
#else
	pthread_mutex_unlock(&m_mutex);
#endif
	return true;
}

bool RTPTransmitterV4::delete_destination(const char *ip, const uint16_t &port)
{
	IPAddrV4 tmp(ip, port);
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(m_mutex);
#else
	pthread_mutex_lock(&m_mutex);
#endif
	std::vector<IPAddrV4 *>::iterator it = m_destinations.begin();
	while (it != m_destinations.end())
	{
		if (tmp == (**it))
		{
			delete (*it);
			it = m_destinations.erase(it);
		}
		else
		{
			it++;
		}
	}

#ifdef _WIN32
#else
	pthread_mutex_unlock(&m_mutex);
#endif
	return true;
}

bool RTPTransmitterV4::clear_destination()
{
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(m_mutex);
#else
	pthread_mutex_lock(&m_mutex);
#endif
	std::vector<IPAddrV4 *>::iterator it = m_destinations.begin();
	for (; it != m_destinations.end(); it++)
	{
		delete *it;
	}
	m_destinations.clear();
#ifdef _WIN32
#else
	pthread_mutex_unlock(&m_mutex);
#endif
	return true;
}

bool RTPTransmitterV4::send_data(const uint8_t *data, size_t len)
{
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(m_mutex);
#else
	pthread_mutex_lock(&m_mutex);
#endif

	std::vector<IPAddrV4 *>::const_iterator it = m_destinations.begin();
	for (; it != m_destinations.end(); it++)
	{
		int ret = ::sendto(m_bind_socket, (const char *)data, (int)len, 0,
						   (const struct sockaddr *)(*it)->get_sock_addr(), (int)sizeof(sockaddr_in));
		if (ret == -1)
		{
#ifdef _WIN32
#else
			pthread_mutex_unlock(&m_mutex);
#endif
			return false;
		}
		else if (ret < (int)len)
		{
#ifdef _WIN32
#else
			pthread_mutex_unlock(&m_mutex);
#endif
			return false;
		}
	}

#ifdef _WIN32
#else
	pthread_mutex_unlock(&m_mutex);
#endif
	return true;
}

int RTPTransmitterV4::receive_data(uint8_t *buffer, int bufferlen, int64_t microseconds)
{
	if (!m_initialize)
	{
		return 0;
	}

	int ret;
	fd_set fdset;
	struct timeval zerotv;
	int recvLen = 0;
	bool dataArrived = false;
	unsigned long len = 0;
	sockaddr_in srcAddr;
	int fromLen = sizeof(sockaddr_in);

	len = 0;
#ifdef _WIN32
	ret = ioctlsocket(m_bind_socket, FIONREAD, &len);
#else
	ret = ioctl(m_bind_socket, FIONREAD, &len);
#endif
	if (ret == 0 && len > 0)
	{
		dataArrived = true;
	}
	else
	{
		FD_ZERO(&fdset);
		FD_SET(m_bind_socket, &fdset);

		zerotv.tv_sec = (long)(microseconds / 1000000);
		zerotv.tv_usec = (long)(microseconds % 1000000);

		if (select(FD_SETSIZE, &fdset, 0, 0, &zerotv) < 0)
		{
			return 0;
		}

		if (FD_ISSET(m_bind_socket, &fdset))
		{
			dataArrived = true;
		}
		else
		{
			dataArrived = false;
			return 0;
		}
	}

	if (dataArrived)
	{
		//If no error occurs, recvfrom returns the number of bytes received.
		//If the connection has been gracefully closed, the return value is zero.
		//Otherwise, a value of SOCKET_ERROR is returned,
		//and a specific error code can be retrieved by calling WSAGetLastError(windows platform)
#ifdef _WIN32
		recvLen = ::recvfrom(m_bind_socket, (char *)buffer, bufferlen, 0, (struct sockaddr *)&srcAddr, &fromLen);
#else
		recvLen = ::recvfrom(m_bind_socket, (char *)buffer, bufferlen, 0, (struct sockaddr *)&srcAddr, (socklen_t *)&fromLen);
#endif
		if (recvLen > 0)
		{
			//uint32_t srcIP = (uint32_t)ntohl(srcAddr.sin_addr.s_addr);
			//uint16_t srcPort = ntohs(srcAddr.sin_port);

			return recvLen;
		}
		else if (recvLen == -1) //error
		{
			return 0;
		}
		else if (recvLen == 0) //connection closed
		{
			return 0;
		}
	}

	return 0;
}