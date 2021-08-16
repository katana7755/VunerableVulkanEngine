#include "EditorManager.h"
#include <algorithm>
#include "EditorInspector.h"
#include "EditorSceneBrowser.h"
#include "EditorProjectBrowser.h"

EditorManager EditorManager::s_Instance;
EditorManager* EditorManager::s_UniquePtr = &EditorManager::s_Instance;

EditorBase::EditorBase()
{
    m_IsChecked = false;
}

EditorBase::~EditorBase()
{
}

EditorManager::EditorManager()
{
    RegisterEditorInternal(new EditorInspector());
    RegisterEditorInternal(new EditorSceneBrowser());
    RegisterEditorInternal(new EditorProjectBrowser());
}

EditorManager::~EditorManager()
{
    for (auto editorPtr : m_RegisteredEditors)
    {
        delete editorPtr;
    }

    m_RegisteredEditors.clear();
}

void EditorManager::DrawEditorsInternal()
{
    ImGui::Begin("All editors");

    for (auto it : m_RegisteredEditors)
    {
        it->CheckBox();
    }

    ImGui::End();

    for (auto it : m_RegisteredEditors)
    {
        if (it->IsChecked())
        {
            it->DrawWhenChecked();
        }        
    }    

    //bool test = true;
    //ImGui::ShowDemoWindow(&test);
}

void EditorManager::RegisterEditorInternal(EditorBase* editorPtr)
{
    auto it = std::find(m_RegisteredEditors.begin(), m_RegisteredEditors.end(), editorPtr);

    if (it != m_RegisteredEditors.end())
    {
        return;
    }

    m_RegisteredEditors.push_back(editorPtr);
}
