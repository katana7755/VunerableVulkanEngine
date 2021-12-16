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
	static EditorManager& GetInstance();

public:
	EditorManager();
	~EditorManager();

public:
	void DrawEditors();
	void RegisterEditor(EditorBase* editorPtr);

private:
	std::vector<EditorBase*> m_RegisteredEditors;
};