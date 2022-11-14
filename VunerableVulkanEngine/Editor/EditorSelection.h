#pragma once

#include "../ECS/Domain.h"

class EditorSelection
{
public:
	static EditorSelection& GetInstance();

public:
	EditorSelection() {};

public:
	void UnselectAll();

	typedef void (*FuncForEachEntity)(const ECS::Entity& entity);

	void SelectEntity(const ECS::Entity& entity);
	void UnselectEntity(const ECS::Entity& entity);
	void UnselectAllEntities();
	bool IsEntitySelected(const ECS::Entity& entity);
	bool IsEntitySelectionExist();
	std::vector<ECS::Entity>& GetSelectedEntities() { return m_SelectedEntityArray; };

	typedef void (*FuncForEachChunk)(const ECS::ComponentTypesKey& componentTypesKey);

	void SelectChunk(const ECS::ComponentTypesKey& componentTypesKey);
	void UnselectChunk(const ECS::ComponentTypesKey& componentTypesKey);
	void UnselectAllChunks();
	bool IsChunkSelected(const ECS::ComponentTypesKey& componentTypesKey);
	bool IsChunkSelectionExist();
	std::vector<ECS::ComponentTypesKey>& GetSelectedChunks() { return m_SelectedChunkArray; };

private:
	std::vector<ECS::Entity>			m_SelectedEntityArray;
	std::vector<ECS::ComponentTypesKey> m_SelectedChunkArray;
};