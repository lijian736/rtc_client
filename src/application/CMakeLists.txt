set (DIR_LIB_SRCS 
    ./app_command.cpp
    ./app_util.cpp
    ./app_meeting_room.cpp
    ./app_room_user.cpp
)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/application
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/codec
    ${PROJECT_SOURCE_DIR}/src/rtp
    ${PROJECT_SOURCE_DIR}/src/websocket
)

add_library(application ${DIR_LIB_SRCS})

target_link_libraries(application
    common
    codec
    rtp
    websocket
)