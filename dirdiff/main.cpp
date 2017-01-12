#include <iostream>
#include "md5file.h"

int main(int argc, char** argv)
{
	if (argc != 7)
	{
		std::cout << "Error: Invalid parameters. Usage: dirdiff -s sourcedir -t targetdir -o outputdir" << std::endl;
		return -1;
	}


	int i = 1;
	while (i < 7)
	{
		if (strcmp(argv[1], "-s") == 0)
		{

		}
	}

	std::cout << getFileMD5("F:\\work\\cocall-all\\client_win\\cocall\\Debug\\about_dlg.obj") << std::endl;

	system("pause");

	return 0;
}