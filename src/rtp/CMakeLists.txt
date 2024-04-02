set (DIR_LIB_SRCS 
    ./rtp_h264_frame_assembler.cpp
    ./rtp_h264_packet_builder.cpp
    ./rtp_aac_packet_builder.cpp
    ./rtp_packet.cpp
    ./rtp_session_audio.cpp
    ./rtp_session_video.cpp
    ./rtp_session_receiver.cpp
    ./rtp_transmitter_v4.cpp
)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/codec
)

add_library(rtp ${DIR_LIB_SRCS})

target_link_libraries(rtp
    common
    codec
)