#pragma once

#include "EditorManager.h"
#include "../ECS/Domain.h"

class EditorSceneBrowser : public EditorBase
{
public:
	void CheckBox() override;
	void DrawWhenChecked() override;

private:
	void CreateChunk(bool isOpen);
	void DestroyChunk(bool isOpen);
	void CreateEntity(bool isOpen);
	void DestroyEntity(bool isOpen);
};