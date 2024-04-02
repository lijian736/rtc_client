#include "common_port_manager.h"

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include "common_logger.h"

PortManager::PortManager()
{
	m_initialized = false;
#ifdef _WIN32
#else
	pthread_mutex_init(&m_mutex, NULL);
#endif
}

PortManager::~PortManager()
{
#ifdef _WIN32
#else
	pthread_mutex_destroy(&m_mutex);
#endif
}

PortManager* PortManager::get_instance()
{
	static PortManager instance;
	return &instance;
}

void PortManager::initialize_udp_ports(uint16_t start, uint16_t end)
{
	if (m_initialized)
	{
		return;
	}

	m_available_ports.clear();
	m_ports_in_use.clear();

	for (uint16_t i = start; i < end; i++)
	{
		m_available_ports.push_back(i);
	}

	m_initialized = true;
}

bool PortManager::get_udp_port(uint16_t& port)
{
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(m_mutex);
#else
	pthread_mutex_lock(&m_mutex);
#endif
	if (m_available_ports.empty())
	{
		std::set<uint16_t>::iterator it = m_ports_in_use.begin();
		while (it != m_ports_in_use.end())
		{
			m_available_ports.push_back(*it);
			it++;
		}

		m_ports_in_use.clear();
	}

	while (!m_available_ports.empty())
	{
		std::vector<uint16_t>::iterator it = m_available_ports.begin();
		if (it != m_available_ports.end())
		{
			port = *it;
			
			//check if the port is available
			if (check_available(port, PORT_TYPE_UDP))
			{
				m_ports_in_use.insert(port);
				m_available_ports.erase(it);
#ifdef _WIN32
#else
				pthread_mutex_unlock(&m_mutex);
#endif
				return true;
			}
			else
			{
				m_ports_in_use.insert(port);
				m_available_ports.erase(it);
				continue;
			}
		}
	}

	if (m_available_ports.empty())
	{
		std::set<uint16_t>::iterator it = m_ports_in_use.begin();
		while (it != m_ports_in_use.end())
		{
			m_available_ports.push_back(*it);
			it++;
		}

		m_ports_in_use.clear();
	}
#ifdef _WIN32
#else
	pthread_mutex_unlock(&m_mutex);
#endif
	return false;
}

bool PortManager::release_port(uint16_t port)
{
#ifdef _WIN32
	std::unique_lock<std::mutex> lock(m_mutex);
#else
	pthread_mutex_lock(&m_mutex);
#endif
	std::set<uint16_t>::iterator it = m_ports_in_use.find(port);
	if (it != m_ports_in_use.end())
	{
		m_available_ports.push_back(port);
		m_ports_in_use.erase(it);
#ifdef _WIN32
#else
		pthread_mutex_unlock(&m_mutex);
#endif
		return true;
	}

#ifdef _WIN32
#else
	pthread_mutex_unlock(&m_mutex);
#endif
	return false;
}

bool PortManager::check_available(uint16_t port, PortType type)
{
#ifdef _WIN32
	SOCKET listenSocket;
#else
	int listenSocket;
#endif
	
	listenSocket = ::socket(PF_INET, (type == PORT_TYPE_UDP) ? SOCK_DGRAM : SOCK_STREAM, (type == PORT_TYPE_UDP) ? IPPROTO_UDP : IPPROTO_TCP);
#ifdef _WIN32
	if (listenSocket == INVALID_SOCKET)
	{
		LOG_WARNING("Create socket at port[%d:%s] is not available", port, (type == PORT_TYPE_UDP) ? "UDP" : "TCP");
		return false;
	}
#else
	if (listenSocket == -1)
	{
		LOG_WARNING("Create socket at port[%d:%s] is not available", port, (type == PORT_TYPE_UDP) ? "UDP" : "TCP");
		return false;
	}
#endif

	struct sockaddr_in service;
	service.sin_family = PF_INET;
	service.sin_addr.s_addr = inet_addr("0.0.0.0");
	service.sin_port = htons(port);

	/*
	int one = 1;
#ifdef _WIN32
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one)) != 0)
	{
		closesocket(listenSocket);
#else
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const void *)&one, sizeof(one)) != 0)
	{
		close(listenSocket);
#endif
		LOG_DEBUG("setsocketopt SO_REUSEADDR failed, errno:%d, strerror:%s", errno, strerror(errno));
		return false;
	}
	*/

	if (bind(listenSocket, (struct sockaddr*)&service, sizeof(service)) != 0)
	{
#ifdef _WIN32
		closesocket(listenSocket);
#else
		close(listenSocket);
#endif
		LOG_DEBUG("bind failed, errno:%d, strerror:%s", errno, strerror(errno));
		return false;
	}
#ifdef _WIN32
	closesocket(listenSocket);
#else
	close(listenSocket);
#endif
	return true;
}