#pragma once

#include "../IMGUI/imgui.h"
#include <vector>
#include <memory>
#include <string>

class EditorBase
{
public:
	EditorBase();
	~EditorBase();

public:
	bool IsChecked()
	{
		return m_IsChecked;
	}

public:
	virtual void CheckBox() = 0;
	virtual void DrawWhenChecked() = 0;

protected:
	bool m_IsChecked;
};

class EditorManager
{
public:
	static EditorManager& GetInstance();

public:
	EditorManager();
	~EditorManager();

public:
	void DrawEditors();
	void RegisterEditor(EditorBase* editorPtr);
	void TryToCreateNewProject();

private:
	void DrawMainMenu();
	void ExecuteMenuCreatingNewProject();
	void ExecuteMenuLoadingProject();
	void ExecuteMenuSavingProject();

#if _WIN32
	std::string ToWindowsCOMPath(const std::string& strInput);
#endif

private:
	std::vector<EditorBase*> m_RegisteredEditors;
};