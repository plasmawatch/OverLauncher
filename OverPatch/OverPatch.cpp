#include "OverPatch.h"
#include <Windows.h>
#include <iostream>
#include <locale>
#include <codecvt>
#include <Psapi.h>
#include <vector>

const unsigned char pinSignature[] = { 0xFF, 0x50, 0x38 };
const unsigned char jneSignature[] = { 0x0F, 0x85, 0x78, 0x01, 0x00, 0x00 };
const unsigned char curlSslSignature[] = { 0x74, 0x10 };

const unsigned char curlSslPatch[] = { 0x75, 0x10 };

bool OverPatch::LaunchAndPatch(std::string filePath)
{
    // Launch
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(filePath);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL,
        &wide[0],
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)
        )
    {
        std::cerr << "Error: CreateProcess failed\n";
        return false;
    }

    if (WaitForInputIdle(pi.hProcess, INFINITE) != 0)
    {
        std::cerr << "Error: WaitForInputIdle failed\n";
        return false;
    }
    
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (!EnumProcessModules(pi.hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        std::cout << "Error: Failed to enumerate process modules. " << GetLastError() << std::endl;
        return 1;
    }

    MODULEINFO modInfo;
    if (!GetModuleInformation(pi.hProcess, hMods[0], &modInfo, sizeof(modInfo)))
    {
        std::cout << "Error: Failed to get module information." << std::endl;
        return 1;
    }

    std::cout << "Launch finished!" << std::endl;

    // Patch

    unsigned char buffer[4];
    SIZE_T bytesRead;

    uintptr_t pPinRVA = 0x1100106;
    uintptr_t pJneCodeRVA = 0x110010D;
    uintptr_t pCurlRVA = 0x7C106;

    uintptr_t pPinAddr = (uintptr_t)modInfo.lpBaseOfDll + pPinRVA;
    uintptr_t pJneCodeAddr = (uintptr_t)modInfo.lpBaseOfDll + pJneCodeRVA;
    uintptr_t pCurlAddr = (uintptr_t)modInfo.lpBaseOfDll + pCurlRVA;

    while (true)
    {
        if (IsMemoryAvailable(pi.hProcess, pPinAddr, (void*)&pinSignature, sizeof(pinSignature)) &&
            IsMemoryAvailable(pi.hProcess, pJneCodeAddr, (void*)&jneSignature, sizeof(jneSignature)))
            //IsMemoryAvailable(pi.hProcess, pCurlAddr, (void*)&curlSslSignature, sizeof(curlSslSignature)))
        {
            std::cout << "Unpacked, ready to patch!\n";
            break;
        }

        std::cout << "Waiting for unpacking (region1) to finish..\n";

        Sleep(50);
    }

    HANDLE hProcessWithWriteAccess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pi.dwProcessId);

    if (!hProcessWithWriteAccess)
    {
        std::cout << "Error: Failed to open process with write access.\n";
    }

    WriteNops(hProcessWithWriteAccess, pPinAddr, sizeof(pinSignature));
    WriteNops(hProcessWithWriteAccess, pJneCodeAddr, sizeof(jneSignature));
    //PatchBytes(hProcessWithWriteAccess, pCurlAddr, (void*)&curlSslPatch, sizeof(curlSslPatch));
 

    std::cout << "Patch 1 applied!" << std::endl;

    while (true)
    {
        if (IsMemoryAvailable(pi.hProcess, pCurlAddr, (void*)&curlSslSignature, sizeof(curlSslSignature)))
        {
            std::cout << "Unpacked, ready to patch!\n";
            break;
        }

        std::cout << "Waiting for unpacking (region2) to finish..\n";

        Sleep(50);
    }

    PatchBytes(hProcessWithWriteAccess, pCurlAddr, (void*)&curlSslPatch, sizeof(curlSslPatch));

    std::cout << "Patch 2 applied!" << std::endl;
    std::cout << "Finished!" << std::endl;

    // Cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    CloseHandle(hProcessWithWriteAccess);

    return true;
}

bool OverPatch::IsMemoryAvailable(void* hProcess, uintptr_t pMemory, void* pSignature, size_t sigSize)
{
    void* buffer = new char[sigSize];
    SIZE_T bytesRead;

    if (ReadProcessMemory((HANDLE)hProcess, (LPCVOID)pMemory, buffer, sigSize, &bytesRead))
    {
        if (memcmp(pSignature, buffer, sigSize) == 0)
        {
            delete[] buffer;;
            return true;
        }
    }

    delete[] buffer;

    return false;
}

bool OverPatch::WriteNops(void* hProcess, uintptr_t pMemory, size_t count)
{
    std::vector<char> nops(count, 0x90);
    SIZE_T bytesWritten;
    DWORD oldProtect;

    MEMORY_BASIC_INFORMATION mbi;
    VirtualQueryEx(hProcess, (LPCVOID)pMemory, &mbi, count);

    int protect = mbi.Protect & PAGE_GUARD;

    std::cout << pMemory << " protection: " << protect << "\n";

    // Change memory protection to PAGE_EXECUTE_READWRITE
    if (!VirtualProtectEx((HANDLE)hProcess, (LPVOID)pMemory, count, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        std::cout << "Error changing memory protection: " << GetLastError() << "\n";
        return false;
    }

    bool success = false;

    // Write the NOPs
    if (WriteProcessMemory((HANDLE)hProcess, (LPVOID)pMemory, nops.data(), count, &bytesWritten))
    {
        if (bytesWritten == count)
        {
            std::cout << "Successfully wrote " << count << " NOPs at " << pMemory << "\n";
            success = true;
        }
        else
        {
            std::cout << "Error: Only " << bytesWritten << " out of " << count << " NOPs were written.\n";
        }
    }
    else
    {
        std::cout << "Error writing to process memory: " << GetLastError() << "\n";
    }

    // Restore the old memory protection
    DWORD dummy;
    if (!VirtualProtectEx((HANDLE)hProcess, (LPVOID)pMemory, count, oldProtect, &dummy))
    {
        std::cout << "Error restoring memory protection: " << GetLastError() << "\n";
        success = false;
    }

    return success;
}

bool OverPatch::PatchBytes(void* hProcess, uintptr_t pMemory, void* pPatch, size_t count)
{
    char* pPatchBytes = static_cast<char*>(pPatch);
    std::vector<char> patchBuffer(pPatchBytes, pPatchBytes + count);
    SIZE_T bytesWritten;
    DWORD oldProtect;

    MEMORY_BASIC_INFORMATION mbi;
    VirtualQueryEx(hProcess, (LPCVOID)pMemory, &mbi, count);

    int protect = mbi.Protect & PAGE_GUARD;

    std::cout << pMemory << " protection: " << protect << "\n";

    // Change memory protection to PAGE_EXECUTE_READWRITE
    if (!VirtualProtectEx((HANDLE)hProcess, (LPVOID)pMemory, count, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        std::cout << "Error changing memory protection: " << GetLastError() << "\n";
        return false;
    }

    bool success = false;

    // Write the NOPs
    if (WriteProcessMemory((HANDLE)hProcess, (LPVOID)pMemory, patchBuffer.data(), count, &bytesWritten))
    {
        if (bytesWritten == count)
        {
            std::cout << "Successfully wrote " << count << " bytes at " << pMemory << "\n";
            success = true;
        }
        else
        {
            std::cout << "Error: Only " << bytesWritten << " out of " << count << " bytes were written.\n";
        }
    }
    else
    {
        std::cout << "Error writing to process memory: " << GetLastError() << "\n";
    }

    // Restore the old memory protection
    DWORD dummy;
    if (!VirtualProtectEx((HANDLE)hProcess, (LPVOID)pMemory, count, oldProtect, &dummy))
    {
        std::cout << "Error restoring memory protection: " << GetLastError() << "\n";
        success = false;
    }

    return success;
}

