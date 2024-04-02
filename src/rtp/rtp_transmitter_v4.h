#ifndef _H_RTP_UDPV4_SOCKET_H_
#define _H_RTP_UDPV4_SOCKET_H_

#include <vector>

#include <stdint.h>
#include <sys/types.h>

#ifdef _WIN32
#include <mutex>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <pthread.h>
#endif

#include "common_address_ipv4.h"

//the udp over ipv4 socket parameters
struct RTPTransParamsV4
{
	//the ip address to bind
	uint32_t bindIP;

	//the port to bind
	uint16_t bindPort;

	//the socket send buffer size
	uint32_t sendBufferSize;

	//the socket receive buffer size
	uint32_t recvBufferSize;

	//time to live
	uint8_t ttl;

	RTPTransParamsV4()
	{
		bindIP = 0;
		bindPort = 0;
		sendBufferSize = 1024 * 32;
		recvBufferSize = 1024 * 32;
		ttl = 128;
	}
};

//the rtp transmitter by udp over ipv4
class RTPTransmitterV4
{
public:
	RTPTransmitterV4();
	virtual ~RTPTransmitterV4();

	/**
	 * @brief initialize the rtp transmitter
	 *
	 * @param  params -- the transmitter parameters
	 *
	 * @return if initialize successfully, return true otherwise return false
	 */
	bool init(RTPTransParamsV4* params);

	/**
	 * @brief add the rtp destination address
	 *
	 * @param ip -- the destination ip address
	 *        port -- the destination port
	 *
	 * @return true - successful
	 * @return false - fail
	 */
	bool add_destination(const char* ip, const uint16_t& port);

	/**
	* @brief delete the rtp destination address
	*
	* @param ip -- the destination ip address
	*        port -- the destination port
	*
	* @return true - successful
	* @return false - fail
	*/
	bool delete_destination(const char* ip, const uint16_t& port);

	/**
	 * @brief clear the destination
	 * 
	 * @return true 
	 * @return false 
	 */
	bool clear_destination();

	/**
	 * @brief send data
	 *
	 * @param data -- the data pointer
	 * @param len -- the data length
	 *
	 * @return true - successful
	 * @return false - fail
	 */
	bool send_data(const uint8_t* data, size_t len);

	/**
	 * @brief receive udp data by function select()
	 *
	 * @param buffers -- the buffer which receive rtp data
	 * @param bufferlen -- the buffer length
	 * @param microseconds -- the timeout time in microseconds
	 * @return the received data length
	 */
	int receive_data(uint8_t* buffer, int bufferlen, int64_t microseconds);

private:
	bool m_initialize;

	//local bind ip
	uint32_t m_bind_ip;
	//local bind port
	uint16_t m_bind_port;

#ifdef _WIN32
	SOCKET m_bind_socket;
#else
	//local bind socket
	int m_bind_socket;
#endif

#ifdef _WIN32
	std::mutex m_mutex;
#else
	pthread_mutex_t m_mutex;
#endif
	//the destination addresses
	std::vector<IPAddrV4*> m_destinations;
};

#endif