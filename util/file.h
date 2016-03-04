#ifndef FILE_H
#define FILE_H

#include "global.h"

namespace File
{
	struct Buffer
	{
		std::string fileName;
		std::vector<char> data;
	};

	bool exists(const std::string& fileName);
	Buffer read(const std::string& fileName);
}

#endif /* FILE_H */
