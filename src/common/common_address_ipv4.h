#ifndef _H_COMMON_IP_ADDRESS_V4_H_
#define _H_COMMON_IP_ADDRESS_V4_H_

#include <string>
#include <stdint.h>

#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#endif

//ipv4 destination address
class IPAddrV4
{
public:

	/**
	* @brief constructor
	* @param ip -- the ip address
	*        port -- the port, in host byte order
	*/
	IPAddrV4(const char* ip, const uint16_t& port);
	virtual ~IPAddrV4();

	bool operator ==(const IPAddrV4 &rhs) const;
	const sockaddr_in* get_sock_addr() const;

	std::string get_ipaddr();

	/**
	* @return get the port in host byte order
	*/
	uint16_t get_port();

private:
	sockaddr_in m_sock_addr;

	std::string m_ipaddr;
	//port in host byte order
	uint16_t m_port;
};

#endif
