#pragma once

#include "EditorManager.h"

class EditorProjectBrowser : public EditorBase
{
public:
	void CheckBox() override;
	void DrawWhenChecked() override;
};