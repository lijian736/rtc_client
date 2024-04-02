#ifndef _H_COMMON_UTILS_H_
#define _H_COMMON_UTILS_H_

#include <string>
#include <vector>

/**
* @brief check if the path exists
* @param path -- the path
* @return true if the path exists, otherwise return false
*/
bool check_path_exists(const std::string &path);

/**
* @brief create directory recursively
* @param directory -- the directory path
* @return true if create successfully, otherwise return false
*/
bool create_directory_recursively(const std::string &directory);

/**
* @brief split the string
* @param str -- the string trimed
* @param seperator -- the seperator string
* @return the splited string list
*/
std::vector<std::string> string_split(const std::string& str, const std::string& seperator);

/**
* @brief if the string starts with substr
* @param str -- the string
* @param substr -- the substr
* @return
*/
bool string_starts_with(const std::string& str, const std::string& substr);

/**
* @brief if the string ends with substr
* @param str -- the string
* @param substr -- the substr
* @return
*/
bool string_ends_with(const std::string& str, const std::string& substr);


/**
* @brief trim char in trimChars from end of str
* @param str -- the string trimed
*/
void string_trim_end(std::string& str);

/**
* @brief trim char in trimChars from start of str
* @param str -- the string trimed
*/
void string_trim_start(std::string& str);

/**
* @brief trim char in trimChars from start and end of str
* @param str -- the string trimed
*/
void string_trim(std::string& str);

/**
* @brief convert chars in string to lower
* @param str -- the string
*/
void string_to_lower(std::string& str);

/**
* @brief convert chars in string to upper
* @param str -- the string
*/
void string_to_upper(std::string& str);

/**
* @brief replace substring 'search' in string with 'replacement'
* @param str -- the string
*        search -- the substring be replaced
*        replacement -- replace string
*/
void string_replace(std::string& str, const std::string& search, const std::string& replacement);

std::string common_to_string(bool value);
std::string common_to_string(int value);
std::string common_to_string(long value);
std::string common_to_string(long long value);
std::string common_to_string(unsigned int value);
std::string common_to_string(unsigned long value);
std::string common_to_string(unsigned long long value);
std::string common_to_string(float value);
std::string common_to_string(double value);
std::string common_to_string(long double value);

int string_to_int(const std::string& str);
long string_to_long(const std::string& str);
double string_to_double(const std::string& str);

#endif