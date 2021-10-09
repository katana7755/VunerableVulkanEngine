#pragma once

#include "VulkanGraphicsResourceBase.h"
#include <vector>

namespace EVulkanShaderType
{
	enum TYPE
	{
		NONE = -1,

		VERTEX,
		FRAGMENT,

		MAX,
	};
}

namespace EVulkanShaderBindingResource
{
	enum TYPE
	{
		NONE = -1,

		TEXTURE2D,

		MAX,
	};
};

namespace EVulkanShaderVertexInput
{
	enum TYPE
	{
		NONE = -1,

		VECTOR1,
		VECTOR2,
		VECTOR3,
		VECTOR4,

		MAX
	};
};

struct VulkanShaderMetaData
{
	EVulkanShaderType::TYPE m_Type;
	std::string m_Name;
	std::vector<EVulkanShaderBindingResource::TYPE> m_InputBindingArray;
	size_t m_PushConstantOffset;
	size_t m_PushConstantSize;
	std::vector<EVulkanShaderVertexInput::TYPE> m_VertexInputArray; // only for vertex shader...support only 1 binding

	bool operator==(const VulkanShaderMetaData& rhs)
	{
		if (m_Type != rhs.m_Type)
		{
			return false;
		}

		if (m_Name != rhs.m_Name)
		{
			return false;
		}

		if (memcmp(m_InputBindingArray.data(), rhs.m_InputBindingArray.data(), sizeof(EVulkanShaderBindingResource::TYPE) * m_InputBindingArray.size()) != 0)
		{
			return false;
		}

		if (m_PushConstantOffset != rhs.m_PushConstantOffset)
		{
			return false;
		}

		if (m_PushConstantSize != rhs.m_PushConstantSize)
		{
			return false;
		}


		if (memcmp(m_VertexInputArray.data(), rhs.m_VertexInputArray.data(), sizeof(EVulkanShaderVertexInput::TYPE) * m_VertexInputArray.size()))
		{
			return false;
		}

		return true;
	}

	VkShaderStageFlagBits GetShaderStageBits() const
	{
		switch (m_Type)
		{
		case EVulkanShaderType::VERTEX:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case EVulkanShaderType::FRAGMENT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		default:
			printf_console("[VulkanGraphics] failed to get the shader stage bits\n");
			throw;
		}
	}

	bool DoesHaveInputBinding() const
	{
		bool flag = false;

		for (auto inputBinding : m_InputBindingArray)
		{
			if (inputBinding > EVulkanShaderBindingResource::NONE && inputBinding < EVulkanShaderBindingResource::MAX)
			{
				flag = true;
				break;
			}
		}

		return flag;
	}

	bool DoesHavePushConstant() const
	{
		return m_PushConstantSize > 0;
	}

	bool DoesHaveVertexInput() const
	{
		if (m_VertexInputArray.size() <= 0)
		{
			return false;
		}

		int actualCount = 0;

		for (auto vertexInput : m_VertexInputArray)
		{
			if (vertexInput <= EVulkanShaderVertexInput::NONE || vertexInput >= EVulkanShaderVertexInput::MAX)
			{
				continue;
			}

			++actualCount;
		}

		return actualCount > 0;
	}
};

class VulkanGraphicsResourceShaderManager : public VulkanGraphicsResourceManagerBase<VkShaderModule, size_t, VulkanShaderMetaData>
{
public:
	static VulkanGraphicsResourceShaderManager& GetInstance();

protected:
	VkShaderModule CreateResourcePhysically(const size_t& uploadBufferIdentifier) override;
	void DestroyResourcePhysicially(const VkShaderModule& shaderModule) override;
};