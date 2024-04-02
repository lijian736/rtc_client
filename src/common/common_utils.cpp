#include "common_utils.h"

#include <algorithm>
#include <cctype>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#endif

bool check_path_exists(const std::string &path)
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributesA(path.c_str());
	return INVALID_FILE_ATTRIBUTES != dwAttrib && 0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat st;
	if (stat(path.c_str(), &st) == 0)
	{
		return true;
	}

	return false;
#endif
}

bool create_directory_recursively(const std::string &directory)
{
	if (check_path_exists(directory))
	{
		return true;
	}
#ifdef _WIN32
	if (_access(directory.c_str(), 0) != -1 || directory.empty())
	{
		return false;
	}

	create_directory_recursively(directory.substr(0, directory.rfind('\\')));
	mkdir(directory.c_str());
	return true;
#else

	size_t pos = directory.rfind("/");
	if (pos != std::string::npos)
	{
		std::string parent = directory.substr(0, pos);
		bool ret = create_directory_recursively(parent);
		if (!ret)
		{
			return false;
		}
	}

	mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	mode_t oldMask = umask(0);
	int mkdirRet = mkdir(directory.c_str(), mode);
	umask(oldMask);
	if (mkdirRet != 0)
	{
		if (errno == EEXIST)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
#endif
}

std::vector<std::string> string_split(const std::string& str, const std::string& seperator)
{
	std::vector<std::string> result;
	if (seperator.empty())
	{
		result.push_back(str);
		return result;
	}

	std::size_t pos_begin = 0;
	std::size_t pos_seperator = str.find(seperator);
	while (pos_seperator != std::string::npos)
	{
		result.push_back(str.substr(pos_begin, pos_seperator - pos_begin));
		pos_begin = pos_seperator + seperator.length();
		pos_seperator = str.find(seperator, pos_begin);
	}

	result.push_back(str.substr(pos_begin));

	return result;
}

bool string_starts_with(const std::string& str, const std::string& substr)
{
	return str.find(substr) == 0;
}

bool string_ends_with(const std::string& str, const std::string& substr)
{
	size_t pos = str.rfind(substr);
	return (pos != std::string::npos) && (pos == str.length() - substr.length());
}

static bool is_not_space(int ch)
{
	return !std::isspace(ch);
}

void string_trim_end(std::string& str)
{
	str.erase(std::find_if(str.rbegin(), str.rend(), is_not_space).base(), str.end());
}

void string_trim_start(std::string& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), is_not_space));
}

void string_trim(std::string& str)
{
	str.erase(std::find_if(str.rbegin(), str.rend(), is_not_space).base(), str.end());

	str.erase(str.begin(), std::find_if(str.begin(), str.end(), is_not_space));
}

void string_to_lower(std::string& str)
{
	transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void string_to_upper(std::string& str)
{
	transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void string_replace(std::string& str, const std::string& search, const std::string& replacement)
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
}

std::string common_to_string(bool value)
{
	if(value)
	{
		return "true";
	}
	
	return "false";
}

std::string common_to_string(int value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%d", value);
	return std::string(buffer, i);
}

std::string common_to_string(long value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%ld", value);
	return std::string(buffer, i);
}

std::string common_to_string(long long value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%lld", value);
	return std::string(buffer, i);
}

std::string common_to_string(unsigned int value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%u", value);
	return std::string(buffer, i);
}

std::string common_to_string(unsigned long value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%lu", value);
	return std::string(buffer, i);
}

std::string common_to_string(unsigned long long value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%llu", value);
	return std::string(buffer, i);
}

std::string common_to_string(float value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%f", value);
	return std::string(buffer, i);
}

std::string common_to_string(double value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%f", value);
	return std::string(buffer, i);
}

std::string common_to_string(long double value)
{
	char buffer[128] = {0};
	int i = sprintf(buffer, "%Lf", value);
	return std::string(buffer, i);
}

int string_to_int(const std::string& str)
{
	return atoi(str.c_str());
}

long string_to_long(const std::string& str)
{
	return atol(str.c_str());
}

double string_to_double(const std::string& str)
{
	return atof(str.c_str());
}