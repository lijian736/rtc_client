set (SRC_LIST ./main.cpp)

add_subdirectory(common)
add_subdirectory(codec)
add_subdirectory(rtp)
add_subdirectory(websocket)
add_subdirectory(application)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/codec
    ${PROJECT_SOURCE_DIR}/src/rtp
    ${PROJECT_SOURCE_DIR}/src/websocket
    ${PROJECT_SOURCE_DIR}/src/application
)

add_executable(rtc_client ${SRC_LIST})
#add_library(rtc_client ${SRC_LIST})

target_link_libraries(rtc_client
    common
    codec
    rtp
    application
    websocket
)