#include "md5file.h"
#include "md5global.h"
#include "md5.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <iomanip>

std::string getFileMD5(const std::string& filename)
{
	std::ifstream fin(filename.c_str(), std::ifstream::in | std::ifstream::binary);
	if (fin)
	{
		// get length of file:
		fin.seekg(0, fin.end);
		unsigned int length = static_cast<int>(fin.tellg());
		fin.seekg(0, fin.beg);

		std::unique_ptr<unsigned char[]> buffer(new unsigned char[length]{});
		fin.read(reinterpret_cast<char*>(buffer.get()), length);
		if (!fin)
		{
			std::cout << filename << " can't be read" << std::endl;
			fin.close();
			return "";
		}
		fin.close();

		MD5_CTX context;
		MD5Init(&context);
		MD5Update(&context, buffer.get(), length);
		unsigned char digest[16];
		MD5Final(digest, &context);

		std::ostringstream oss;
		for (int i = 0; i < 16; ++i)
		{
			oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(digest[i]);
		}
		oss << std::endl;

		return std::move(oss.str());
	}
	else
	{
		std::cout << filename << " can't be opened" << std::endl;
		return "";
	}
}