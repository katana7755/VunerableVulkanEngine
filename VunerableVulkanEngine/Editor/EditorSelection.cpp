#include "EditorSelection.h"

EditorSelection g_Instance;

EditorSelection& EditorSelection::GetInstance()
{
	return g_Instance;
}

void EditorSelection::UnselectAll()
{
	UnselectAllEntities();
	UnselectAllChunks();
}

void EditorSelection::SelectEntity(const ECS::Entity& entity)
{
	m_SelectedEntityArray.push_back(entity);
}

void EditorSelection::UnselectEntity(const ECS::Entity& entity)
{
	auto findIter = std::find(m_SelectedEntityArray.begin(), m_SelectedEntityArray.end(), entity);

	if (findIter == m_SelectedEntityArray.end())
	{
		return;
	}

	m_SelectedEntityArray.erase(findIter);
}

void EditorSelection::UnselectAllEntities()
{
	m_SelectedEntityArray.clear();
}

bool EditorSelection::IsEntitySelected(const ECS::Entity& entity)
{
	return std::find(m_SelectedEntityArray.begin(), m_SelectedEntityArray.end(), entity) != m_SelectedEntityArray.end();
}

bool EditorSelection::IsEntitySelectionExist()
{
	return !m_SelectedEntityArray.empty();
}

void EditorSelection::SelectChunk(const ECS::ComponentTypesKey& componentTypesKey)
{
	m_SelectedChunkArray.push_back(componentTypesKey);
}

void EditorSelection::UnselectChunk(const ECS::ComponentTypesKey& componentTypesKey)
{
	auto findIter = std::find(m_SelectedChunkArray.begin(), m_SelectedChunkArray.end(), componentTypesKey);

	if (findIter == m_SelectedChunkArray.end())
	{
		return;
	}

	m_SelectedChunkArray.erase(findIter);
}

void EditorSelection::UnselectAllChunks()
{
	m_SelectedChunkArray.clear();
}

bool EditorSelection::IsChunkSelected(const ECS::ComponentTypesKey& componentTypesKey)
{
	return std::find(m_SelectedChunkArray.begin(), m_SelectedChunkArray.end(), componentTypesKey) != m_SelectedChunkArray.end();
}

bool EditorSelection::IsChunkSelectionExist()
{
	return !m_SelectedChunkArray.empty();
}
