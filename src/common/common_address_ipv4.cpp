#include "common_address_ipv4.h"
#include <string.h>

IPAddrV4::IPAddrV4(const char* ip, const uint16_t& port)
	:m_ipaddr(ip), m_port(port)
{
	memset(&m_sock_addr, 0, sizeof(sockaddr_in));

	m_sock_addr.sin_family = AF_INET;
	m_sock_addr.sin_port = htons(port);
	m_sock_addr.sin_addr.s_addr = inet_addr(ip);
}

IPAddrV4::~IPAddrV4()
{
}

bool IPAddrV4::operator ==(const IPAddrV4 &rhs) const
{
	if (m_sock_addr.sin_addr.s_addr == rhs.m_sock_addr.sin_addr.s_addr &&
		m_sock_addr.sin_port == rhs.m_sock_addr.sin_port)
	{
		return true;
	}
	return false;
}

const sockaddr_in* IPAddrV4::get_sock_addr() const
{
	return &m_sock_addr;
}

std::string IPAddrV4::get_ipaddr()
{
	return m_ipaddr;
}

uint16_t IPAddrV4::get_port()
{
	return m_port;
}