#pragma once

#include "../IMGUI/imgui.h"
#include <vector>

class EditorBase
{
public:
	virtual void CheckBox() = 0;
	virtual void DrawWhenChecked() = 0;
};

class EditorManager
{
public:
	static void DrawEditors()
	{
		return s_Instance.DrawEditorsInternal();
	}

	static void RegisterEditor(EditorBase* editorPtr)
	{
		return s_Instance.RegisterEditorInternal(editorPtr);
	}

	static void UnregisterEditor(EditorBase* editorPtr)
	{
		return s_Instance.UnregisterEditorInternal(editorPtr);
	}

private:
	static EditorManager s_Instance;

private:
	EditorManager() : 
		show_demo_window(false), 
		show_another_window(false),
		clear_color(ImVec4(0.45f, 0.55f, 0.60f, 1.00f))
	{
	}

	void DrawEditorsInternal();
	void RegisterEditorInternal(EditorBase* editorPtr);
	void UnregisterEditorInternal(EditorBase* editorPtr);

	bool show_demo_window;
	bool show_another_window;
	ImVec4 clear_color;

	std::vector<EditorBase*> m_RegisteredEditors;
};