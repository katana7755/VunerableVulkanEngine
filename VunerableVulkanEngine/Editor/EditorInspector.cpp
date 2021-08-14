#include "EditorInspector.h"

#define EDITOR_NAME "Inspector"

EditorInspector gInstance;

EditorInspector::EditorInspector()
{
	m_IsChecked = false;

	EditorManager::RegisterEditor(this);
}

EditorInspector::~EditorInspector()
{
	EditorManager::UnregisterEditor(this);
}

void EditorInspector::CheckBox()
{
	ImGui::Checkbox(EDITOR_NAME, &m_IsChecked);
}

void EditorInspector::DrawWhenChecked()
{
	if (!m_IsChecked)
	{
		return;
	}

	ImGui::Begin(EDITOR_NAME);
	ImGui::Text("This will be my inspector!!!");
	ImGui::End();
}