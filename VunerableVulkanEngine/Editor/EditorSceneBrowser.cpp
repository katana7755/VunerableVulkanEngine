#include "EditorSceneBrowser.h"
#include "EditorSelection.h"

#define MENU_NAME "Scene"
#define EDITOR_NAME "Scene Browser"

void EditorSceneBrowser::CheckBox()
{
	ImGui::Checkbox(EDITOR_NAME, &m_IsChecked);
}

void EditorSceneBrowser::DrawWhenChecked()
{
    const ImGuiTreeNodeFlags BASE_FLAGS = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    const ImGuiTreeNodeFlags SELECT_BASE_FLAGS = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected;
    const ImGuiTreeNodeFlags CHILD_FLAGS = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
    const ImGuiTreeNodeFlags SELECT_CHILD_FLAGS = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected;

	// currently this is all faked...
	ImGui::Begin(EDITOR_NAME, &m_IsChecked);
	
    int                     chunkIndex = 0;
    bool                    isMouseClicked = false;
    bool                    existChunkCandidate = false;
    bool                    existEntityCandidate = false;
    ECS::ComponentTypesKey  chunkCandidate;
    ECS::Entity             entityCandidate = ECS::Entity::INVALID_ENTITY;

    if (ImGui::IsMouseClicked(0, false))
    {
        isMouseClicked = true;
    }

    ECS::Domain::ForEach([&](ECS::ComponentArrayChunk* chunkPtr) {
        bool isChunkOpen = ImGui::TreeNodeEx((void*)(intptr_t)chunkIndex, EditorSelection::GetInstance().IsChunkSelected(chunkPtr->m_ComponentTypesKey) ? SELECT_BASE_FLAGS : BASE_FLAGS, chunkPtr->GetComponentsKeyAsString().c_str());

        if (ImGui::IsItemClicked())
        {
            existChunkCandidate = true;
            chunkCandidate = chunkPtr->m_ComponentTypesKey;
        }

        if (isChunkOpen)
        {
            for (int entityIndex = 0; entityIndex < chunkPtr->m_EntityArray.size(); ++entityIndex)
            {
                auto& entity = chunkPtr->m_EntityArray[entityIndex];
                bool isEntityOpen = ImGui::TreeNodeEx((void*)(intptr_t)entityIndex, EditorSelection::GetInstance().IsEntitySelected(entity) ? SELECT_CHILD_FLAGS : CHILD_FLAGS, "Entity %d", entity.m_Identifier);

                if (ImGui::IsItemClicked())
                {
                    existEntityCandidate = true;
                    entityCandidate = entity;
                }

                if (isEntityOpen)
                {
                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }

        ++chunkIndex;
        });

    bool isCreateChunkOpen = false;
    bool isDestroyChunkOpen = false;
    bool isCreateEntityOpen = false;
    bool isDestroyEntityOpen = false;

    if (ImGui::BeginPopupContextWindow("SceneContextMenu"))
    {
        isMouseClicked = false;
        isCreateChunkOpen = ImGui::Selectable("Create Chunk", false);

        if (EditorSelection::GetInstance().IsChunkSelectionExist())
        {
            isDestroyChunkOpen = ImGui::Selectable("Destroy Chunk", false);
            
        }
        
        if (EditorSelection::GetInstance().IsChunkSelectionExist() || EditorSelection::GetInstance().IsEntitySelectionExist())
        {
            isCreateEntityOpen = ImGui::Selectable("Create Entity", false);
        }
        
        if (EditorSelection::GetInstance().IsEntitySelectionExist())
        {
            isDestroyEntityOpen = ImGui::Selectable("Destroy Entity", false);
        }

        ImGui::EndPopup();
    }

	ImGui::End();

    if (isMouseClicked)
    {
        if (!ImGui::GetIO().KeyCtrl)
        {
            EditorSelection::GetInstance().UnselectAll();
        }

        if (existChunkCandidate)
        {
            EditorSelection::GetInstance().SelectChunk(chunkCandidate);
        }

        if (existEntityCandidate)
        {
            EditorSelection::GetInstance().SelectEntity(entityCandidate);
        }
    }

    CreateChunk(isCreateChunkOpen);
    DestroyChunk(isDestroyChunkOpen);
    CreateEntity(isCreateEntityOpen);
    DestroyEntity(isDestroyEntityOpen);
}

void EditorSceneBrowser::CreateChunk(bool isOpen)
{
    static bool s_IsOpen = false;
    static ECS::ComponentTypesKey s_ComponentTypesKey;
    static bool s_ComponentTypeCheckArray[ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT];

    if (!s_IsOpen && isOpen)
    {
        s_ComponentTypesKey.reset();
        memset(s_ComponentTypeCheckArray, 0, ECS_MAX_REGISTERED_COMPONENTTYPE_COUNT * sizeof(bool));
    }

    s_IsOpen = isOpen;

    if (s_IsOpen)
    {
        ImGui::OpenPopup("Select Components");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(430.0f, 450.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Select Components", NULL, ImGuiWindowFlags_NoResize))
    {
        if (ImGui::BeginTable("split", 1, ImGuiTableFlags_BordersOuter, ImVec2(414.0f, 360.0f)))
        {
            uint32_t count = ECS::ComponentTypeUtility::GetComponentTypeCount();

            if (count > 0)
            {
                for (uint32_t i = 0; i < count; ++i)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    auto& typeInfo = ECS::ComponentTypeUtility::GetComponentTypeInfo(i);

                    if (ImGui::Checkbox(typeInfo.m_Name.c_str() + 7, &s_ComponentTypeCheckArray[i]))
                    {
                        s_ComponentTypesKey[i] = s_ComponentTypeCheckArray[i];
                    }
                }
            }
            else
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("No registered components.");
            }

            ImGui::EndTable();
        }

        if (ECS::Domain::IsChunkExist(s_ComponentTypesKey))
        {
            ImGui::Text("Currently selected components already have its own chunk.\nUnique component set is needed to be set.");
            ImGui::Spacing();
            ImGui::SameLine(ImGui::GetWindowWidth() - 130.0f);

            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::Dummy(ImVec2(0.0f, 24.0f));
            ImGui::Spacing();
            ImGui::SameLine(ImGui::GetWindowWidth() - 270.0f);

            if (ImGui::Button("Confirm", ImVec2(120.0f, 0.0f)))
            {
                ECS::Domain::CreateChunk(s_ComponentTypesKey);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void EditorSceneBrowser::DestroyChunk(bool isOpen)
{
    static bool s_IsOpen = false;

    if (!s_IsOpen && isOpen)
    {
    }

    s_IsOpen = isOpen;

    if (!s_IsOpen)
    {
        return;
    }

    auto& componentTypesKeyArray = EditorSelection::GetInstance().GetSelectedChunks();

    for (auto& componentTypesKey : componentTypesKeyArray)
    {
        ECS::Domain::DestroyChunk(componentTypesKey);
    }

    EditorSelection::GetInstance().UnselectAllChunks();

    auto& entityArray = EditorSelection::GetInstance().GetSelectedEntities();
    size_t lastIndex = entityArray.size() - 1;

    for (size_t i = 0; i <= lastIndex; ++i)
    {
        auto& entity = entityArray[lastIndex - i];

        if (ECS::Domain::IsAliveEntity(entity))
        {
            continue;
        }

        EditorSelection::GetInstance().UnselectEntity(entity);
    }
}

void EditorSceneBrowser::CreateEntity(bool isOpen)
{
    static bool s_IsOpen = false;

    if (!s_IsOpen && isOpen)
    {
    }

    s_IsOpen = isOpen;

    if (!s_IsOpen)
    {
        return;
    }

    auto& componentTypesKeyArray = EditorSelection::GetInstance().GetSelectedChunks();

    for (auto& componentTypesKey : componentTypesKeyArray)
    {
        ECS::Domain::CreateEntity(componentTypesKey);
    }

    auto& entityArray = EditorSelection::GetInstance().GetSelectedEntities();

    for (auto& entity : entityArray)
    {
        ECS::Domain::CreateEntity(ECS::Domain::GetComponentTypesKey(entity));
    }
}

void EditorSceneBrowser::DestroyEntity(bool isOpen)
{
    static bool s_IsOpen = false;

    if (!s_IsOpen && isOpen)
    {
    }

    s_IsOpen = isOpen;

    if (!s_IsOpen)
    {
        return;
    }

    auto& entityArray = EditorSelection::GetInstance().GetSelectedEntities();

    for (auto& entity : entityArray)
    {
        ECS::Domain::DestroyEntity(entity);
    }

    EditorSelection::GetInstance().UnselectAllEntities();
}