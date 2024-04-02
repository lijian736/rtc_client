#ifndef _H_COMMON_PORT_MANAGER_H_
#define _H_COMMON_PORT_MANAGER_H_

#include <set>
#include <vector>
#include <stdint.h>

#ifdef _WIN32
#include <mutex>
#else
#include <pthread.h>
#endif

/**
* the port type
*/
enum PortType
{
	PORT_TYPE_UDP,  //the udp port type
	PORT_TYPE_TCP   //the tcp port type
};

/**
* the port manager
*/
class PortManager
{
private:
	PortManager();

public:
	virtual ~PortManager();

	static PortManager* get_instance();

	/**
	* @brief initialize the udp ports
	*
	* @param start -- the start udp port, included
	*        end -- the end udp port, excluded
	*/
	void initialize_udp_ports(uint16_t start, uint16_t end);

	/**
	* @brief get the udp port
	* @param port - the udp port returned, output parameter
	*
	* @return true - successful, and the returned port is in @param port
	*
	*/
	bool get_udp_port(uint16_t& port);

	/**
	* @brief release the port
	* @param port - the port to release
	* @return true - release successfully. false - failed
	*/
	bool release_port(uint16_t port);
	
	/**
	* @brief check if the port is available
	* @param port - the port to check
	*        type - the port type
	*
	* @return true - the port is available, false - not available
	*/
	bool check_available(uint16_t port, PortType type);

private:
	bool m_initialized;

#ifdef _WIN32
	std::mutex m_mutex;
#else
	pthread_mutex_t m_mutex;
#endif

	//the available ports vector
	std::vector<uint16_t> m_available_ports;

	//the ports in use
	std::set<uint16_t> m_ports_in_use;
};

#endif