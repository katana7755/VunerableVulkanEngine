#include "GameCoreLoop.h"
#include "GameCoreComponentClasses.h"
#include "../Graphics/VulkanGraphics.h"

namespace GameCore
{
	void Loop()
	{
		ECS::Domain::ExecuteSystems();
	}

	void DrawDummyRendererSystem::OnInitialize()
	{
		m_ComponentTypesKey.reset();
		m_ComponentTypesKey[(int)ECS::ComponentTypeUtility::FindComponentIndex<TransformComponent>()] = true;
		m_ComponentTypesKey[(int)ECS::ComponentTypeUtility::FindComponentIndex<DummyRendererComponent>()] = true;
	}

	void DrawDummyRendererSystem::OnExecute()
	{
		VulkanGraphics::ClearAllDummies();

		ECS::Domain::ForEach(m_ComponentTypesKey, [](ECS::Entity& entity) {
			auto transform = ECS::Domain::GetComponent<TransformComponent>(entity);
			VulkanGraphics::AddDummy(transform.m_Position, transform.m_Rotation, transform.m_Scale);
			});
	}
}