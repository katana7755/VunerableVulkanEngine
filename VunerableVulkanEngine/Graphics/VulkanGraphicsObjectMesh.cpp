#include "VulkanGraphicsObjectMesh.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include <fbxsdk.h>
#include <queue>

FbxManager* gFBXManagerPtr = NULL;
FbxScene* gFBXScenePtr = NULL;

void VulkanGraphicsObjectMesh::PrepareDataFromFBX(const char* strFbxPath)
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
	// TODO: there might be more information in a FBX file that those need to be parsed such as texture, skeleton, animation, blend shape, etc...
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
			vertexDataArray.resize(controlPointCount);

			for (int i = 0; i < controlPointCount; ++i)
			{
				auto controlPoint = controlPointArray[i];
				auto vertexData = vertexDataArray[i];
				vertexData.m_Position = glm::vec3((float)controlPoint[0], (float)controlPoint[1], (float)controlPoint[2]);
				vertexData.m_UV = glm::vec2(0.0f, 0.0f);
				vertexData.m_Normal = glm::vec3(0.0f, 0.0f, 1.0f);
				vertexData.m_Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				vertexDataArray[i] = vertexData;
			}

			int polygonCount = mesh->GetPolygonCount();

			for (int i = 0; i < polygonCount; ++i)
			{
				int indexSize = mesh->GetPolygonSize(i);

#ifdef _DEBUG
				if (indexSize < 3)
				{
					printf_console("[FBX SDK] the fbx file %s has more than one invalid polygon\n", strFbxPath);

					throw;
				}
#endif

				if (indexSize == 3)
				{
					indexDataArray.push_back(mesh->GetPolygonVertex(i, 0));
					indexDataArray.push_back(mesh->GetPolygonVertex(i, 1));
					indexDataArray.push_back(mesh->GetPolygonVertex(i, 2));
				}
				else
				{
					printf_console("!!!!\n");
					// TODO: handle not only quad but also any type of polygons...
				}
			}

			// TODO: need to implement the importing process for other attributes...
			//if (mesh->GetLayerCount() > 0)
			//{
			//	auto layer = mesh->GetLayer(0);
			//	layer->GetUVs();
			//}
			//mesh->GetPolygonVertexCount();
			//mesh->GetPolygonVertices();

			break;
		}

		int nodeCount = currentNode->GetChildCount();

		for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
		{
			nodeQueue.push(currentNode->GetChild(nodeIndex));
		}
	}

	auto createInfo = VkBufferCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0; // TODO: this is related to sparse binding...one day we might need this...
	createInfo.size = sizeof(vertexDataArray[0]) * vertexDataArray.size();
	createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = NULL;

	auto result = vkCreateBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &m_GPUBuffer);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a mesh with error code %d\n", result);

		throw;
	}

	auto requirements = VkMemoryRequirements();
	vkGetBufferMemoryRequirements(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_GPUBuffer, &requirements);

	uint32_t memoryTypeIndex;

	if (!VulkanGraphicsResourceDevice::GetMemoryTypeIndex(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryTypeIndex))
	{
		printf_console("[VulkanGraphics] failed to get a memory type index for this mesh with error code %d\n", result);

		throw;
	}

	auto allocateInfo = VkMemoryAllocateInfo();
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.pNext = NULL;
	allocateInfo.allocationSize = requirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;
	result = vkAllocateMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, NULL, &m_GPUMemory);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to allocate a memory block for this mesh with error code %d\n", result);

		throw;
	}

	// ***** Let's create a function for creating new buffer...
	// ***** Need To Implement Actual Binding...
	//vkMapMemory();
	// ***** And Transfer Through Transient Command Buffer...

	importer->Destroy();
}

bool VulkanGraphicsObjectMesh::CreateInternal()
{
	return true;
}

bool VulkanGraphicsObjectMesh::DestroyInternal()
{
	vkFreeMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_GPUMemory, NULL);
	vkDestroyBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_GPUBuffer, NULL);

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