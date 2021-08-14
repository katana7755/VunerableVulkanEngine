#pragma once

#include "EditorManager.h"

class EditorInspector : public EditorBase
{
public:
	EditorInspector();
	~EditorInspector();

public:
	void CheckBox() override;
	void DrawWhenChecked() override;

private:
	bool m_IsChecked;
};