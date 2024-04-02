#include "app_util.h"
#include "common_utils.h"

static uint64_t g_uuid_generator = 0;

std::string get_new_uuid()
{
	g_uuid_generator++;
	return common_to_string(g_uuid_generator);	
}