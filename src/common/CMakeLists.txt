set (DIR_LIB_SRCS 
    ./common_address_ipv4.cpp
    ./common_json.cpp
    ./common_logger.cpp
    ./common_msg_queue.cpp
    ./common_port_manager.cpp
    ./common_utf8.cpp
    ./common_utils.cpp)

add_library(common ${DIR_LIB_SRCS})


