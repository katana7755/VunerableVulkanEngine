#pragma once

#include <string>
#include "../rapidjson/document.h"

namespace GameCore
{
	class ProjectManager
	{
	public:
		static ProjectManager& GetInstance();

	public:
		bool LoadRecentlyModifiedProject();
		bool SaveRecentlyModifiedProject(const std::string& strProjectPath);
		bool LoadProject(const std::string& strProjectPath);
		bool CreateProject(const std::string& strProjectPath);
		std::string GetResourcePath(const std::string& strResourcePath);
		std::string GetCurrentScenePath();
		bool SetCurrentScenePath(const std::string& strScenePath, std::string& strOutPath);

	private:
		bool IsProperNewProjectPath(const std::string& strProjectPath);
		std::string ToProperPath(const std::string& strInputPath);

	private:
		std::string	m_CurrentProjectPath;
		std::string m_CurrentScenePath;
	};
}