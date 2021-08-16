#pragma once

#include "EditorManager.h"

class EditorInspector : public EditorBase
{
public:
	void CheckBox() override;
	void DrawWhenChecked() override;

private:
	float m_FakePosition[3];
	float m_FakeRotation[3];
	float m_FakeScale[3];
	int m_FakeRenderType;
	int m_FakeRenderMesh;
	int m_FakeRenderTexture;
};