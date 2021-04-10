#include "VulkanGraphicsObjectMesh.h"
#include "../DebugUtility.h"
#include <fbxsdk.h>

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

	importer->Destroy();
}

bool VulkanGraphicsObjectMesh::CreateInternal()
{
	return true;
}

bool VulkanGraphicsObjectMesh::DestroyInternal()
{
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