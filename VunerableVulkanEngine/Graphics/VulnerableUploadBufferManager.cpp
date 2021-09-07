#include "VulnerableUploadBufferManager.h"
#include "../DebugUtility.h"
#include <stdio.h>
#include <vector>

std::vector<VulnerableUploadBuffer> s_UploadBufferArray;

int VulnerableUploadBufferManager::LoadFromFile(const char* strFileName)
{
	// TODO: need to consider multex for supporting multi threading?
	FILE* file = fopen(strFileName, "rb");

	if (file == NULL)
	{
		printf_console("[VulnerableUploadBufferManager] failed to open file(%s)\n", strFileName);

		throw;
	}

	fseek(file, 0, SEEK_END);

	auto byteSize = ftell(file);
	VulnerableUploadBuffer newBuffer;
	newBuffer.m_Data = new char[byteSize];
	newBuffer.m_Size = (unsigned int)byteSize;
	rewind(file);
	fread(newBuffer.m_Data, 1, byteSize, file);
	fclose(file);
	s_UploadBufferArray.push_back(newBuffer);

	return s_UploadBufferArray.size() - 1;
}

const VulnerableUploadBuffer& VulnerableUploadBufferManager::GetUploadBuffer(int bufferID)
{
	// TODO: need to consider multex for supporting multi threading?
	return s_UploadBufferArray[bufferID];
}

void VulnerableUploadBufferManager::ClearAllUploadBuffer()
{
	// TODO: need to consider multex for supporting multi threading?
	s_UploadBufferArray.clear();
}