#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <iostream>
#include <atltime.h>
#include <atlstr.h>
#include <ShlObj.h>

extern std::string logFile;

#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#define LOG(msg) \
	do \
	{\
		std::ofstream fout(logFile.c_str(), std::ios::out | std::ios::app);\
		if (fout)\
		{\
			CTime curTime = CTime::GetCurrentTime();\
			CStringA str = curTime.Format(TEXT("[%Y-%m-%d %H:%M:%S] "));\
			DWORD dwThreadId = GetCurrentThreadId();\
			fout << str.GetBuffer() << dwThreadId << " " << __FILENAME__ << "(" << __LINE__ << ") " << msg << std::endl;\
			std::cout << str.GetBuffer() << dwThreadId << " " << __FILENAME__ << "(" << __LINE__ << ") " << msg << std::endl;\
			fout.close();\
		}\
	} while (false)

#endif
