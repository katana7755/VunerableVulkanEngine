#pragma once

#include "../ECS/Domain.h"

namespace GameCore
{
	void Loop();

	class DrawDummyRendererSystem : public ECS::SystemBase
	{
	public:
		virtual void OnInitialize() override;
		virtual void OnExecute() override;

	private:
		ECS::ComponentTypesKey m_ComponentTypesKey;
	};
}