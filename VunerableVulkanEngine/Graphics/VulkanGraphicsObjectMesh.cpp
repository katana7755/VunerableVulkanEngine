#include "VulkanGraphicsObjectMesh.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include <fbxsdk.h>
#include <queue>

FbxManager* gFBXManagerPtr = NULL;
FbxScene* gFBXScenePtr = NULL;

void SetNormalAttribute(std::vector<VertexData>& vertexDataArray, int vertexIndex, FbxVector4 val)
{
	auto vertexData = vertexDataArray[vertexIndex];
	vertexData.m_Normal += glm::vec3(val[0], val[1], val[2]);;
	vertexDataArray[vertexIndex] = vertexData;
}

void SetUVAttribute(std::vector<VertexData>& vertexDataArray, int vertexIndex, FbxVector2 val)
{
	auto vertexData = vertexDataArray[vertexIndex];
	vertexData.m_UV += glm::vec2(val[0], val[1]);;
	vertexDataArray[vertexIndex] = vertexData;
}

template <class TFbxLayerElementType, class TValue>
struct VertexAttributeUtil
{
	typedef void (*FuncSetter)(std::vector<VertexData>& vertexDataArray, int vertexIndex, TValue val);

	static void HandleWhenTriangle(const TFbxLayerElementType* elementPtr, std::vector<VertexData>& vertexDataArray, FuncSetter funcSetter, int vertexIndices[3], int polygonIndex, std::vector<int>& attributeCountArray)
	{
		auto directArray = (elementPtr != NULL) ? &elementPtr->GetDirectArray() : NULL;
		auto indirectArray = (elementPtr != NULL) ? &elementPtr->GetIndexArray() : NULL;

		if (directArray == NULL || directArray->GetCount() <= 0)
		{
			return;
		}

		int indirectIndices[3] = { vertexIndices[0], vertexIndices[1], vertexIndices[2] };

		switch (elementPtr->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
			break;

		case FbxGeometryElement::eByPolygonVertex:
			indirectIndices[0] = polygonIndex + 0;
			indirectIndices[1] = polygonIndex + 1;
			indirectIndices[2] = polygonIndex + 2;
			break;

		default:
			printf_console("!!!! - -1\n");
			throw;
		}

		switch (elementPtr->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			for (int i = 0; i < 3; ++i)
			{
				(*funcSetter)(vertexDataArray, vertexIndices[i], directArray->GetAt(indirectIndices[i]));
				++attributeCountArray[vertexIndices[i]];
			}
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			for (int i = 0; i < 3; ++i)
			{
				int id = indirectArray->GetAt(indirectIndices[i]);
				(*funcSetter)(vertexDataArray, vertexIndices[i], directArray->GetAt(id));
				++attributeCountArray[vertexIndices[i]];
			}
		}
		break;

		default:
			printf_console("!!!! - -2\n");
			throw;
		}
	}
};

void VulkanGraphicsObjectMesh::CreateFromFBX(const char* strFbxPath)
{
	if (gFBXManagerPtr == NULL)
	{
		gFBXManagerPtr = FbxManager::Create();

		auto ioSettings = FbxIOSettings::Create(gFBXManagerPtr, IOSROOT);
		gFBXManagerPtr->SetIOSettings(ioSettings);
	}

#ifdef _DEBUG
	int sdkVersionMajor, sdkVersionMinor, sdkVersionRevision;
	FbxManager::GetFileFormatVersion(sdkVersionMajor, sdkVersionMinor, sdkVersionRevision);
	printf_console("[FBX SDK] SDK version is (%d. %d. %d)\n", sdkVersionMajor, sdkVersionMinor, sdkVersionRevision);
#endif

	auto importer = FbxImporter::Create(gFBXManagerPtr, "");
	auto ioSettingsPtr = gFBXManagerPtr->GetIOSettings();

	if (!importer->Initialize(strFbxPath, -1, ioSettingsPtr))
	{
		auto status = importer->GetStatus();
		printf_console("[FBX SDK] failed to initialize an importer for the fbx file %s because %s\n", strFbxPath, status.GetErrorString());

#ifdef _DEBUG
		int fileVersionMajor, fileVersionMinor, fileVersionRevision;
		importer->GetFileVersion(fileVersionMajor, fileVersionMinor, fileVersionRevision);
		printf_console("[FBX SDK] the file version is (%d. %d. %d)\n", fileVersionMajor, fileVersionMinor, fileVersionRevision);
#endif

		throw;
	}

	if (!importer->IsFBX())
	{
		printf_console("[FBX SDK] the fbx file %s is invalid\n", strFbxPath);

		throw;
	}

#ifdef _DEBUG
	// TODO: print the detail in the fbx file...
#endif

	// TODO: in the future let's figure out how each item affects importing result...
	ioSettingsPtr->SetBoolProp(IMP_FBX_MATERIAL, true);
	ioSettingsPtr->SetBoolProp(IMP_FBX_TEXTURE, true);
	ioSettingsPtr->SetBoolProp(IMP_FBX_LINK, true);
	ioSettingsPtr->SetBoolProp(IMP_FBX_SHAPE, true);
	ioSettingsPtr->SetBoolProp(IMP_FBX_GOBO, true);
	ioSettingsPtr->SetBoolProp(IMP_FBX_ANIMATION, true);
	ioSettingsPtr->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	if (gFBXScenePtr != NULL)
	{
		gFBXScenePtr->Destroy();
		gFBXScenePtr = NULL;
	}

	gFBXScenePtr = FbxScene::Create(gFBXManagerPtr, "");

	if (!importer->Import(gFBXScenePtr))
	{
		auto status = importer->GetStatus();
		printf_console("[FBX SDK] failed to import the fbx file %s because %s\n", strFbxPath, status.GetErrorString());

		// TODO: need to care the case when the fbx file is protected by password...
		throw;
	}

	// TODO: currently, for the first version we simply handle only one mesh, but in the future let's handle multiple meshes too...
	// TODO: there might be more information in a FBX file that those need to be parsed such as 
	//       - texture
	//       - skeleton animation
	//       - blend shape animation
	auto vertexDataArray = std::vector<VertexData>();
	auto vertexNormalCountArray = std::vector<int>();
	auto vertexUVCountArray = std::vector<int>();
	auto indexDataArray = std::vector<int>();
	auto nodeQueue = std::queue<FbxNode*>();
	nodeQueue.push(gFBXScenePtr->GetRootNode());

	while (!nodeQueue.empty())
	{
		auto currentNode = nodeQueue.front();
		nodeQueue.pop();

		auto attribute = currentNode->GetNodeAttribute();

		if (attribute != NULL && attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			auto mesh = (FbxMesh*)currentNode->GetNodeAttribute();

#ifdef _DEBUG
			// check validity for polygon groups
			int polygonGrpCount = mesh->GetElementPolygonGroupCount();

			for (int polygonGrpIndex = 0; polygonGrpIndex < polygonGrpCount; ++polygonGrpIndex)
			{
				if (mesh->GetElementPolygonGroup(polygonGrpIndex)->GetMappingMode() != FbxGeometryElement::eByPolygon)
				{
					printf_console("[FBX SDK] the fbx file %s has more than one invalid element group\n", strFbxPath);

					throw;
				}
			}
#endif

			int controlPointCount = mesh->GetControlPointsCount();
			auto controlPointArray = mesh->GetControlPoints();
			vertexDataArray.resize(controlPointCount);
			vertexNormalCountArray.resize(controlPointCount, 0);
			vertexUVCountArray.resize(controlPointCount, 0);

			for (int i = 0; i < controlPointCount; ++i)
			{
				auto controlPoint = controlPointArray[i];
				auto vertexData = vertexDataArray[i];
				vertexData.m_Position = glm::vec3((float)controlPoint[0], (float)controlPoint[1], (float)controlPoint[2]);
				vertexData.m_UV = glm::vec2(0.0f, 0.0f);
				vertexData.m_Normal = glm::vec3(0.0f, 0.0f, 1.0f);
				vertexData.m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
				vertexDataArray[i] = vertexData;
			}

			int polygonCount = mesh->GetPolygonCount();
			int polygonIndex = 0;
			auto layerPtr = (mesh->GetLayerCount() > 0) ? mesh->GetLayer(0) : NULL; // TODO: need to research on what the purpose multiple layers are...
			auto elementNormalPtr = (layerPtr != NULL) ? layerPtr->GetNormals() : NULL;
			auto elementUVPtr = (layerPtr != NULL) ? layerPtr->GetUVs() : NULL;
			int vertexIndices[3];

			for (int i = 0; i < polygonCount; ++i)
			{
				int indexSize = mesh->GetPolygonSize(i);

				if (indexSize == 3)
				{
					vertexIndices[0] = mesh->GetPolygonVertex(i, 0);
					vertexIndices[1] = mesh->GetPolygonVertex(i, 1);
					vertexIndices[2] = mesh->GetPolygonVertex(i, 2);
					indexDataArray.push_back(vertexIndices[0]);
					indexDataArray.push_back(vertexIndices[1]);
					indexDataArray.push_back(vertexIndices[2]);
					VertexAttributeUtil<FbxLayerElementNormal, FbxVector4>::HandleWhenTriangle(elementNormalPtr, vertexDataArray, &SetNormalAttribute, vertexIndices, polygonIndex, vertexNormalCountArray);
					VertexAttributeUtil<FbxLayerElementUV, FbxVector2>::HandleWhenTriangle(elementUVPtr, vertexDataArray, &SetUVAttribute, vertexIndices, polygonIndex, vertexUVCountArray);
					polygonIndex += 3;
				}
				else
				{
					printf_console("!!!! - 0\n");
					// TODO: handle not only triangle but also any type of polygons...

					throw;
				}
			}
			
			for (int i = 0; i < controlPointCount; ++i)
			{
				auto vertexData = vertexDataArray[i];
				vertexData.m_Normal = (vertexNormalCountArray[i] > 1) ? vertexData.m_Normal / (float)vertexNormalCountArray[i] : vertexData.m_Normal;
				vertexData.m_UV = (vertexUVCountArray[i] > 1) ? vertexData.m_UV / (float)vertexUVCountArray[i] : vertexData.m_UV;
				vertexDataArray[i] = vertexData;
			}

			break;
		}

		int nodeCount = currentNode->GetChildCount();

		for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
		{
			nodeQueue.push(currentNode->GetChild(nodeIndex));
		}
	}

	CreateGPUResource(m_GPUVertexBuffer, m_GPUVertexMemory, vertexDataArray.data(), vertexDataArray.size() * sizeof(VertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	CreateGPUResource(m_GPUIndexBuffer, m_GPUIndexMemory, indexDataArray.data(), indexDataArray.size() * sizeof(int), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_IndexCount = indexDataArray.size();
	importer->Destroy();
	VulkanGraphicsObjectBase::Create();
}

bool VulkanGraphicsObjectMesh::CreateInternal()
{
	return true;
}

bool VulkanGraphicsObjectMesh::DestroyInternal()
{
	vkFreeMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_GPUIndexMemory, NULL);
	vkDestroyBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_GPUIndexBuffer, NULL);
	vkFreeMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_GPUVertexMemory, NULL);
	vkDestroyBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_GPUVertexBuffer, NULL);

	if (gFBXScenePtr != NULL)
	{
		gFBXScenePtr->Destroy();
		gFBXScenePtr = NULL;
	}

	if (gFBXManagerPtr != NULL)
	{
		gFBXManagerPtr->Destroy();
		gFBXManagerPtr = NULL;
	}

	return true;
}

void VulkanGraphicsObjectMesh::CreateGPUResource(VkBuffer& gpuBuffer, VkDeviceMemory& gpuMemory, void* dataPtr, VkDeviceSize dataSize, VkBufferUsageFlags bufferUsage, VkFlags memoryFlags)
{
	auto createInfo = VkBufferCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0; // TODO: this is related to sparse binding...one day we might need this...
	createInfo.size = dataSize;
	createInfo.usage = bufferUsage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = NULL;

	auto result = vkCreateBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &gpuBuffer);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a mesh with error code %d\n", result);

		throw;
	}

	auto requirements = VkMemoryRequirements();
	vkGetBufferMemoryRequirements(VulkanGraphicsResourceDevice::GetLogicalDevice(), gpuBuffer, &requirements);

	uint32_t memoryTypeIndex;

	if (!VulkanGraphicsResourceDevice::GetMemoryTypeIndex(requirements.memoryTypeBits, memoryFlags, &memoryTypeIndex))
	{
		printf_console("[VulkanGraphics] failed to get a memory type index for this mesh with error code %d\n", result);

		throw;
	}

	auto allocateInfo = VkMemoryAllocateInfo();
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.pNext = NULL;
	allocateInfo.allocationSize = requirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;
	result = vkAllocateMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, NULL, &gpuMemory);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to allocate a memory block for this mesh with error code %d\n", result);

		throw;
	}

	result = vkBindBufferMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), gpuBuffer, gpuMemory, 0);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to bind a memory block for this mesh with error code %d\n", result);

		throw;
	}

	// ***** Let's create a function for creating new buffer...
	// ***** Need To Implement Actual Binding...
	void* bufferPtr;
	vkMapMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), gpuMemory, 0, dataSize, 0, &bufferPtr);
	memcpy(bufferPtr, dataPtr, dataSize);
	vkUnmapMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), gpuMemory);

	// ***** And Transfer Through Transient Command Buffer...
}

void VulkanGraphicsObjectMesh::DestroyGPUResource(VkBuffer& gpuBuffer, VkDeviceMemory& gpuMemory)
{
	vkFreeMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), gpuMemory, NULL);
	vkDestroyBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), gpuBuffer, NULL);
}