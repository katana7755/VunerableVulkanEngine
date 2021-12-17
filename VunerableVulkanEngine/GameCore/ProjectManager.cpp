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
		m_CurrentProjectPath = strProjectPath;

		return true;
	}

	bool ProjectManager::LoadProject(const std::string& strProjectPath)
	{
		std::string strPath = ToProperPath(strProjectPath);
		SaveRecentlyModifiedProject(strPath);

		std::string strProjectFilePath = strPath + "/ProjectSettings.json";
		std::string strJson;
		std::ifstream fin(strProjectFilePath.c_str());

		if (!fin.is_open())
		{
			return false;
		}

		for (std::string strBuffer; std::getline(fin, strBuffer);)
		{
			strJson += strBuffer;
		}

		fin.close();

		rapidjson::Document jsonDoc;
		jsonDoc.Parse(strJson.c_str());

		if (!jsonDoc.HasMember("m_CurrentProjectPath"))
		{
			return false;
		}

		if (!jsonDoc.HasMember("m_CurrentScenePath"))
		{
			return false;
		}

		m_CurrentProjectPath = jsonDoc["m_CurrentProjectPath"].GetString();
		m_CurrentScenePath = jsonDoc["m_CurrentScenePath"].GetString();

		return true;
	}

	bool ProjectManager::CreateProject(const std::string& strProjectPath)
	{
		std::string strPath = ToProperPath(strProjectPath);

		if (!IsProperNewProjectPath(strPath))
		{
			return false;
		}

		if (!SaveRecentlyModifiedProject(strPath))
		{
			return false;
		}

		if (!SaveCurrentProjectSettings())
		{
			return false;
		}

		return true;
	}

	std::string ProjectManager::GetCurrentScenePath()
	{
		return GetResourcePath(m_CurrentScenePath);
	}

	std::string ProjectManager::GetResourcePath(const std::string& strResourcePath)
	{
		return m_CurrentProjectPath + "/" + strResourcePath;
	}

	bool ProjectManager::SetCurrentScenePath(const std::string& strScenePath, std::string& strOutPath)
	{
		if (!IsProperScenePath(strScenePath))
		{
			return false;
		}

		std::string strPath = ToProperPath(strScenePath);
		std::filesystem::path fPath(strPath);

		if (!std::filesystem::exists(fPath))
		{
			return false;
		}

		size_t findIndex = strPath.find(m_CurrentProjectPath);
		strPath = strPath.replace(strPath.begin() + findIndex, strPath.begin() + findIndex + m_CurrentProjectPath.size(), "");
		findIndex = 0;

		while (strPath.at(findIndex) == '/')
		{
			++findIndex;
		}

		if (findIndex > 0)
		{
			strPath = strPath.substr(findIndex, strPath.size() - findIndex);
		}

		m_CurrentScenePath = strPath;
		strOutPath = strPath;

		return true;
	}

	bool ProjectManager::IsCurrentSceneOpen()
	{
		return IsProperScenePath(GetCurrentScenePath());
	}

	bool ProjectManager::IsProperScenePath(const std::string& strScenePath)
	{
		if (strScenePath.find(".scene") != strScenePath.size() - 6)
		{
			return false;
		}

		std::string strPath = ToProperPath(strScenePath);

		if (strPath.find(m_CurrentProjectPath) >= strPath.size())
		{
			return false;
		}

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

	bool ProjectManager::SaveCurrentProjectSettings()
	{
		rapidjson::Document jsonDoc(rapidjson::kObjectType);
		auto& allocator = jsonDoc.GetAllocator();
		jsonDoc.AddMember("m_CurrentProjectPath", rapidjson::Value().SetString(m_CurrentProjectPath.c_str(), m_CurrentProjectPath.size()), allocator);
		jsonDoc.AddMember("m_CurrentScenePath", rapidjson::Value().SetString(m_CurrentScenePath.c_str(), m_CurrentScenePath.size()), allocator);

		std::string strProjectFilePath = m_CurrentProjectPath + "/ProjectSettings.json";
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

		return true;
	}
}