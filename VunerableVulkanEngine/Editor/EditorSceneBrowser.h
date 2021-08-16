#pragma once

#include "EditorManager.h"

class EditorSceneBrowser : public EditorBase
{
public:
	void CheckBox() override;
	void DrawWhenChecked() override;
};