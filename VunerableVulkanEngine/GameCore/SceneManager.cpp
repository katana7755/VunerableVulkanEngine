#include "SceneManager.h"
#include "ProjectManager.h"
#include <fstream>
#include "../ECS/Domain.h"
#include "../ECS/JsonDeserialize.h"
#include "../ECS/JsonSerialize.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/prettywriter.h"
#include "GameCoreComponentClasses.h"
#include "../DebugUtility.h"

namespace GameCore
{
	SceneManager g_Instance;

	SceneManager& SceneManager::GetInstance()
	{
		return g_Instance;
	}

	bool SceneManager::LoadRecentlyModifiedScene()
	{
		return LoadScene(ProjectManager::GetInstance().GetCurrentScenePath());
	}

	bool SceneManager::LoadScene(const std::string& strScenePath)
	{
		std::string strPath;

		if (!ProjectManager::GetInstance().SetCurrentScenePath(strScenePath, strPath))
		{
			return false;
		}

		ECS::Domain::Terminate();
		ECS::ComponentTypeUtility::RegisterComponentType<TransformComponent>();

		std::string strResourcePath = ProjectManager::GetInstance().GetResourcePath(strPath);
		std::string strJson;

		{
			std::ifstream fin(strResourcePath.c_str());

			if (!fin.is_open())
			{
				return false;
			}

			std::string strBuffer;

			while (!fin.eof())
			{
				std::getline(fin, strBuffer);
				strJson += strBuffer;
				strJson += '\n';
			}

			fin.close();
		}

		rapidjson::Document jsonDoc(rapidjson::kObjectType);
		jsonDoc.Parse(strJson.c_str());
		
		if (!jsonDoc["m_ChunkArray"].IsArray())
		{
			return false;
		}

		auto jsonChunkArray = jsonDoc["m_ChunkArray"].GetArray();
		
		for (uint32_t i = 0; i < jsonChunkArray.Size(); ++i)
		{
			auto& jsonValue = jsonChunkArray[0];

			if (!jsonValue.IsObject())
			{
				continue;
			}

			auto jsonObject = jsonValue.GetObject();
			ECS::JsonDeserizlieChunk(jsonObject);
		}

		return true;
	}

	bool SceneManager::SaveCurrentScene()
	{
		rapidjson::Document jsonDoc(rapidjson::kObjectType);
		auto& allocator = jsonDoc.GetAllocator();
		jsonDoc.AddMember("m_ChunkArray", rapidjson::Value().SetArray(), allocator);

		auto& jsonChunkArray = jsonDoc["m_ChunkArray"].SetArray();
		uint32_t i = 0;

		ECS::Domain::ForEach([&](ECS::ComponentArrayChunk* chunkPtr) {
			auto& jsonValue = jsonChunkArray[i].SetObject();
			auto jsonObject = jsonValue.GetObject();
			ECS::JsonSerizlieChunk(jsonObject, allocator, chunkPtr);
			++i;
		});
		
		std::string strScenePath = ProjectManager::GetInstance().GetCurrentScenePath();
		std::string strResourcePath = ProjectManager::GetInstance().GetResourcePath(strScenePath);
		rapidjson::StringBuffer strBuffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> prettyWriter(strBuffer);
		jsonDoc.Accept(prettyWriter);

		std::string strJson = strBuffer.GetString();

		{
			std::ofstream fout(strResourcePath.c_str());

			if (!fout.is_open())
			{
				return false;
			}

			fout.write(strBuffer.GetString(), strBuffer.GetSize());
			fout.close();
		}

		return true;
	}
}