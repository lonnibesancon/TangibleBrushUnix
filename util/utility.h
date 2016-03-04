#ifndef UTILITY_H
#define UTILITY_H

#include "global.h"

#include <string>
#include <sstream>
#include <fstream>

namespace Utility {
	template <typename T>
	inline T fromString(const std::string& str)
	{
		std::istringstream iss(str);
		T result;
		iss >> result;
		return result;
	}

	template <typename T>
	inline T fromString(const std::string& str, bool& ok)
	{
		std::istringstream iss(str);
		T result;
		ok = (iss >> result);
		return result;
	}

	template <typename T>
	inline T fromStringExcept(const std::string& str)
	{
		std::istringstream iss(str);
		iss.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		T result;
		iss >> result;
		return result;
	}

	template <typename T>
	inline std::string toString(const T& value)
	{
		std::ostringstream oss;
		oss << value;
		return oss.str();
	}

	inline std::vector<std::string> split(const std::string& str, char sep)
	{
		std::vector<std::string> result;
		std::istringstream iss(str);
		std::string sub;
		while (std::getline(iss, sub, sep))
			result.push_back(sub);
		return result;
	}

} // namespace

#endif /* UTILITY_H */
