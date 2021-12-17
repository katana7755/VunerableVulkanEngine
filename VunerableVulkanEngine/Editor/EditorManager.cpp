#include "EditorManager.h"
#include <algorithm>
#include "EditorInspector.h"
#include "EditorSceneBrowser.h"
#include "EditorProjectBrowser.h"
#include "EditorGameView.h"

#include "../GameCore/ProjectManager.h"
#include "../GameCore/SceneManager.h"

#ifdef _WIN32
#include <ShObjIdl.h>
#endif

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

    DrawMainMenu();
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

void EditorManager::TryToCreateNewProject()
{
#ifdef _WIN32
    while (true)
    {
        if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        {
            continue;
        }

        IFileOpenDialog* pFileDialog;

        if (!SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (LPVOID*)&pFileDialog)))
        {
            CoUninitialize();

            continue;
        }

        auto strPath = GameCore::ProjectManager::GetInstance().GetResourcePath("");
        strPath = ToWindowsCOMPath(strPath);
        IShellItem* pFolder;
        WCHAR szBuffer[MAX_PATH];
        mbstowcs(szBuffer, strPath.c_str(), MAX_PATH);
        auto result = SHCreateItemFromParsingName(szBuffer, NULL, IID_PPV_ARGS(&pFolder));

        if (!SUCCEEDED(result))
        {
            CoUninitialize();

            continue;
        }

        pFileDialog->SetDefaultFolder(pFolder);

        DWORD options;
        pFileDialog->GetOptions(&options);
        pFileDialog->SetOptions(options | FOS_PICKFOLDERS);

        if (!SUCCEEDED(pFileDialog->Show(NULL)))
        {
            CoUninitialize();

            break;
        }

        IShellItem* pItem;

        if (!SUCCEEDED(pFileDialog->GetResult(&pItem)))
        {
            CoUninitialize();

            continue;
        }

        PWSTR pszFilePath;

        if (!SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
        {
            CoUninitialize();

            continue;
        }

        CHAR chBuffer[MAX_PATH];
        wcstombs(chBuffer, pszFilePath, MAX_PATH);

        std::string strFilePath = chBuffer;

        if (GameCore::ProjectManager::GetInstance().CreateProject(strFilePath))
        {
            break;
        }
    }
#endif
}

void EditorManager::DrawMainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New")) 
            {
                ExecuteMenuCreatingNewProject();
            }

            if (ImGui::MenuItem("Load"))
            {
                ExecuteMenuLoadingProject();
            }

            if (ImGui::MenuItem("Save"))
            {
                ExecuteMenuSavingProject();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                ExecuteMenuExitingApplication();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorManager::ExecuteMenuCreatingNewProject()
{
    TryToCreateNewProject();
    GameCore::SceneManager::GetInstance().LoadEmptyScene();
}

void EditorManager::ExecuteMenuLoadingProject()
{
#ifdef _WIN32
    while (true)
    {
        if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        {
            continue;
        }

        IFileOpenDialog* pFileDialog;

        if (!SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (LPVOID*)&pFileDialog)))
        {
            CoUninitialize();

            continue;
        }

        auto strPath = GameCore::ProjectManager::GetInstance().GetResourcePath("");
        strPath = ToWindowsCOMPath(strPath);
        IShellItem* pFolder;
        WCHAR szBuffer[MAX_PATH];
        mbstowcs(szBuffer, strPath.c_str(), MAX_PATH);
        auto result = SHCreateItemFromParsingName(szBuffer, NULL, IID_PPV_ARGS(&pFolder));

        if (!SUCCEEDED(result))
        {
            CoUninitialize();

            continue;
        }

        pFileDialog->SetDefaultFolder(pFolder);

        DWORD options;
        pFileDialog->GetOptions(&options);
        pFileDialog->SetOptions(options | FOS_PICKFOLDERS);

        if (!SUCCEEDED(pFileDialog->Show(NULL)))
        {
            CoUninitialize();

            break;
        }

        IShellItem* pItem;

        if (!SUCCEEDED(pFileDialog->GetResult(&pItem)))
        {
            CoUninitialize();

            continue;
        }

        PWSTR pszFilePath;

        if (!SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
        {
            CoUninitialize();

            continue;
        }

        CHAR chBuffer[MAX_PATH];
        wcstombs(chBuffer, pszFilePath, MAX_PATH);

        std::string strFilePath = chBuffer;

        if (GameCore::ProjectManager::GetInstance().LoadProject(strFilePath))
        {
            break;
        }
    }
#endif

    GameCore::SceneManager::GetInstance().LoadRecentlyModifiedScene();
}

void EditorManager::ExecuteMenuSavingProject()
{
#ifdef _WIN32
    while (!GameCore::ProjectManager::GetInstance().IsCurrentSceneOpen())
    {
        if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        {
            continue;
        }

        IFileSaveDialog* pFileDialog;

        if (!SUCCEEDED(CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, (LPVOID*)&pFileDialog)))
        {
            CoUninitialize();

            continue;
        }

        auto strPath = GameCore::ProjectManager::GetInstance().GetResourcePath("");
        strPath = ToWindowsCOMPath(strPath);
        IShellItem* pFolder;
        WCHAR szBuffer[MAX_PATH];
        mbstowcs(szBuffer, strPath.c_str(), MAX_PATH);
        auto result = SHCreateItemFromParsingName(szBuffer, NULL, IID_PPV_ARGS(&pFolder));

        if (!SUCCEEDED(result))
        {
            CoUninitialize();

            continue;
        }

        pFileDialog->SetDefaultFolder(pFolder);

        COMDLG_FILTERSPEC fileTypes[] =
        {
            { L"scene file", L"*.scene" },
        };

        pFileDialog->SetFileTypes(1, fileTypes);

        DWORD options;
        pFileDialog->GetOptions(&options);
        pFileDialog->SetOptions(options | FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);

        if (!SUCCEEDED(pFileDialog->Show(NULL)))
        {
            CoUninitialize();

            break;
        }

        IShellItem* pItem;

        if (!SUCCEEDED(pFileDialog->GetResult(&pItem)))
        {
            CoUninitialize();

            continue;
        }

        PWSTR pszFilePath;

        if (!SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
        {
            CoUninitialize();

            continue;
        }

        CHAR chBuffer[MAX_PATH];
        wcstombs(chBuffer, pszFilePath, MAX_PATH);

        std::string strFilePath = chBuffer;

        if (strFilePath.find(".scene") >= strFilePath.size())
        {
            strFilePath = strFilePath.append(".scene");
        }

        std::string strScenePath;

        if (GameCore::SceneManager::GetInstance().SaveScene(strFilePath))
        {
            GameCore::ProjectManager::GetInstance().SetCurrentScenePath(strFilePath, strScenePath);
        }        
    }
#endif

    GameCore::SceneManager::GetInstance().SaveCurrentScene();
    GameCore::ProjectManager::GetInstance().SaveCurrentProjectSettings();
}

void EditorManager::ExecuteMenuExitingApplication()
{
    exit(0);
}

#if _WIN32
std::string EditorManager::ToWindowsCOMPath(const std::string& strInput)
{
    std::string strRet = strInput;
    size_t index = 0;

    while (true)
    {
        index = strRet.find("/", index);

        if (index >= strRet.size())
        {
            break;
        }

        strRet = strRet.replace(strRet.begin() + index, strRet.begin() + index + 1, "\\");
    }

    return strRet;
}
#endif