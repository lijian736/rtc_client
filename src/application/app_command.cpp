#include "app_command.h"

#include "common_utils.h"

std::string cmdtype_to_string(const CmdType &cmdType)
{
	switch (cmdType)
	{
	case TYPE_CONFERENCE_CREATE:
		return "TYPE_CONFERENCE_CREATE";

	case TYPE_CONFERENCE_JOIN:
		return "TYPE_CONFERENCE_JOIN";

	case TYPE_CONFERENCE_PULL_STREAM:
		return "TYPE_CONFERENCE_PULL_STREAM";

	case TYPE_CONFERENCE_STOP_PULLING:
		return "TYPE_CONFERENCE_STOP_PULLING";

	case TYPE_CONFERENCE_EXIT:
		return "TYPE_CONFERENCE_EXIT";

	case TYPE_CONFERENCE_STOP:
		return "TYPE_CONFERENCE_STOP";

	case TYPE_CONFERENCE_ONLINE_USERS:
		return "TYPE_CONFERENCE_ONLINE_USERS";

	case TYPE_CONFERENCE_HEARTBEAT:
		return "TYPE_CONFERENCE_HEARTBEAT";

	default:
		return "Unknown command type";
	}
}

std::string app_get_request(CmdType cmd, const std::string &uuid,
							const std::map<std::string, std::string> &params)
{
	std::string result;

	JsonObject paramsJson;

	std::map<std::string, std::string>::const_iterator it = params.begin();
	for (; it != params.end(); it++)
	{
		paramsJson[it->first] = it->second;
	}

	JsonObject json;
	json["opcode"] = common_to_string(cmd);
	json["request"] = "true";
	json["uuid"] = uuid;
	json["params"] = paramsJson;

	if (json.write_to_string(result))
	{
		return result;
	}

	return "";
}

std::string app_get_request(CmdType cmd, const std::string &uuid, const JsonObject &params)
{
	std::string result;

	JsonObject json;
	json["opcode"] = common_to_string(cmd);
	json["request"] = "true";
	json["uuid"] = uuid;
	json["params"] = params;

	if (json.write_to_string(result))
	{
		return result;
	}

	return "";
}

std::string app_get_response(const std::string &code, const std::string &msg,
							 CmdType cmd, const std::string &uuid, const std::map<std::string, std::string> &params)
{
	std::string result;

	JsonObject paramsJson;

	std::map<std::string, std::string>::const_iterator it = params.begin();
	for (; it != params.end(); it++)
	{
		paramsJson[it->first] = it->second;
	}

	JsonObject json;
	json["code"] = code;
	json["msg"] = msg;
	json["request"] = "false";
	json["opcode"] = common_to_string(cmd);
	json["uuid"] = uuid;
	json["params"] = paramsJson;

	if (json.write_to_string(result))
	{
		return result;
	}

	return "";
}