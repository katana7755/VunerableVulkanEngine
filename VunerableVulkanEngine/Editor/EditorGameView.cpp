#include "EditorGameView.h"
#include "../IMGUI/imgui.h"
#include "../IMGUI/imgui_impl_vulkan.h"
#include "../DebugUtility.h"

#define EDITOR_NAME "Game View"

EditorGameView* EditorGameView::s_UniquePtr = NULL;

void EditorGameView::CheckBox()
{
	ImGui::Checkbox(EDITOR_NAME, &m_IsChecked);
}

void UpdatePipelineAndDescriptorSet(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	ImGui_ImplVulkan_ChangeDescriptorSet(cmd->UserCallbackData);
}

void ResetPipelineAndDescriptorSet(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	ImGui_ImplVulkan_ResetDescriptorSet();
}

void EditorGameView::DrawWhenChecked()
{
	// currently this is all faked...
	ImGui::Begin(EDITOR_NAME, &m_IsChecked);

	if (m_TextureID != NULL)
	{
		auto size = ImGui::GetWindowSize();
		size.x -= 32.0f;
		size.y -= 32.0f;

		ImGui::GetWindowDrawList()->AddCallback(UpdatePipelineAndDescriptorSet, m_DescriptorSet);
		ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, NULL);
		ImGui::Image(m_TextureID, size);
		ImGui::GetWindowDrawList()->AddCallback(ResetPipelineAndDescriptorSet, NULL);
		ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, NULL);
	}

	ImGui::End();
}

void EditorGameView::SetTextureInternal(ImTextureID textureID, ImTextureID descriptorSet)
{
	m_TextureID = textureID;
	m_DescriptorSet = descriptorSet;
}