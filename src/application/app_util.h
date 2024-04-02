#ifndef _H_APP_UTIL_H_
#define _H_APP_UTIL_H_

#include <string>
#include <stdint.h>

struct SourceMessage
{
	std::string ip;
	uint16_t port;
	std::string msg;
};

/**
 * @brief Get the new uuid
 * 
 * @return std::string 
 */
std::string get_new_uuid();

#endif