#ifndef _H_APP_EVENT_H_
#define _H_APP_EVENT_H_

#include <string>
#include <stdint.h>

#define EVENT_CONFERENCE_CREATED 1
#define EVENT_CONFERENCE_JOINED 2
#define EVENT_OTHER_USER_JOINED 3
#define EVENT_ONLINE_USERS 4
#define EVENT_CONFERENCE_EXIT 5
#define EVENT_USER_GONE_OUT 6
#define EVENT_CONFERENCE_STOPPED 7
#define EVENT_CONFERENCE_CLOSING 8

//the online user information
struct OnlineUser
{
    std::string userID;   //the user id
    std::string userName;   //the user name
    std::string userIP;    //the user ip
    std::string userUUID;   //the user uuid
    uint32_t videoSSRC;    //the video ssrc
    uint32_t audioSSRC;    //the audio ssrc
};

#endif