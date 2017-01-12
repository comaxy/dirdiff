#include "md5file.h"
#include "log.h"
#include <iostream>
#include <list>
#include <algorithm>
#include <string>
#include <iterator>
#include <sstream>
#include <fstream>
#include <exception>

class FindData
{
public:
	FindData(const std::string& baseDir, const std::string& path, bool isDirectory)
		: m_baseDir(baseDir), m_path(path), m_isDirectory(isDirectory) 
	{
		auto pos = m_path.find(m_baseDir);
		m_relativePath = m_path.substr(pos + m_baseDir.length());
	}

	bool isDirectory() const { return m_isDirectory; }
	const std::string& path() const { return m_path; }
	const std::string relativePath() const { return m_relativePath; }

private:
	std::string m_baseDir;
	std::string m_path;
	std::string m_relativePath;
	bool m_isDirectory;
};

bool operator < (const FindData& lhs, const FindData& rhs)
{
	return lhs.relativePath() < rhs.relativePath();
}

std::ostream& operator << (std::ostream& out, const FindData& fd)
{
	return out << (fd.isDirectory() ? "Directory: " : "File: ") << fd.relativePath();
}

std::string logFile;

static void findFile(const std::string& baseDir, const std::string& directory, std::list<FindData>& result);

int main(int argc, char** argv)
{
	try
	{
		// Init log
		CTime curTime = CTime::GetCurrentTime();
		CStringA str = curTime.Format(TEXT("%Y%m%d%H%M%S"));
		str = "update" + str + ".log";
		logFile = static_cast<LPCSTR>(str);

		// get directories from command line.
		const std::string usage = "Usage: dirdiff -s sourcedir -t targetdir -o outputdir";
		if (argc != 7)
		{
			LOG("Error: Invalid parameters. " << usage);
			return -1;
		}
		std::string sourcedir;
		std::string targetdir;
		std::string outputdir;
		int i = 0;
		while (++i < 7)
		{
			if (strcmp(argv[i], "-s") == 0)
			{
				sourcedir = argv[++i];
			}
			else if (strcmp(argv[i], "-t") == 0)
			{
				targetdir = argv[++i];
			}
			else if (strcmp(argv[i], "-o") == 0)
			{
				outputdir = argv[++i];
			}
			else
			{
				LOG("Error: Invalid parameters. " << usage);
			}
		}
		LOG("source directory: " << sourcedir);
		LOG("target directory: " << targetdir);
		LOG("output directory: " << outputdir);

		// iterate directories, find differences
		LOG("Finding all files and directories in directory: " << sourcedir << "...");
		std::list<FindData> sourceFileList;
		findFile(sourcedir, sourcedir, sourceFileList);
		LOG("Find done! Total: " << sourceFileList.size());
		LOG("Sorting all files in source file list...");
		sourceFileList.sort();
		//std::for_each(sourceFileList.begin(), sourceFileList.end(), [](const auto& fd) { LOG(fd); });
		LOG("Sort done!");

		LOG("Finding all files and directories in directory: " << targetdir << "...");
		std::list<FindData> targetFileList;
		findFile(targetdir, targetdir, targetFileList);
		LOG("Find done! Total: " << targetFileList.size());
		LOG("Sorting all files in target file list...");
		targetFileList.sort();
		//std::for_each(targetFileList.begin(), targetFileList.end(), [](const auto& fd) { LOG(fd); });
		LOG("Sort done!");

		LOG("Finding modified files...");
		std::list<FindData> intersection;
		std::set_intersection(targetFileList.begin(), targetFileList.end(),
			sourceFileList.begin(), sourceFileList.end(),
			std::back_inserter(intersection));
		//LOG("Intersection set of target file list and source file list: ");
		//std::for_each(intersection.begin(), intersection.end(), [](const auto& fd) { LOG(fd); });
		std::list<FindData> modifiedFiles;
		std::for_each(intersection.begin(), intersection.end(), [targetdir, sourcedir, &modifiedFiles](const auto& fd) {
			if (!fd.isDirectory())
			{
				std::string targetFilePath = targetdir + fd.relativePath();
				std::string targetFileMD5 = getFileMD5(targetFilePath);
				std::string sourceFilePath = sourcedir + fd.relativePath();
				std::string sourceFileMD5 = getFileMD5(sourceFilePath);
				if (targetFileMD5 != sourceFileMD5)
				{
					modifiedFiles.push_back(fd);
				}
			}
		});
		LOG("Find done! Total: " << modifiedFiles.size());
		LOG("Copying modified files...");
		auto copyProcedure = [targetdir, outputdir](const auto& fd) {
			//LOG(fd);

			// Create directory if need
			auto pos = fd.relativePath().find_first_of('\\');
			while (pos != std::string::npos)
			{
				std::string dir = outputdir + fd.relativePath().substr(0, pos + 1);
				if (!PathFileExistsA(dir.c_str()))
				{
					if (!CreateDirectoryA(dir.c_str(), NULL))
					{
						LOG("FATAL ERROR: " << dir << " can't be created");
						std::ostringstream oss;
						oss << "FATAL ERROR: " << dir << " can't be created" << std::ends;
						throw std::runtime_error(oss.str());
					}
				}
				pos = fd.relativePath().find_first_of('\\', pos + 1);
			}

			// copy file
			std::string targetFilePath = targetdir + fd.relativePath();
			std::string outputFilePath = outputdir + fd.relativePath();
			if (!CopyFileA(targetFilePath.c_str(), outputFilePath.c_str(), FALSE))
			{
				LOG("FATAL ERROR: Copy " << targetFilePath << " to " << outputFilePath << " failed!");
				std::ostringstream oss;
				oss << "FATAL ERROR: Copy " << targetFilePath << " to " << outputFilePath << " failed!" << std::ends;
				throw std::runtime_error(oss.str());
			}
		};
		std::for_each(modifiedFiles.begin(), modifiedFiles.end(), copyProcedure);
		LOG("Copy done!");
	
		LOG("Finding added files...");
		std::list<FindData> differenceT2S;
		std::set_difference(targetFileList.begin(), targetFileList.end(),
			sourceFileList.begin(), sourceFileList.end(),
			std::back_inserter(differenceT2S));
		LOG("Find done! Total: " << differenceT2S.size());
		LOG("Copying added files...");
		std::for_each(differenceT2S.begin(), differenceT2S.end(), copyProcedure);
		LOG("Copy done!");

		LOG("Finding deleted files...");
		std::list<FindData> differenceS2T;
		std::set_difference(sourceFileList.begin(), sourceFileList.end(),
			targetFileList.begin(), targetFileList.end(),
			std::back_inserter(differenceS2T));
		LOG("Find done! Total: " << differenceS2T.size());
		LOG("Writing deleted file paths to file...");
		std::string deletedFileLogPath = outputdir + "\\deletedfile.txt";
		std::ofstream fout(deletedFileLogPath, std::ofstream::out);
		if (!fout)
		{
			LOG(deletedFileLogPath << " can't be opened");
			std::ostringstream oss;
			oss << deletedFileLogPath << " can't be opened" << std::ends;
			throw std::runtime_error(oss.str());
		}
		std::for_each(differenceS2T.begin(), differenceS2T.end(), [outputdir, &fout](const auto& fd) { 
			fout << (fd.isDirectory() ? "Directory: " : "File: ") << fd.relativePath() << std::endl;
		});
		fout.close();
		LOG("Write done!");

		LOG("All done!");

		system("pause");

		return 0;
	}
	catch (const std::runtime_error& error)
	{
		LOG(error.what());
		return -1;
	}
	catch (...)
	{
		LOG("FATAL ERROR: Unknown error");
		return -1;
	}
}

void findFile(const std::string& baseDir, const std::string& directory, std::list<FindData>& result)
{
	std::string findFiles = directory + "\\*";

	WIN32_FIND_DATAA fd = { 0 };
	HANDLE hFind = FindFirstFileA(findFiles.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		BOOL bFound = TRUE;
		while (bFound)
		{
			if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0)
			{
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					std::string subDirectory = directory + "\\" + fd.cFileName;
					//LOG("directory found: " << subDirectory);
					result.push_back(FindData(baseDir, subDirectory, true));
					findFile(baseDir, subDirectory, result);
				}
				else
				{
					std::string filePath = directory + "\\" + fd.cFileName;
					//LOG("file found: " << filePath);
					result.push_back(FindData(baseDir, filePath, false));
				}	
			}

			bFound = FindNextFileA(hFind, &fd);
		}

		FindClose(hFind);
	}
}
