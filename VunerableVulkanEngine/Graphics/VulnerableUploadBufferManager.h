#pragma once

struct VulnerableUploadBuffer
{
	char* m_Data;
	unsigned int m_Size;
};

class VulnerableUploadBufferManager
{
private:
	VulnerableUploadBufferManager();

public:
	static int LoadFromFile(const char* strFileName);
	static const VulnerableUploadBuffer& GetUploadBuffer(int bufferID);
	static void ClearAllUploadBuffer();
};