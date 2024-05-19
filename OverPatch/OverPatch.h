#pragma once

#include <string>

class OverPatch
{
public:
	static bool LaunchAndPatch(std::string filePath);

private:
	static bool IsMemoryAvailable(void* hProcess, uintptr_t pMemory, void* pSignature, size_t sigSize);
	static bool WriteNops(void* hProcess, uintptr_t pMemory, size_t count);
	static bool PatchBytes(void* hProcess, uintptr_t pMemory, void* pPatch, size_t count);
};

