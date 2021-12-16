#include "EditorManager.h"
#include <algorithm>
#include "EditorInspector.h"
#include "EditorSceneBrowser.h"
#include "EditorProjectBrowser.h"
#include "EditorGameView.h"

EditorBase::EditorBase()
{
    m_IsChecked = true;
}

EditorBase::~EditorBase()
{
}

EditorManager g_Instance;

EditorManager& EditorManager::GetInstance()
{
    return g_Instance;
}

EditorManager::EditorManager()
{
    RegisterEditor(new EditorInspector());
    RegisterEditor(new EditorSceneBrowser());
    RegisterEditor(new EditorProjectBrowser());
    RegisterEditor(new EditorGameView());
}

EditorManager::~EditorManager()
{
    for (auto editorPtr : m_RegisteredEditors)
    {
        delete editorPtr;
    }

    m_RegisteredEditors.clear();
}

void EditorManager::DrawEditors()
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

    bool test = true;
    ImGui::ShowDemoWindow(&test);
}

void EditorManager::RegisterEditor(EditorBase* editorPtr)
{
    auto it = std::find(m_RegisteredEditors.begin(), m_RegisteredEditors.end(), editorPtr);

    if (it != m_RegisteredEditors.end())
    {
        return;
    }

    m_RegisteredEditors.push_back(editorPtr);
}
