#pragma once

#include "GameCoreComponentClasses.h"

namespace GameCore
{
	void TransformComponent::JsonDeserialize(rapidjson::Value& jsonValue, const ECS::Entity& entity)
	{
		assert(jsonValue.HasMember("TransformComponent") && jsonValue["TransformComponent"].IsObject());

		auto jsonComponentObject = jsonValue["TransformComponent"].GetObject();
		auto componentData = TransformComponent();

		assert(jsonComponentObject.HasMember("m_Parent") && jsonComponentObject["m_Parent"].IsUint());
		componentData.m_Parent = jsonComponentObject["m_Parent"].GetUint();

		assert(jsonComponentObject.HasMember("m_Position") && jsonComponentObject["m_Position"].IsObject());
		{
			auto newJsonObject = jsonComponentObject["m_Position"].GetObject();

			assert(newJsonObject.HasMember("x") && newJsonObject["x"].IsFloat());
			componentData.m_Position.x = newJsonObject["x"].GetFloat();

			assert(newJsonObject.HasMember("y") && newJsonObject["y"].IsFloat());
			componentData.m_Position.y = newJsonObject["y"].GetFloat();

			assert(newJsonObject.HasMember("z") && newJsonObject["z"].IsFloat());
			componentData.m_Position.z = newJsonObject["z"].GetFloat();
		}

		assert(jsonComponentObject.HasMember("m_Rotation") && jsonComponentObject["m_Rotation"].IsObject());
		{
			auto newJsonObject = jsonComponentObject["m_Rotation"].GetObject();

			assert(newJsonObject.HasMember("x") && newJsonObject["x"].IsFloat());
			componentData.m_Rotation.x = newJsonObject["x"].GetFloat();

			assert(newJsonObject.HasMember("y") && newJsonObject["y"].IsFloat());
			componentData.m_Rotation.y = newJsonObject["y"].GetFloat();

			assert(newJsonObject.HasMember("z") && newJsonObject["z"].IsFloat());
			componentData.m_Rotation.z = newJsonObject["z"].GetFloat();

			assert(newJsonObject.HasMember("w") && newJsonObject["w"].IsFloat());
			componentData.m_Rotation.w = newJsonObject["w"].GetFloat();
		}

		assert(jsonComponentObject.HasMember("m_Scale") && jsonComponentObject["m_Scale"].IsObject());
		{
			auto newJsonObject = jsonComponentObject["m_Scale"].GetObject();

			assert(newJsonObject.HasMember("x") && newJsonObject["x"].IsFloat());
			componentData.m_Scale.x = newJsonObject["x"].GetFloat();

			assert(newJsonObject.HasMember("y") && newJsonObject["y"].IsFloat());
			componentData.m_Scale.y = newJsonObject["y"].GetFloat();

			assert(newJsonObject.HasMember("z") && newJsonObject["z"].IsFloat());
			componentData.m_Scale.z = newJsonObject["z"].GetFloat();
		}

		ECS::Domain::SetComponent<TransformComponent>(entity, componentData);
	}

	void TransformComponent::JsonSerialize(rapidjson::Value& jsonValue, RapidJsonAllocator& allocator, const ECS::Entity& entity)
	{
		auto componentData = ECS::Domain::GetComponent<TransformComponent>(entity);
		auto jsonComponentObject = rapidjson::Value();
		jsonComponentObject.SetObject();
		jsonComponentObject.AddMember("m_Parent", rapidjson::Value().SetUint(componentData.m_Parent), allocator);

		{
			auto newJsonObject = rapidjson::Value();
			newJsonObject.SetObject();
			newJsonObject.AddMember("x", rapidjson::Value().SetFloat(componentData.m_Position.x), allocator);
			newJsonObject.AddMember("y", rapidjson::Value().SetFloat(componentData.m_Position.y), allocator);
			newJsonObject.AddMember("z", rapidjson::Value().SetFloat(componentData.m_Position.z), allocator);
			jsonComponentObject.AddMember("m_Position", newJsonObject, allocator);
		}

		{
			auto newJsonObject = rapidjson::Value();
			newJsonObject.SetObject();
			newJsonObject.AddMember("x", rapidjson::Value().SetFloat(componentData.m_Rotation.x), allocator);
			newJsonObject.AddMember("y", rapidjson::Value().SetFloat(componentData.m_Rotation.y), allocator);
			newJsonObject.AddMember("z", rapidjson::Value().SetFloat(componentData.m_Rotation.z), allocator);
			newJsonObject.AddMember("w", rapidjson::Value().SetFloat(componentData.m_Rotation.w), allocator);
			jsonComponentObject.AddMember("m_Rotation", newJsonObject, allocator);
		}

		{
			auto newJsonObject = rapidjson::Value();
			newJsonObject.SetObject();
			newJsonObject.AddMember("x", rapidjson::Value().SetFloat(componentData.m_Scale.x), allocator);
			newJsonObject.AddMember("y", rapidjson::Value().SetFloat(componentData.m_Scale.y), allocator);
			newJsonObject.AddMember("z", rapidjson::Value().SetFloat(componentData.m_Scale.z), allocator);
			jsonComponentObject.AddMember("m_Scale", newJsonObject, allocator);
		}

		jsonValue.AddMember("TransformComponent", jsonComponentObject.GetObject(), allocator);
	}

	void DummyRendererComponent::JsonDeserialize(rapidjson::Value& jsonValue, const ECS::Entity& entity)
	{
		assert(jsonValue.HasMember("DummyRendererComponent") && jsonValue["DummyRendererComponent"].IsObject());

		auto jsonComponentObject = jsonValue["DummyRendererComponent"].GetObject();
		auto componentData = DummyRendererComponent();
		ECS::Domain::SetComponent<DummyRendererComponent>(entity, componentData);
	}

	void DummyRendererComponent::JsonSerialize(rapidjson::Value& jsonValue, RapidJsonAllocator& allocator, const ECS::Entity& entity)
	{
		auto componentData = ECS::Domain::GetComponent<DummyRendererComponent>(entity);
		auto jsonComponentObject = rapidjson::Value();
		jsonComponentObject.SetObject();
		jsonValue.AddMember("DummyRendererComponent", jsonComponentObject.GetObject(), allocator);
	}
}
