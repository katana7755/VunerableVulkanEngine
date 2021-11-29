#pragma once

#include <string>

namespace GameCore
{
	class SceneManager
	{
	public:
		static SceneManager& GetInstance();

	public:
		bool LoadRecentlyModifiedScene();
		bool LoadScene(const std::string& strScenePath);
		bool SaveCurrentScene();
	};
}
