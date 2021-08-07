#include "VulkanGraphicsObjectMesh.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include <fbxsdk.h>
#include <queue>

FbxManager* gFBXManagerPtr = NULL;
FbxScene* gFBXScenePtr = NULL;

FbxSurfaceMaterial* GetMaterialFromNode(const FbxNode* node, int directIndex)
{
	return node->GetMaterial(directIndex);
}

void SetNormalAttribute(VertexData vertexDatas[3], int vertexIndex, FbxVector4 val)
{
	auto vertexData = vertexDatas[vertexIndex];
	vertexData.m_Normal = glm::vec3(val[0], val[1], val[2]);;
	vertexDatas[vertexIndex] = vertexData;
}

void SetUVAttribute(VertexData vertexDatas[3], int vertexIndex, FbxVector2 val)
{
	auto vertexData = vertexDatas[vertexIndex];
	vertexData.m_UV = glm::vec2(val[0], 1.0f - val[1]);;
	vertexDatas[vertexIndex] = vertexData;
}

void SetDirectIndexAttribute(VertexData vertexDatas[3], int vertexIndex, int val)
{
	auto vertexData = vertexDatas[vertexIndex];
	vertexData.m_Material = (float)val;
	vertexDatas[vertexIndex] = vertexData;
}

template <class TFbxLayerElementType, class TReturnValue>
struct VertexAttributeUtil
{
	typedef void (*FuncSetter)(VertexData vertexDatas[3], int vertexIndex, TReturnValue val);
	typedef void (*FuncIntegerSetter)(VertexData vertexDatas[3], int vertexIndex, int val);

	static void HandleWhenTriangle(const TFbxLayerElementType* elementPtr, VertexData vertexDatas[3], FuncSetter funcSetter, int vertexIndices[3], int polygonVertexIndex)
	{
		auto directArray = (elementPtr != NULL) ? &elementPtr->GetDirectArray() : NULL;
		auto indirectArray = (elementPtr != NULL) ? &elementPtr->GetIndexArray() : NULL;

		if (directArray == NULL || directArray->GetCount() <= 0)
		{
			return;
		}

		int directIndices[3];
		int polygonIndex = polygonVertexIndex / 3;

		switch (elementPtr->GetMappingMode())
		{
			case FbxGeometryElement::eByControlPoint:
				directIndices[0] = vertexIndices[0];
				directIndices[1] = vertexIndices[1];
				directIndices[2] = vertexIndices[2];
				break;

			case FbxGeometryElement::eByPolygonVertex:
				directIndices[0] = polygonVertexIndex + 0;
				directIndices[1] = polygonVertexIndex + 1;
				directIndices[2] = polygonVertexIndex + 2;
				break;

			case FbxGeometryElement::eByPolygon:
				directIndices[0] = polygonIndex;
				directIndices[1] = polygonIndex;
				directIndices[2] = polygonIndex;
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
						(*funcSetter)(vertexDatas, i, directArray->GetAt(directIndices[i]));
					}
				}
				break;

			//case FbxGeometryElement::eIndex:
			//	{
			//		for (int i = 0; i < 3; ++i)
			//		{
			//			(*funcSetter)(vertexDatas, i, indirectArray->GetAt(directIndices[i]));
			//		}
			//	}
			//	break;

			case FbxGeometryElement::eIndexToDirect:
				{
					for (int i = 0; i < 3; ++i)
					{
						int directIndex = indirectArray->GetAt(directIndices[i]);
						(*funcSetter)(vertexDatas, i, directArray->GetAt(directIndex));
					}
				}
				break;

			default:
				printf_console("!!!! - -2\n");
				throw;
		}
	}

	static void HandleWhenTriangleNoDirectArray(const TFbxLayerElementType* elementPtr, VertexData vertexDatas[3], FuncIntegerSetter funcSetter, int vertexIndices[3], int polygonVertexIndex)
	{
		auto indirectArray = (elementPtr != NULL) ? &elementPtr->GetIndexArray() : NULL;
		int directIndices[3];
		int polygonIndex = polygonVertexIndex / 3;

		switch (elementPtr->GetMappingMode())
		{
			case FbxGeometryElement::eByControlPoint:
				directIndices[0] = vertexIndices[0];
				directIndices[1] = vertexIndices[1];
				directIndices[2] = vertexIndices[2];
				break;

			case FbxGeometryElement::eByPolygonVertex:
				directIndices[0] = polygonVertexIndex + 0;
				directIndices[1] = polygonVertexIndex + 1;
				directIndices[2] = polygonVertexIndex + 2;
				break;

			case FbxGeometryElement::eByPolygon:
				directIndices[0] = polygonIndex;
				directIndices[1] = polygonIndex;
				directIndices[2] = polygonIndex;
				break;

			default:
				printf_console("!!!! - -1 : %d\n", elementPtr->GetMappingMode());
				throw;
		}

		switch (elementPtr->GetReferenceMode())
		{
			case FbxGeometryElement::eDirect:
				{
					for (int i = 0; i < 3; ++i)
					{
						(*funcSetter)(vertexDatas, i, directIndices[i]);
					}
				}
				break;

			case FbxGeometryElement::eIndex:
				{
					for (int i = 0; i < 3; ++i)
					{
						(*funcSetter)(vertexDatas, i, indirectArray->GetAt(directIndices[i]));
					}
				}
				break;

			case FbxGeometryElement::eIndexToDirect:
				{
					for (int i = 0; i < 3; ++i)
					{
						int directIndex = indirectArray->GetAt(directIndices[i]);
						(*funcSetter)(vertexDatas, i, directIndex);
					}
				}
				break;

			default:
				printf_console("!!!! - -2 : %d\n", elementPtr->GetReferenceMode());
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
	auto positionDataArray = std::vector<glm::vec3>();
	auto vertexDataArray = std::vector<VertexData>();
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
			positionDataArray.resize(controlPointCount);

			for (int i = 0; i < controlPointCount; ++i)
			{
				auto controlPoint = controlPointArray[i];
				positionDataArray[i] = glm::vec3((float)controlPoint[0], (float)controlPoint[1], (float)controlPoint[2]);
			}

			int polygonCount = mesh->GetPolygonCount();
			int polygonVertexIndex = 0;
			auto layerPtr = (mesh->GetLayerCount() > 0) ? mesh->GetLayer(0) : NULL; // TODO: need to research on what the purpose multiple layers are...
			auto elementUVPtr = (layerPtr != NULL) ? layerPtr->GetUVs() : NULL;
			auto elementNormalPtr = (layerPtr != NULL) ? layerPtr->GetNormals() : NULL;
			auto elementMaterialPtr = (layerPtr != NULL) ? layerPtr->GetMaterials() : NULL;
			int vertexIndices[3];
			VertexData vertexDatas[3];

			for (int i = 0; i < polygonCount; ++i)
			{
				int indexSize = mesh->GetPolygonSize(i);

				if (indexSize == 3)
				{
					vertexIndices[0] = mesh->GetPolygonVertex(i, 0);
					vertexIndices[1] = mesh->GetPolygonVertex(i, 1);
					vertexIndices[2] = mesh->GetPolygonVertex(i, 2);
					vertexDatas[0].m_Position = positionDataArray[vertexIndices[0]];
					vertexDatas[1].m_Position = positionDataArray[vertexIndices[1]];
					vertexDatas[2].m_Position = positionDataArray[vertexIndices[2]];
					VertexAttributeUtil<FbxLayerElementUV, FbxVector2>::HandleWhenTriangle(elementUVPtr, vertexDatas, &SetUVAttribute, vertexIndices, polygonVertexIndex);
					VertexAttributeUtil<FbxLayerElementNormal, FbxVector4>::HandleWhenTriangle(elementNormalPtr, vertexDatas, &SetNormalAttribute, vertexIndices, polygonVertexIndex);
					VertexAttributeUtil<FbxLayerElementMaterial, int>::HandleWhenTriangleNoDirectArray(elementMaterialPtr, vertexDatas, &SetDirectIndexAttribute, vertexIndices, polygonVertexIndex);
					indexDataArray.push_back(polygonVertexIndex + 0);
					indexDataArray.push_back(polygonVertexIndex + 1);
					indexDataArray.push_back(polygonVertexIndex + 2);
					vertexDataArray.push_back(vertexDatas[0]);
					vertexDataArray.push_back(vertexDatas[1]);
					vertexDataArray.push_back(vertexDatas[2]);

					polygonVertexIndex += 3;
				}
				else
				{
					printf_console("!!!! - 0\n");
					// TODO: handle not only triangle but also any type of polygons...

					throw;
				}
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