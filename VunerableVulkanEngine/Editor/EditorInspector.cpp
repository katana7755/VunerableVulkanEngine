#include "EditorInspector.h"

#define EDITOR_NAME "Inspector"

void EditorInspector::CheckBox()
{
	ImGui::Checkbox(EDITOR_NAME, &m_IsChecked);
}

void EditorInspector::DrawWhenChecked()
{
	// currently this is all faked...
	ImGui::Begin(EDITOR_NAME, &m_IsChecked);

	if (ImGui::CollapsingHeader("Transform"))
	{
		ImGui::InputFloat3("Position", m_FakePosition);
		ImGui::InputFloat3("Rotation", m_FakeRotation);
		ImGui::InputFloat3("Scale", m_FakeScale);
	}

	const char* RENDER_TYPES[] = { "Opaque", "Transparent" };
	const char* RENDER_MESHES[] = { "None", "Character_Head", "Character_Body" };
	const char* RENDER_TEXTURES[] = { "None", "Character_Head", "Character_Body"};

	if (ImGui::CollapsingHeader("Mesh Renderer"))
	{
		ImGui::Combo("Type", &m_FakeRenderType, RENDER_TYPES, IM_ARRAYSIZE(RENDER_TYPES));
		ImGui::Combo("Mesh", &m_FakeRenderMesh, RENDER_MESHES, IM_ARRAYSIZE(RENDER_MESHES));
		ImGui::Combo("Texture", &m_FakeRenderTexture, RENDER_TEXTURES, IM_ARRAYSIZE(RENDER_TEXTURES));
	}

	ImGui::End();
}