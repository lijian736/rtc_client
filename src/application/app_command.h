#ifndef _H_APP_COMMAND_H_
#define _H_APP_COMMAND_H_

#include <map>
#include <string>
#include <stdint.h>
#include "common_json.h"

/**
 *error code
 */

/*all is ok*/
const int OK = 200;

/*the invalid request*/
const int REQUEST_INVALID = 400;
/*the resource not found*/
const int RESOURCE_NOT_FOUND = 404;

/*server error*/
const int SERVER_ERROR = 500;

/**
* the command type
*/
typedef int32_t CmdType;

/*create a new conference*/
const CmdType TYPE_CONFERENCE_CREATE = 1000;

/*user join the conference*/
const CmdType TYPE_CONFERENCE_JOIN = 1001;

/*new user has joined the conference*/
const CmdType TYPE_CONFERENCE_NEW_JOINED = 1002;

/*pull audio/video stream from the server*/
const CmdType TYPE_CONFERENCE_PULL_STREAM = 1100;

/*stop pulling audio/video stream from the server*/
const CmdType TYPE_CONFERENCE_STOP_PULLING = 1101;

/*exit the conference*/
const CmdType TYPE_CONFERENCE_EXIT = 1200;

/*a user has gone out of the conference*/
const CmdType TYPE_CONFERENCE_USER_GONE = 1201;

/*stop the conference*/
const CmdType TYPE_CONFERENCE_STOP = 1202;

/*the conference is been closing*/
const CmdType TYPE_CONFERENCE_CLOSING = 1203;

/*query the online users*/
const CmdType TYPE_CONFERENCE_ONLINE_USERS = 2000;

/*kick out a user from the conference*/
const CmdType TYPE_CONFERENCE_KICK_OUT = 2001;

/*the conference heartbeat*/
const CmdType TYPE_CONFERENCE_HEARTBEAT = 3000;

/**
 * @brief CmdType to string
 * 
 * @param cmdType -- the command type
 * @return std::string
 */
std::string cmdtype_to_string(const CmdType &cmdType);

/**
* TYPE_CONFERENCE_CREATE: create a new conference
* {
*      "opcode" : TYPE_CONFERENCE_CREATE,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "user_id" : "user id",
*          "user_name" : "user name"
*      }
* }
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_CREATE,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "conference_id" : "the conference id"，
*          "user_id" : "user id",
*          "user_name" : "user name",
*          "user_ip" : "user ip address",
*          "user_uuid" : "the uuid of user in the conference",
*          "video_ssrc" : "the ssrc of video",
*          "push_video_ip" : "the ip address receiving the pushed video",
*          "push_video_port" : "the port receiving the pushed video",
*          "audio_ssrc" : "the ssrc of audio",
*          "push_audio_ip" ： ”the ip address receiving the pushed audio",
*          "push_audio_port" : "the port receiving the pushed audio"
*      }
* }
*/

/**
* TYPE_CONFERENCE_JOIN: join the conference
*
* {
*      "opcode" : TYPE_CONFERENCE_JOIN,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "conference_id" : "the conference id",
*          "user_id" : "user id",
*          "user_name" : "user name"
*      }
* }
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_JOIN,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "conference_id" : "the conference id"，
*          "user_id" : "user id",
*          "user_name" : "user name",
*          "user_ip" : "user ip address",
*          "user_uuid" : "the uuid of user in the conference",
*          "video_ssrc" : "the ssrc of video",
*          "audio_ssrc" : "the ssrc of audio",
*          "push_video_ip" : "the ip address receiving the pushed video",
*          "push_video_port" : "the port receiving the pushed video",
*          "push_audio_ip" ： ”the ip address receiving the pushed audio",
*          "push_audio_port" : "the port receiving the pushed audio",
*       }
*  }
*/

/**
* TYPE_CONFERENCE_NEW_JOINED: new user has joined the conference
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_NEW_JOINED,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "conference_id" : "the conference id"， 
*          "user_id" : "user id",
*          "user_name" : "user name",
*          "user_ip" : "user ip address",
*          "user_uuid" : "the uuid of user in the conference",
*          "video_ssrc" : "the ssrc of video",
*          "audio_ssrc" : "the ssrc of audio",
*      }
* }
*/

/**
* TYPE_CONFERENCE_PULL_STREAM: pull audio/video stream from the server
*
*{
*      "opcode" : TYPE_CONFERENCE_PULL_STREAM,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference",
*          "streams" : [
*                {
*                   "user_uuid" : "the user uuid of the streams", 
*                   "ssrcs" : [
*                       {"ssrc" : "the first ssrc"}, 
*                       {"ssrc" : "the second ssrc"}
*                   ]
*                },
*                {},
*                {}
*          ]
*      }
* }
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_PULL_STREAM,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference",
*          "streams" : [
*                { 
*                   "user_uuid" : "the user uuid of the streams", 
*                   "ssrcs" : [
*                       {"ssrc" : "the first ssrc", "ip" : "the pinhole ip of the stream", "port": "the pinhole port of the stream"}, 
*                       {"ssrc" : "the second ssrc", "ip" : "the pinhole ip of the stream", "port": "the pinhole port of the stream"}
*                   ]
*                },
*                {},
*                {}
*           ]
*       }
* }
*
*/

/**
* TYPE_CONFERENCE_STOP_PULLING: stop pulling audio/video stream from the server
*
*{
*      "opcode" : TYPE_CONFERENCE_STOP_PULLING,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference",
*          "streams": [
*                { 
*                   "user_uuid" : "the user uuid of the streams", 
*                   "ssrcs" : [
*                       {"ssrc" : "the first ssrc"}, 
*                       {"ssrc" : "the second ssrc"}
*                   ]
*                },
*                {},
*                {}
*          ]
*      }
* }
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_STOP_PULLING,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference"
*      }
*  }
*
*/

/**
* TYPE_CONFERENCE_EXIT: exit the conference
* {
*      "opcode" : TYPE_CONFERENCE_EXIT,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference"
*      }
* }
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_EXIT,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference"
*      }
*  }
*/

/**
* TYPE_CONFERENCE_USER_GONE: a user has gone out of the conference
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_USER_GONE,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference"
*      }
*  }
*/


/**
* TYPE_CONFERENCE_STOP: stop the conference
*
* {
*      "opcode" : TYPE_CONFERENCE_STOP,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference"
*      }
* }
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_STOP,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "user_uuid": "the requested user uuid",
*          "conference_id" : "the id of the conference"
*      }
*  }
*
*/

/**
* TYPE_CONFERENCE_CLOSING: the conference is been closing
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_CLOSING,
*      "request" : "false",
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*           "conference_id" : "the id of the conference"
*       }
* }
*
*/

/**
* TYPE_CONFERENCE_ONLINE_USERS: query the online users
*
* {
*      "opcode" : TYPE_CONFERENCE_ONLINE_USERS,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "conference_id" : "the conference id",
*          "user_uuid" : "the request user uuid",
*      }
* }
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_ONLINE_USERS,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "conference_id" : "the conference id",
*          "online_users": [
*             { 
*                "user_id" : "user id",
*                "user_name" : "user name",
*                "user_ip" : "user ip address",
*                "user_uuid" : "the uuid of user in the conference",
*                "video_ssrc" : "the ssrc of video",
*                "audio_ssrc" : "the ssrc of audio",
*             }, 
*             {user2 info}, 
*             {user3 info}
*          ]
*       }
*  }
*/

/**
* TYPE_CONFERENCE_HEARTBEAT: the conference heartbeat
*
* {
*      "opcode" : TYPE_CONFERENCE_HEARTBEAT,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "conference_id" : "the conference id",
*          "user_uuid" : "the request user uuid",
*      }
* }
*
* {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_HEARTBEAT,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "user_uuid" : "user uuid",
*          "conference_id" : "the conference id"
*       }
*  }
*/

/**
* @brief get a request command
* @param cmd -- the command type
* @param uuid -- the command uuid
* @param params -- the command parameters
* 
* @return the command json string, take the the TYPE_CONFERENCE_CREATE as an example, 
*  the format follows:
*
* TYPE_CONFERENCE_CREATE: create a new conference
* {
*      "opcode" : TYPE_CONFERENCE_CREATE,
*      "request" : "true"
*      "uuid" : "the request uuid",
*      "params" : {
*          "user_id" : "user id",
*          "user_name" : "user name"
*      }
* }
*
*/
std::string app_get_request(CmdType cmd, const std::string &uuid, const std::map<std::string, std::string> &params);

/**
* @brief get a request command
* @param cmd -- the command type
* @param uuid -- the command uuid
* @param params -- the command parameters
* 
* @return the command json string
*/
std::string app_get_request(CmdType cmd, const std::string &uuid, const JsonObject &params);

/**
* @brief get a response command
* @param code -- the response code
* @param msg -- the response code message
* @param cmd -- the CmdType command
* @param uuid -- the uuid of request
* @param params -- the command parameters
*
* @return the command json string, take the the TYPE_CONFERENCE_CREATE as an example, 
* the format follows:
*
*  {
*      "code" : "200",
*      "msg" : "OK",
*      "opcode" : TYPE_CONFERENCE_CREATE,
*      "request" : "false"
*      "uuid" : "the corresponding request uuid",
*      "params" : {
*          "conference_id" : "the conference id"，
*          "user_id" : "user id",
*          "user_name" : "user name",
*          "user_ip" : "user ip address",
*          "user_uuid" : "the uuid of user in the conference",
*          "user_video_ssrc" : "the ssrc of video",
*          "user_audio_ssrc" : "the ssrc of audio",
*          "push_video_ip" : "the ip address receiving the pushed video",
*          "push_video_port" : "the port receiving the pushed video",
*          "push_audio_ip" ： ”the ip address receiving the pushed audio",
*          "push_audio_port" : "the port receiving the pushed audio"
*      }
*  }
*/
std::string app_get_response(const std::string &code, const std::string &msg,
                             CmdType cmd, const std::string &uuid, const std::map<std::string, std::string> &params);

#endif