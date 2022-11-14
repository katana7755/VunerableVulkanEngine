#include "EditorInspector.h"
#include "EditorSelection.h"
#include "../GameCore/GameCoreComponentClasses.h"
#include "../ECS/Domain.h"

#define EDITOR_NAME "Inspector"

void EditorInspector::CheckBox()
{
	ImGui::Checkbox(EDITOR_NAME, &m_IsChecked);
}

void EditorInspector::DrawWhenChecked()
{
	// currently this is all faked...
	ImGui::Begin(EDITOR_NAME, &m_IsChecked);

	auto& componentTypesKeyArray = EditorSelection::GetInstance().GetSelectedChunks();
	auto& entityArray = EditorSelection::GetInstance().GetSelectedEntities();

	ECS::ComponentTypesKey componentTypesKey;
	std::vector<ECS::Entity> inoutEntityArray; // 0:input, 1 ~:output

	if (entityArray.size() == 1 && componentTypesKeyArray.size() <= 0)
	{
		// Single selection mode
		ImGui::Text("Single Selection Mode");
		componentTypesKey = ECS::Domain::GetComponentTypesKey(entityArray[0]);
		inoutEntityArray.push_back(entityArray[0]); // input
		inoutEntityArray.push_back(entityArray[0]); // output
	}
	else if (entityArray.size() > 0 || componentTypesKeyArray.size() > 0)
	{	
		// Multi selection mode
		ImGui::Text("***** Multi Selection Mode *****");
	}

	if (inoutEntityArray.size() > 1)
	{
		size_t componentCount = componentTypesKey.count();

		if (componentCount == 0)
		{
			ImGui::Text("No Components");
		}
		else
		{
			for (size_t i = 0; i < ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT; ++i)
			{
				if (componentTypesKey[i] == false)
				{
					continue;
				}
			}
		}
	}
	else
	{
		ImGui::Text("No Selection");
	}

	//if (ImGui::CollapsingHeader("Transform"))
	//{
	//	ImGui::InputFloat3("Position", m_FakePosition);
	//	ImGui::InputFloat3("Rotation", m_FakeRotation);
	//	ImGui::InputFloat3("Scale", m_FakeScale);
	//}

	//const char* RENDER_TYPES[] = { "Opaque", "Transparent" };
	//const char* RENDER_MESHES[] = { "None", "Character_Head", "Character_Body" };
	//const char* RENDER_TEXTURES[] = { "None", "Character_Head", "Character_Body"};

	//if (ImGui::CollapsingHeader("Mesh Renderer"))
	//{
	//	ImGui::Combo("Type", &m_FakeRenderType, RENDER_TYPES, IM_ARRAYSIZE(RENDER_TYPES));
	//	ImGui::Combo("Mesh", &m_FakeRenderMesh, RENDER_MESHES, IM_ARRAYSIZE(RENDER_MESHES));
	//	ImGui::Combo("Texture", &m_FakeRenderTexture, RENDER_TEXTURES, IM_ARRAYSIZE(RENDER_TEXTURES));
	//}

	ImGui::End();
}