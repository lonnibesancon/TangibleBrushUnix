#include "file.h"

#include <fstream>

bool File::exists(const std::string& fileName)
{
	std::ifstream file(fileName.c_str(), std::ios::in);
	return file.is_open(); // file is automatically closed by destructor
}

File::Buffer File::read(const std::string& fileName)
{
	std::ifstream file(fileName.c_str(),
	                   std::ios::in | std::ios::binary | std::ios::ate);

	if (!file.is_open())
		throw std::runtime_error("Unable to open file: " + fileName);

	Buffer buf;
	buf.fileName = fileName;
	buf.data = decltype(Buffer::data)(file.tellg());

	file.seekg(0, std::ios::beg);
	file.read(&buf.data[0], buf.data.size());

	return buf;
}
