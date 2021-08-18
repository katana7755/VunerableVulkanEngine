#pragma once

#include "EditorManager.h"
#include "vulkan/vulkan.h"

class EditorGameView : public EditorBase
{
public:
	static void SetTexture(ImTextureID textureID, ImTextureID descriptorSet)
	{
		if (s_UniquePtr == NULL)
		{
			return;
		}

		s_UniquePtr->SetTextureInternal(textureID, descriptorSet);
	}

private:
	static EditorGameView* s_UniquePtr;

public:
	EditorGameView()
		: EditorBase()
	{
		s_UniquePtr = this;
	}

	~EditorGameView()
	{
		if (s_UniquePtr == this)
		{
			s_UniquePtr = NULL;
		}		
	}

public:
	void CheckBox() override;
	void DrawWhenChecked() override;

private:
	void SetTextureInternal(ImTextureID textureID, ImTextureID descriptorSet);

	ImTextureID m_TextureID;
	ImTextureID m_DescriptorSet;
};