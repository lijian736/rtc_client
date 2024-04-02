#ifndef _H_COMMON_JSON_H_
#define _H_COMMON_JSON_H_

#include <string>
#include <map>
#include <vector>

#include <stdint.h>

/**
 * json value type
 */
enum ValueType
{
	T_NULL,
	T_BOOL,
	T_INT8,
	T_INT16,
	T_INT32,
	T_INT64,
	T_UINT8,
	T_UINT16,
	T_UINT32,
	T_UINT64,
	T_DOUBLE,
	T_STRING,
	T_OBJ,
	T_UNDEFINED
};

std::string valuetype_to_string(ValueType type);

class JsonObject;

struct JsonValue
{
	ValueType type;

	union {
		bool bool_v;
		int8_t int8_v;
		int16_t int16_v;
		int32_t int32_v;
		int64_t int64_v;
		uint8_t uint8_v;
		uint16_t uint16_v;
		uint32_t uint32_v;
		uint64_t uint64_v;
		double double_v;
		std::string *str;
		JsonObject *object;
	} value;

	JsonValue();
	~JsonValue();
	JsonValue(const JsonValue& value);
	JsonValue& operator=(const JsonValue& value);

	JsonValue(const bool &val);
	JsonValue(const int8_t &val);
	JsonValue(const int16_t &val);
	JsonValue(const int32_t &val);
	JsonValue(const int64_t &val);
	JsonValue(const uint8_t &val);
	JsonValue(const uint16_t &val);
	JsonValue(const uint32_t &val);
	JsonValue(const uint64_t &val);
	JsonValue(const double &val);

	JsonValue(const char *val);
	JsonValue(const uint8_t *val, uint32_t len);

	JsonValue(const std::string &val);
	JsonValue(const JsonObject &val);

	bool as_bool();
	int8_t as_int8();
	int16_t as_int16();
	int32_t as_int32();
	int64_t as_int64();
	uint8_t as_uint8();
	uint16_t as_uint16();
	uint32_t as_uint32();
	uint64_t as_uint64();
	double as_double();
	std::string as_string();
	JsonObject as_object();

	void reset();
};

/**
* JSON Object
*/
class JsonObject
{
public:
	JsonObject();
	JsonObject(const JsonObject& value);
	JsonObject& operator=(const JsonObject& value);
	~JsonObject() {}

	/**
	 * @brief deserialize from string
	 * if the string is a valid json string, return true.
	 *
	 * @param str -- the string
	 * @return true - parse the string successfully.
	 *         false - failed to parse the string
	 */
	bool read_from_string(const std::string& str);

	/**
	 * @brief serialize the json object to string
	 * 
	 * @param str - the serialize result string
	 *
	 * @return true - serialize successfully
	 *         false - failed to serialize
	 */
	bool write_to_string(std::string& str);

	/**
	* @brief get the array size
	*
	* @return if the json object is not an array, return -1.
	*/
	int array_size();

	JsonValue& operator[](int index);
	const JsonValue operator[](int index) const;

	JsonValue& operator[](const char* key);
	const JsonValue operator[](const char* key) const;

	JsonValue& operator[](const std::string& key);
	const JsonValue operator[](const std::string& key) const;

	void clear();

private:
	void escape_string(std::string& str);
	void unescape_string(std::string& str);

	bool read_bool(const std::string &str, JsonValue &result, uint32_t &startPos, const std::string& wanted);
	bool read_null(const std::string &str, uint32_t &startPos);
	bool read_whitespace(const std::string &str, uint32_t &startPos);
	bool read_delimiter(const std::string &str, uint32_t &startPos, char &c);
	bool read_string(const std::string &str, JsonValue &result, uint32_t &startPos);
	bool read_number(const std::string &str, JsonValue &result, uint32_t &startPos);
	bool read_array(const std::string &str, JsonValue &result, uint32_t &startPos);
	bool read_array(const std::string &str, uint32_t &startPos);
	
	bool read_object(const std::string &str, uint32_t &startPos);
	bool read_json_key_value(const std::string &str, JsonValue &result, uint32_t &startPos);

	void push(JsonValue& obj);
private:
	std::map<std::string, JsonValue> m_members;
	bool m_is_array;
};

#endif
