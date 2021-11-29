#include "ProjectManager.h"
#include <fstream>
#include <filesystem>
#include "../rapidjson/filereadstream.h"
#include "../rapidjson/filewritestream.h"
#include "../rapidjson/prettywriter.h"
#include "../DebugUtility.h"

#ifdef _WIN32
#include <ShlObj_core.h>
#endif

namespace GameCore
{
	ProjectManager g_Instance;

	ProjectManager& ProjectManager::GetInstance()
	{
		return g_Instance;
	}

	bool ProjectManager::LoadRecentlyModifiedProject()
	{
		std::string strPath;

#ifdef _WIN32
		CHAR buffer[1024];
		
		if (SHGetSpecialFolderPath(NULL, buffer, CSIDL_LOCAL_APPDATA, TRUE) == FALSE)
		{
			printf_console("special folder cannot be found\n");

			throw;
		}

		strPath = buffer;
#else
		strPath = std::filesystem::current_path().c_str();
#endif

		strPath = ToProperPath(strPath);
		strPath += "/vveregistry.txt";

		std::string strProjectPath;
		std::ifstream fin(strPath.c_str());

		if (!fin.is_open())
		{
			return false;
		}

		std::string strBuffer;
		std::getline(fin, strBuffer);
		strProjectPath += strBuffer;
		fin.close();

		return LoadProject(strProjectPath);
	}

	bool ProjectManager::SaveRecentlyModifiedProject(const std::string& strProjectPath)
	{
		std::string strPath;

#ifdef _WIN32
		CHAR buffer[1024];

		if (SHGetSpecialFolderPath(NULL, buffer, CSIDL_LOCAL_APPDATA, TRUE) == FALSE)
		{
			printf_console("special folder cannot be found\n");

			throw;
		}

		strPath = buffer;
#else
		strPath = std::filesystem::current_path().c_str();
#endif

		strPath = ToProperPath(strPath);
		strPath += "/vveregistry.txt";

		std::ofstream fout(strPath.c_str());

		if (!fout.is_open())
		{
			return false;
		}

		fout << strProjectPath << std::endl;
		fout.close();

		return true;
	}

	bool ProjectManager::LoadProject(const std::string& strProjectPath)
	{
		std::string strPath = ToProperPath(strProjectPath);
		std::string strProjectFilePath = strPath + "/ProjectSettings.json";
		std::string strJson;
		std::ifstream fin(strProjectFilePath.c_str());

		if (!fin.is_open())
		{
			return false;
		}

		fin.close();
		SaveRecentlyModifiedProject(strPath);

		return true;
	}

	bool ProjectManager::CreateProject(const std::string& strProjectPath)
	{
		std::string strPath = ToProperPath(strProjectPath);

		if (!IsProperNewProjectPath(strPath))
		{
			return false;
		}

		rapidjson::Document jsonDoc(rapidjson::kObjectType);
		auto& allocator = jsonDoc.GetAllocator();
		jsonDoc.AddMember("m_CurrentProjectPath", rapidjson::Value().SetString(strPath.c_str(), strPath.length()), allocator);
		jsonDoc.AddMember("m_CurrentScenePath", rapidjson::Value().SetString("", 0), allocator);

		std::string strProjectFilePath = strPath + "/ProjectSettings.json";
		std::ofstream fout(strProjectFilePath.c_str());

		if (!fout.is_open())
		{
			return false;
		}

		rapidjson::StringBuffer strBuffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> prettyWriter(strBuffer);
		jsonDoc.Accept(prettyWriter);
		fout.write(strBuffer.GetString(), strBuffer.GetSize());
		fout.close();
		SaveRecentlyModifiedProject(strPath);

		return true;
	}

	std::string ProjectManager::GetCurrentScenePath()
	{
		return m_CurrentScenePath;
	}

	std::string ProjectManager::GetResourcePath(const std::string& strResourcePath)
	{
		return m_CurrentProjectPath + "/" + strResourcePath;
	}

	bool ProjectManager::SetCurrentScenePath(const std::string& strScenePath, std::string& strOutPath)
	{
		std::string strPath = ToProperPath(strScenePath);
		std::filesystem::path fPath(GetResourcePath(strPath));

		if (!std::filesystem::exists(fPath))
		{
			return false;
		}

		m_CurrentScenePath = strPath;
		strOutPath = strPath;

		return true;
	}

	bool ProjectManager::IsProperNewProjectPath(const std::string& strProjectPath)
	{
		std::filesystem::path fPath(strProjectPath);

		if (!std::filesystem::exists(fPath))
		{
			return true;
		}

		std::filesystem::recursive_directory_iterator iter(fPath);

		if (iter != std::filesystem::end(iter))
		{
			return false;
		}

		return true;
	}

	std::string ProjectManager::ToProperPath(const std::string& strInputPath)
	{
		std::string strPath = strInputPath;

		{
			std::string strInput = "\\\\";
			auto iter = strPath.find(strInput);

			while (iter != std::string::npos)
			{
				strPath = strPath.replace(iter, strInput.size(), "/");
				iter = strPath.find(strInput);
			}
		}

		{
			std::string strInput = "\\";
			auto iter = strPath.find(strInput);

			while (iter != std::string::npos)
			{
				strPath = strPath.replace(iter, strInput.size(), "/");
				iter = strPath.find(strInput);
			}
		}

		return strPath;
	}
}