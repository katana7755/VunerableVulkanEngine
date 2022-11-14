#include "SceneManager.h"
#include "ProjectManager.h"
#include <fstream>
#include <filesystem>
#include "../ECS/Domain.h"
#include "../ECS/JsonDeserialize.h"
#include "../ECS/JsonSerialize.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/prettywriter.h"
#include "GameCoreComponentClasses.h"
#include "GameCoreLoop.h"
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

	bool SceneManager::LoadEmptyScene()
	{
		ECS::Domain::Terminate();
		ECS::ComponentTypeUtility::RegisterComponentType<TransformComponent>();
		ECS::ComponentTypeUtility::RegisterComponentType<DummyRendererComponent>();
		ECS::Domain::RegisterSystem<DrawDummyRendererSystem>(1);

		return true;
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
		ECS::ComponentTypeUtility::RegisterComponentType<DummyRendererComponent>();
		ECS::Domain::RegisterSystem<DrawDummyRendererSystem>(1);

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

	bool SceneManager::SaveScene(const std::string& strScenePath)
	{
		if (!ProjectManager::GetInstance().IsProperScenePath(strScenePath))
		{
			return false;
		}

		std::ofstream fout(strScenePath.c_str());

		if (!fout.is_open())
		{
			return false;
		}

		fout.close();

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
			auto jsonValue = rapidjson::Value();
			jsonValue.SetObject();
			ECS::JsonSerizlieChunk(jsonValue, allocator, chunkPtr);
			jsonChunkArray.PushBack(jsonValue, allocator);
			++i;
		});
		
		std::string strScenePath = ProjectManager::GetInstance().GetCurrentScenePath();
		rapidjson::StringBuffer strBuffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> prettyWriter(strBuffer);
		jsonDoc.Accept(prettyWriter);

		std::string strJson = strBuffer.GetString();

		{
			std::ofstream fout(strScenePath.c_str());

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