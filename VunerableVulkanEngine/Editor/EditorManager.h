#pragma once

#include "../IMGUI/imgui.h"
#include <vector>
#include <memory>

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
	static void DrawEditors()
	{
		return s_UniquePtr->DrawEditorsInternal();
	}

private:
	static EditorManager s_Instance;
	static EditorManager* s_UniquePtr;

private:
	EditorManager();
	~EditorManager();

	void DrawEditorsInternal();
	void RegisterEditorInternal(EditorBase* editorPtr);

	std::vector<EditorBase*> m_RegisteredEditors;
};