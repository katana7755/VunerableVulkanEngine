#include "EditorSceneBrowser.h"

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
        bool isChunkOpen = ImGui::TreeNodeEx((void*)(intptr_t)chunkIndex, IsSelectedChunk(chunkPtr->m_ComponentTypesKey) ? SELECT_BASE_FLAGS : BASE_FLAGS, chunkPtr->GetComponentsKeyAsString().c_str());

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
                bool isEntityOpen = ImGui::TreeNodeEx((void*)(intptr_t)entityIndex, IsSelectedEntity(entity) ? SELECT_CHILD_FLAGS : CHILD_FLAGS, "Entity %d", entity.m_Identifier);

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

    //static bool test_drag_and_drop = false;
    //static int selection_mask = (1 << 2);
    //int node_clicked = -1;

    //for (int i = 0; i < 6; i++)
    //{
    //    // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
    //    ImGuiTreeNodeFlags node_flags = base_flags;
    //    const bool is_selected = (selection_mask & (1 << i)) != 0;

    //    if (is_selected)
    //        node_flags |= ImGuiTreeNodeFlags_Selected;

    //    if (i < 3)
    //    {
    //        // Items 0..2 are Tree Node
    //        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, "Selectable Node %d", i);

    //        if (ImGui::IsItemClicked())
    //            node_clicked = i;

    //        if (test_drag_and_drop && ImGui::BeginDragDropSource())
    //        {
    //            ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
    //            ImGui::Text("This is a drag and drop source");
    //            ImGui::EndDragDropSource();
    //        }

    //        if (node_open)
    //        {
    //            ImGui::BulletText("Blah blah\nBlah Blah");
    //            ImGui::TreePop();
    //        }
    //    }
    //    else
    //    {
    //        // Items 3..5 are Tree Leaves
    //        // The only reason we use TreeNode at all is to allow selection of the leaf. Otherwise we can
    //        // use BulletText() or advance the cursor by GetTreeNodeToLabelSpacing() and call Text().
    //        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet
    //        ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, "Selectable Leaf %d", i);

    //        if (ImGui::IsItemClicked())
    //            node_clicked = i;

    //        if (test_drag_and_drop && ImGui::BeginDragDropSource())
    //        {
    //            ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
    //            ImGui::Text("This is a drag and drop source");
    //            ImGui::EndDragDropSource();
    //        }
    //    }
    //}

    //if (node_clicked != -1)
    //{
    //    // Update selection state
    //    // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
    //    if (ImGui::GetIO().KeyCtrl)
    //        selection_mask ^= (1 << node_clicked);          // CTRL+click to toggle
    //    else //if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
    //        selection_mask = (1 << node_clicked);           // Click to single-select
    //}

    bool isCreateChunkOpen = false;
    bool isDestroyChunkOpen = false;
    bool isCreateEntityOpen = false;
    bool isDestroyEntityOpen = false;

    if (ImGui::BeginPopupContextWindow("SceneContextMenu"))
    {
        isMouseClicked = false;
        isCreateChunkOpen = ImGui::Selectable("Create Chunk", false);

        if (!m_SelectedChunkArray.empty())
        {
            isDestroyChunkOpen = ImGui::Selectable("Destroy Chunk", false);
        }
        
        if (!(m_SelectedChunkArray.empty() && m_SelectedEntityArray.empty()))
        {
            isCreateEntityOpen = ImGui::Selectable("Create Entity", false);
        }
        
        if (!m_SelectedEntityArray.empty())
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
            ClearAllSelections();
        }

        if (existChunkCandidate)
        {
            m_SelectedChunkArray.push_back(chunkCandidate);
        }

        if (existEntityCandidate)
        {
            m_SelectedEntityArray.push_back(entityCandidate);
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

    for (uint32_t i = 0; i < m_SelectedChunkArray.size(); ++i)
    {
        ECS::Domain::DestroyChunk(m_SelectedChunkArray[i]);
    }

    m_SelectedChunkArray.clear();

    for (uint32_t i = m_SelectedEntityArray.size(); i > 0; --i)
    {
        auto entity = m_SelectedEntityArray[i - 1];

        if (!ECS::Domain::IsAliveEntity(entity))
        {
            m_SelectedEntityArray.erase(m_SelectedEntityArray.begin() + i - 1);
        }
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

    for (auto& componentTypesKey : m_SelectedChunkArray)
    {
        ECS::Domain::CreateEntity(componentTypesKey);
    }

    for (auto& entity : m_SelectedEntityArray)
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

    for (auto entity : m_SelectedEntityArray)
    {
        ECS::Domain::DestroyEntity(entity);
    }    

    m_SelectedEntityArray.clear();
}

void EditorSceneBrowser::ClearAllSelections()
{
    m_SelectedChunkArray.clear();
    m_SelectedEntityArray.clear();
}

bool EditorSceneBrowser::IsSelectedChunk(const ECS::ComponentTypesKey& componentTypesKey)
{
    return std::find(m_SelectedChunkArray.begin(), m_SelectedChunkArray.end(), componentTypesKey) != m_SelectedChunkArray.end();
}

bool EditorSceneBrowser::IsSelectedEntity(const ECS::Entity& entity)
{
    return std::find(m_SelectedEntityArray.begin(), m_SelectedEntityArray.end(), entity) != m_SelectedEntityArray.end();
}