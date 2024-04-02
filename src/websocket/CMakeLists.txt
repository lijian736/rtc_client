set (DIR_LIB_SRCS 
    ./mongoose.cpp
    ./ws_client.cpp
)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/common
)

add_library(websocket ${DIR_LIB_SRCS})

target_link_libraries(websocket
    common
)


