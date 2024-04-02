#include "common_json.h"

#include <sstream>
#include <algorithm>

#include "common_utils.h"

/**static void string_to_lower(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

static void string_to_upper(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

static void string_replace(std::string& str, const std::string& search, const std::string& replacement)
{
	if (search == replacement)
	{
		return;
	}

	if (search == "")
	{
		return;
	}

	std::string::size_type pos;
	std::string::size_type lastPos = 0;
	while ((pos = str.find(search, lastPos)) != std::string::npos)
	{
		str.replace(pos, search.length(), replacement);
		lastPos = pos + replacement.length();
	}
}*/

#define JSONVALUE_RETURN(ctype) \
switch (type)\
{\
case T_BOOL:\
	return value.bool_v ? 1 : 0;\
case T_INT8:\
	return (ctype)value.int8_v;\
case T_INT16:\
	return (ctype)value.int16_v;\
case T_INT32:\
	return (ctype)value.int32_v;\
case T_INT64:\
	return (ctype)value.int64_v;\
case T_UINT8:\
	return (ctype)value.uint8_v;\
case T_UINT16:\
	return (ctype)value.uint16_v;\
case T_UINT32:\
	return (ctype)value.uint32_v;\
case T_UINT64:\
	return (ctype)value.uint64_v;\
case T_DOUBLE:\
	return (ctype)value.double_v;\
case T_OBJ:\
	return value.object != NULL ? 1 : 0;\
case T_STRING:\
	return value.str != NULL ? 1 : 0;\
default:\
	return 0;\
}


std::string valuetype_to_string(ValueType type)
{
	switch (type)
	{
	case T_NULL:
		return "null";
	case T_BOOL:
		return "bool";
	case T_INT8:
		return "int8";
	case T_INT16:
		return "int16";
	case T_INT32:
		return "int32";
	case T_INT64:
		return "int64";
	case T_UINT8:
		return "uint8";
	case T_UINT16:
		return "uint16";
	case T_UINT32:
		return "uint32";
	case T_UINT64:
		return "uint64";
	case T_DOUBLE:
		return "double";
	case T_STRING:
		return "string";
	case T_OBJ:
		return "object";
	case T_UNDEFINED:
		return "undefined";
	default:
		return "error";
	}
}

JsonValue::JsonValue()
{
	type = T_UNDEFINED;
}

JsonValue::~JsonValue()
{
	reset();
}

JsonValue::JsonValue(const JsonValue& rhs)
{
	this->type = rhs.type;

	if (rhs.type == T_STRING)
	{
		this->value.str = new std::string(*rhs.value.str);
	}
	else if (rhs.type == T_OBJ)
	{
		this->value.object = new JsonObject(*rhs.value.object);
	}
	else
	{
		this->value = rhs.value;
	}
}

JsonValue& JsonValue::operator=(const JsonValue& rhs)
{
	if (this == &rhs)
	{
		return *this;
	}

	reset();

	this->type = rhs.type;
	if (rhs.type == T_STRING)
	{
		this->value.str = new std::string(*rhs.value.str);
	}
	else if (rhs.type == T_OBJ)
	{
		this->value.object = new JsonObject(*rhs.value.object);
	}
	else
	{
		this->value = rhs.value;
	}

	return *this;
}

JsonValue::JsonValue(const bool &val)
{
	this->type = T_BOOL;
	this->value.bool_v = val;
}

JsonValue::JsonValue(const int8_t &val)
{
	this->type = T_INT8;
	this->value.int8_v = val;
}

JsonValue::JsonValue(const int16_t &val)
{
	this->type = T_INT16;
	this->value.int16_v = val;
}

JsonValue::JsonValue(const int32_t &val)
{
	this->type = T_INT32;
	this->value.int32_v = val;
}

JsonValue::JsonValue(const int64_t &val)
{
	this->type = T_INT64;
	this->value.int64_v = val;
}

JsonValue::JsonValue(const uint8_t &val)
{
	this->type = T_UINT8;
	this->value.uint8_v = val;
}

JsonValue::JsonValue(const uint16_t &val)
{
	this->type = T_UINT16;
	this->value.uint16_v = val;
}

JsonValue::JsonValue(const uint32_t &val)
{
	this->type = T_UINT32;
	this->value.uint32_v = val;
}

JsonValue::JsonValue(const uint64_t &val)
{
	this->type = T_UINT64;
	this->value.uint64_v = val;
}

JsonValue::JsonValue(const double &val)
{
	this->type = T_DOUBLE;
	this->value.double_v = val;
}

JsonValue::JsonValue(const char *val)
{
	this->type = T_STRING;
	this->value.str = new std::string(val);
}

JsonValue::JsonValue(const uint8_t *val, uint32_t len)
{
	this->type = T_STRING;
	this->value.str = new std::string((const char*)val, len);
}

JsonValue::JsonValue(const std::string &val)
{
	this->type = T_STRING;
	this->value.str = new std::string(val);
}

JsonValue::JsonValue(const JsonObject &val)
{
	this->type = T_OBJ;
	this->value.object = new JsonObject(val);
}

void JsonValue::reset()
{
	if (type == T_STRING)
	{
		delete value.str;
		value.str = NULL;
	}
	else if (type == T_OBJ)
	{
		delete value.object;
		value.object = NULL;
	}

	type = T_UNDEFINED;
}

bool JsonValue::as_bool()
{
	switch (type)
	{
	case T_BOOL:
		return value.bool_v;
	case T_INT8:
		return value.int8_v != 0;
	case T_INT16:
		return value.int16_v != 0;
	case T_INT32:
		return value.int32_v != 0;
	case T_INT64:
		return value.int64_v != 0;
	case T_UINT8:
		return value.uint8_v != 0;
	case T_UINT16:
		return value.uint16_v != 0;
	case T_UINT32:
		return value.uint32_v != 0;
	case T_UINT64:
		return value.uint64_v != 0;
	case T_DOUBLE:
		return !(::abs(value.double_v) < 1e-8);
	case T_OBJ:
		return value.object != NULL ? true : false;
	case T_STRING:
		return value.str != NULL ? true : false;
	default:
		return false;
	}
}

int8_t JsonValue::as_int8()
{
	JSONVALUE_RETURN(int8_t)
}

int16_t JsonValue::as_int16()
{
	JSONVALUE_RETURN(int16_t)
}

int32_t JsonValue::as_int32()
{
	JSONVALUE_RETURN(int32_t)
}

int64_t JsonValue::as_int64()
{
	JSONVALUE_RETURN(int64_t)
}

uint8_t JsonValue::as_uint8()
{
	JSONVALUE_RETURN(uint8_t)
}

uint16_t JsonValue::as_uint16()
{
	JSONVALUE_RETURN(uint16_t)
}

uint32_t JsonValue::as_uint32()
{
	JSONVALUE_RETURN(uint32_t)
}

uint64_t JsonValue::as_uint64()
{
	JSONVALUE_RETURN(uint64_t)
}

double JsonValue::as_double()
{
	JSONVALUE_RETURN(double)
}

std::string JsonValue::as_string()
{
	switch (type)
	{
	case T_BOOL:
		return common_to_string(value.bool_v);
	case T_INT8:
		return common_to_string(value.int8_v);
	case T_INT16:
		return common_to_string(value.int16_v);
	case T_INT32:
		return common_to_string(value.int32_v);
	case T_INT64:
		return common_to_string(value.int64_v);
	case T_UINT8:
		return common_to_string(value.uint8_v);
	case T_UINT16:
		return common_to_string(value.uint16_v);
	case T_UINT32:
		return common_to_string(value.uint32_v);
	case T_UINT64:
		return common_to_string(value.uint64_v);
	case T_DOUBLE:
		return common_to_string(value.double_v);
	case T_STRING:
		return value.str != NULL ? *value.str : "";
	case T_OBJ:
		return "JsonObject";
	default:
		return "";
	}
}

JsonObject JsonValue::as_object()
{
	if (type == T_OBJ)
	{
		return JsonObject(*value.object);
	}

	return JsonObject();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
JsonObject::JsonObject() :m_is_array(false) 
{
}

JsonObject::JsonObject(const JsonObject& value)
{
	this->m_is_array = value.m_is_array;
	this->m_members = value.m_members;
}

JsonObject& JsonObject::operator=(const JsonObject& value)
{
	if (this != &value)
	{
		this->m_is_array = value.m_is_array;
		this->m_members = value.m_members;
	}

	return *this;
}

void JsonObject::clear()
{
	m_members.clear();
	m_is_array = false;
}

bool JsonObject::read_from_string(const std::string& str)
{
	if (str.size() == 0)
	{
		return false;
	}

	uint32_t startPos = 0;

	if (!read_whitespace(str, startPos))
	{
		return false;
	}

	if (str[startPos] == '{')
	{
		return read_object(str, startPos);
	}
	else if (str[startPos] == '[')
	{
		return read_array(str, startPos);
	}
	else
	{
		return false;
	}
}

bool JsonObject::write_to_string(std::string& str)
{
	std::stringstream ss;
	if (m_is_array)
	{
		ss << "[";
	}
	else
	{
		ss << "{";
	}

	std::map<std::string, JsonValue>::const_iterator it;
	for (it = m_members.begin(); it != m_members.end(); it++)
	{
		if (it != m_members.begin())
		{
			ss << ",";
		}

		if (!m_is_array)
		{
			ss << "\"" << it->first << "\"" << ":";
		}

		std::string objStr;
		switch (it->second.type)
		{
		case T_NULL:
			ss << "null";
			break;

		case T_BOOL:
			ss << (it->second.value.bool_v ? "true" : "false");
			break;

		case T_INT8:
			ss << (int32_t)it->second.value.int8_v;
			break;

		case T_INT16:
			ss << (int32_t)it->second.value.int16_v;
			break;

		case T_INT32:
			ss << it->second.value.int32_v;
			break;

		case T_INT64:
			ss << it->second.value.int64_v;
			break;

		case T_UINT8:
			ss << (uint32_t)it->second.value.uint8_v;
			break;

		case T_UINT16:
			ss << (uint32_t)it->second.value.uint16_v;
			break;

		case T_UINT32:
			ss << it->second.value.uint32_v;
			break;

		case T_UINT64:
			ss << it->second.value.uint64_v;
			break;

		case T_DOUBLE:
			ss << it->second.value.double_v;
			break;

		case T_STRING:
			escape_string(*(it->second.value.str));
			ss << "\"" << *(it->second.value.str) << "\"";
			break;

		case T_OBJ:
			if (it->second.value.object->write_to_string(objStr))
			{
				ss << objStr;
			}
			else
			{
				return false;
			}
			break;
			
		case T_UNDEFINED:
			return false;
		}
	}

	if (m_is_array)
	{
		ss << "]";
	}
	else
	{
		ss << "}";
	}

	str = ss.str();
	return true;
}

int JsonObject::array_size()
{
	if (m_is_array)
	{
		return m_members.size();
	}

	return -1;
}

JsonValue& JsonObject::operator[](int index)
{
	m_is_array = true;
	std::map<std::string, JsonValue>::iterator it = m_members.find(common_to_string(index));
	if (it == m_members.end())
	{
		return m_members[common_to_string(index)] = JsonValue();
	}
	else
	{
		return it->second;
	}
}

const JsonValue JsonObject::operator[](int index) const
{
	std::map<std::string, JsonValue>::const_iterator it = m_members.find(common_to_string(index));
	if (it == m_members.end())
	{
		return JsonValue();
	}
	else
	{
		return it->second;
	}
}

JsonValue& JsonObject::operator[](const char* key)
{
	std::map<std::string, JsonValue>::iterator it = m_members.find(key);
	if (it == m_members.end())
	{
		return m_members[key] = JsonValue();
	}
	else
	{
		return it->second;
	}
}

const JsonValue JsonObject::operator[](const char* key) const
{
	std::map<std::string, JsonValue>::const_iterator it = m_members.find(key);
	if (it == m_members.end())
	{
		return JsonValue();
	}
	else
	{
		return it->second;
	}
}

JsonValue& JsonObject::operator[](const std::string& key)
{
	std::map<std::string, JsonValue>::iterator it = m_members.find(key);
	if (it == m_members.end())
	{
		return m_members[key] = JsonValue();
	}
	else
	{
		return it->second;
	}
}

const JsonValue JsonObject::operator[](const std::string& key) const
{
	std::map<std::string, JsonValue>::const_iterator it = m_members.find(key);
	if (it == m_members.end())
	{
		return JsonValue();
	}
	else
	{
		return it->second;
	}
}

bool JsonObject::read_whitespace(const std::string &str, uint32_t &startPos)
{
	for (; startPos < str.length(); startPos++)
	{
		if ((str[startPos] != ' ')
			&& (str[startPos] != '\t')
			&& (str[startPos] != '\r')
			&& (str[startPos] != '\n'))
			break;
	}
	return true;
}

bool JsonObject::read_delimiter(const std::string &str, uint32_t &startPos, char &c)
{
	if (!read_whitespace(str, startPos))
	{
		return false;
	}
	if ((str.size() - startPos) < 1)
	{
		return false;
	}
	c = str[startPos];
	startPos++;
	return read_whitespace(str, startPos);
}

bool JsonObject::read_string(const std::string &str, JsonValue &result, uint32_t &startPos)
{
	if ((str.size() - startPos) < 2)
	{
		return false;
	}
	if (str[startPos] != '\"')
	{
		return false;
	}
	startPos++;
	std::string::size_type pos = startPos;
	while (true)
	{
		pos = str.find('\"', pos);
		if (pos == std::string::npos)
		{
			return false;
		}

		if (str[pos - 1] == '\\')
		{
			pos++;
		}
		else
		{
			std::string value = str.substr(startPos, pos - startPos);
			unescape_string(value);
			result.reset();
			result.type = T_STRING;
			result.value.str = new std::string(value);
			startPos = (uint32_t)(pos + 1);
			return true;
		}
	}
}

bool JsonObject::read_number(const std::string &str, JsonValue &result, uint32_t &startPos)
{
	std::string floatStr = "";
	bool isFloat = false;

	for (; startPos < str.length(); startPos++)
	{
		if ((str[startPos] < '0') || (str[startPos] > '9'))
		{
			if (str[startPos] == '.')
			{
				isFloat = true;
			}
			else
			{
				break;
			}
		}
		floatStr += str[startPos];
	}

	if (floatStr == "")
	{
		return false;
	}

	result.reset();
	if (isFloat)
	{
		result.type = T_DOUBLE;
		result.value.double_v = string_to_double(floatStr);
	}
	else
	{
		result.type = T_INT64;
		result.value.int64_v = (int64_t)string_to_long(floatStr);
	}

	return true;
}

bool JsonObject::read_bool(const std::string &str, JsonValue &result, uint32_t &startPos, const std::string& wanted)
{
	if ((str.size() - startPos) < wanted.size())
	{
		return false;
	}

	std::string temp = str.substr(startPos, wanted.size());
	string_to_lower(temp);

	if (temp != wanted)
	{
		return false;
	}
	startPos += (uint32_t)wanted.size();

	result.reset();
	result.type = T_BOOL;
	result.value.bool_v = (bool)(wanted == "true");

	return true;
}

bool JsonObject::read_null(const std::string &str, uint32_t &startPos)
{
	if ((str.size() - startPos) < 4)
	{
		return false;
	}
	std::string temp = str.substr(startPos, 4);
	string_to_lower(temp);
	if (temp != "null")
	{
		return false;
	}
	startPos += 4;
	return true;
}

bool JsonObject::read_object(const std::string &str, uint32_t &startPos)
{
	if ((str.size() - startPos) < 2)
	{
		return false;
	}
	if (str[startPos] != '{')
	{
		return false;
	}
	startPos++;

	char c;
	while (startPos < str.length())
	{
		if (str[startPos] == '}')
		{
			startPos++;
			return true;
		}

		JsonValue key;
		if (!read_json_key_value(str, key, startPos))
		{
			return false;
		}

		if (key.type != T_STRING)
		{
			return false;
		}

		if (!read_delimiter(str, startPos, c))
		{
			return false;
		}
		if (c != ':')
		{
			return false;
		}

		JsonValue value;
		if (!read_json_key_value(str, value, startPos))
		{
			return false;
		}
		m_members[*(key.value.str)] = value;

		if (!read_delimiter(str, startPos, c))
		{
			return false;
		}

		if (c == '}')
		{
			return true;
		}
		else if (c == ',')
		{
			continue;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool JsonObject::read_array(const std::string &str, uint32_t &startPos)
{
	this->m_is_array = true;

	if ((str.size() - startPos) < 2)
	{
		return false;
	}
	if (str[startPos] != '[')
	{
		return false;
	}
	startPos++;

	char c;
	while (startPos < str.length())
	{
		if (str[startPos] == ']')
		{
			startPos++;
			return true;
		}

		JsonValue jsonValue;
		if (!read_json_key_value(str, jsonValue, startPos))
		{
			return false;
		}
		this->push(jsonValue);

		if (!read_delimiter(str, startPos, c))
		{
			return false;
		}
		if (c == ']')
		{
			return true;
		}
		else if (c == ',')
		{
			continue;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool JsonObject::read_array(const std::string &str, JsonValue &result, uint32_t &startPos)
{
	result.reset();
	result.type = T_OBJ;
	result.value.object = new JsonObject();
	result.value.object->m_is_array = true;

	if ((str.size() - startPos) < 2)
	{
		return false;
	}
	if (str[startPos] != '[')
	{
		return false;
	}
	startPos++;

	char c;
	while (startPos < str.length())
	{
		if (str[startPos] == ']')
		{
			startPos++;
			return true;
		}

		JsonValue jsonValue;
		if (!read_json_key_value(str, jsonValue, startPos))
		{
			return false;
		}
		result.value.object->push(jsonValue);

		if (!read_delimiter(str, startPos, c))
		{
			return false;
		}
		if (c == ']')
		{
			return true;
		}
		else if (c == ',')
		{
			continue;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool JsonObject::read_json_key_value(const std::string &str, JsonValue &result, uint32_t &startPos)
{
	if (startPos >= str.size())
	{
		return false;
	}

	if (!read_whitespace(str, startPos))
	{
		return false;
	}

	switch (str[startPos])
	{
	case '\"':
	{
		return read_string(str, result, startPos);
	}
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	{
		return read_number(str, result, startPos);
	}
	case '{':
	{
		JsonObject* obj = new JsonObject();
		bool ret = obj->read_object(str, startPos);
		if (ret)
		{
			result.reset();
			result.type = T_OBJ;
			result.value.object = obj;
			return true;
		}
		else
		{
			return false;
		}
	}
	case '[':
	{
		return read_array(str, result, startPos);
	}
	case 't':
	case 'T':
	{
		return read_bool(str, result, startPos, "true");
	}
	case 'f':
	case 'F':
	{
		return read_bool(str, result, startPos, "false");
	}
	case 'n':
	case 'N':
	{
		return read_null(str, startPos);
	}
	default:
	{
		result.reset();
		return false;
	}
	}
}

void JsonObject::push(JsonValue& value)
{
	if (!m_is_array)
	{
		return;
	}

	size_t size = m_members.size();

	m_members[common_to_string(size)] = value;
}

void JsonObject::escape_string(std::string& str)
{
	string_replace(str, "\\", "\\\\");
	string_replace(str, "/", "\\/");
	string_replace(str, "\"", "\\\"");
	string_replace(str, "\b", "\\b");
	string_replace(str, "\f", "\\f");
	string_replace(str, "\n", "\\n");
	string_replace(str, "\r", "\\r");
	string_replace(str, "\t", "\\t");
}

void JsonObject::unescape_string(std::string& str)
{
	string_replace(str, "\\/", "/");
	string_replace(str, "\\\"", "\"");
	string_replace(str, "\\b", "\b");
	string_replace(str, "\\f", "\f");
	string_replace(str, "\\n", "\n");
	string_replace(str, "\\r", "\r");
	string_replace(str, "\\t", "\t");
	string_replace(str, "\\\\", "\\");
}