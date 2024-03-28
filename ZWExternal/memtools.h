#pragma once

#define STATUS_SUCCESS 0x00000000

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <array>
//#include <memory>
#include <optional>

#include "structs.h"

// Functions defined in procs.asm
extern "C" NTSTATUS ZwReadVirtualMemory(HANDLE hProcess, void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesRead = NULL);
extern "C" NTSTATUS ZwWriteVirtualMemory(HANDLE hProcess, void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesRead = NULL);
extern "C" NTSTATUS ZwOpenProcess(PHANDLE hProcess, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
extern "C" NTSTATUS ZwProtectVirtualMemory(HANDLE hProcess, void* pAddress, size_t dwSize, ULONG newProtect, PULONG oldProtect);

class HandleManager
{
private:
  HANDLE handle;

public:
  HandleManager() : handle(NULL) {}

  explicit HandleManager(HANDLE h) : handle(h) {}

  /// Destructor
  ~HandleManager()
  {
    if (handle && handle != INVALID_HANDLE_VALUE)
    {
      CloseHandle(handle);
    }
  }

  /// Deleted copy constructor and copy assignment to prevent copying
  HandleManager(const HandleManager&) = delete;
  HandleManager& operator=(const HandleManager&) = delete;

  /// Move constructor
  HandleManager(HandleManager&& other) noexcept : handle(other.handle)
  {
    other.handle = NULL;
  }

  /// Move assignment operator
  HandleManager& operator=(HandleManager&& other) noexcept
  {
    if (this != &other)
    {
      if (handle && handle != INVALID_HANDLE_VALUE)
      {
        CloseHandle(handle);
      }
      handle = other.handle;
      other.handle = NULL;
    }
    return *this;
  }

  /// Getter for the handle
  HANDLE GetHandle() const { return handle; }

  void SetHandle(HANDLE h)
  {
    if (handle && handle != INVALID_HANDLE_VALUE)
    {
      CloseHandle(handle);
    }
    handle = h;
  }

  /// Setter for the handle
  void SetHandle(DWORD dwProcId)
  {
    HANDLE h;

    CLIENT_ID cid = { (HANDLE)dwProcId, NULL };

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, 0, 0, 0, 0);

    ZwOpenProcess(&h, GENERIC_ALL, &oa, &cid);

    if (handle && handle != INVALID_HANDLE_VALUE)
    {
      CloseHandle(handle);
    }
    handle = h;
  }
};

class ZwMemTools
{
private:
  uintptr_t procId;
  HandleManager handleManager;

public:
  /// Constructor that takes a process name, finds ProcessID and opens a handle to the process
  explicit ZwMemTools(const std::wstring& processName) : procId(0), handleManager()
  {
    auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    HandleManager snapshotHandle(hSnapshot);

    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
      OutputDebugString(L"Failed to create snapshot\n");
      return;
    }

    PROCESSENTRY32 procEntry;
    procEntry.dwSize = sizeof(PROCESSENTRY32);

    BOOL hResult = Process32First(hSnapshot, &procEntry);
    do
    {
      if (processName.compare(procEntry.szExeFile) == 0)
      {
        procId = procEntry.th32ProcessID;
        handleManager.SetHandle(procId);
        return;
      }
    } while (Process32Next(hSnapshot, &procEntry));

    OutputDebugString(L"Failed to find process\n");
    return;
  }

  /// Gets the base address of a module
  bool GetModuleBase(std::wstring moduleName, uintptr_t& moduleBase)
  {
    auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->procId);
    HandleManager snapshotHandle(hSnapshot);

    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
      OutputDebugString(L"Failed to create snapshot\n");
      return false;
    }

    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof(modEntry);

    BOOL hResult;
    hResult = Module32First(hSnapshot, &modEntry);

    do
    {
      if (moduleName.compare(modEntry.szModule) == 0)
      {
        moduleBase = (uintptr_t)modEntry.modBaseAddr;
        return true;
      }

      hResult = Module32Next(hSnapshot, &modEntry);
    } while (hResult);

    OutputDebugString(L"Failed to find module\n");
    return false;
  }

  MODULEENTRY32 GetModule(std::wstring moduleName)
  {
    auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->procId);
    HandleManager snapshotHandle(hSnapshot);

    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
      OutputDebugString(L"Failed to create snapshot\n");
      MODULEENTRY32 failedModule{ -1 };
      return failedModule;
    }

    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof(modEntry);

    BOOL hResult;
    hResult = Module32First(hSnapshot, &modEntry);

    do
    {
      if (moduleName.compare(modEntry.szModule) == 0)
      {
        return modEntry;
      }

      hResult = Module32Next(hSnapshot, &modEntry);
    } while (hResult);

    OutputDebugString(L"Failed to find module\n");
    MODULEENTRY32 failedModule{ -1 };
    return failedModule;
  }

  /// Resolves the address of a single-offset pointer
  /// NOTE: The Resolve function could be considered useless, but I wrote it just to simplify the code in main
  uintptr_t Resolve(uintptr_t baseAddress, uintptr_t offset)
  {
    uintptr_t addr = baseAddress;
    if (ZwReadVirtualMemory(handleManager.GetHandle(), reinterpret_cast<void*>(baseAddress), &addr, sizeof(addr), NULL) != STATUS_SUCCESS)
    {
      OutputDebugString(L"Failed to read memory\n");
      return NULL;
    }
    addr += offset;
    return addr;
  }

  /// Resolves the address of a multi-offset pointer (pointer chain)
  uintptr_t ResolveDMA(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets)
  {
    uintptr_t addr = baseAddress;
    for (size_t i = 0; i < offsets.size(); ++i)
    {
      if (ZwReadVirtualMemory(handleManager.GetHandle(), reinterpret_cast<void*>(addr), &addr, sizeof(addr), NULL) != STATUS_SUCCESS)
      {
        OutputDebugString(L"Failed to read memory\n");
        return NULL;
      }

      addr += offsets[i];
    }
    return addr;
  }

  /// Wrapper for ReadProcessMemory
  template<typename T>
  T Read(uintptr_t address)
  {
    T val;
    if (ZwReadVirtualMemory(handleManager.GetHandle(), reinterpret_cast<void*>(address), &val, sizeof(T), NULL) != STATUS_SUCCESS)
    {
      OutputDebugString(L"Failed to read memory\n");
      return NULL;
    }
    return val;
  }

  bool ReadSized(uintptr_t address, size_t size, LPCVOID buffer)
  {
    if (ZwReadVirtualMemory(handleManager.GetHandle(), reinterpret_cast<void*>(address), &buffer, size, NULL) != STATUS_SUCCESS)
    {
      OutputDebugString(L"Failed to read memory\n");
      return NULL;
    }
    return true;
  }

  /// Wrapper for WriteProcessMemory
  template<typename T>
  bool Write(uintptr_t address, T& val)
  {
    if (ZwWriteVirtualMemory(handleManager.GetHandle(), reinterpret_cast<void*>(address), &val, sizeof(T), NULL) != STATUS_SUCCESS)
    {
      OutputDebugString(L"Failed to write memory\n");
      return false;
    }
  }

  /// Patches a region of memory with the given opcodes
  /// NOTE: The concept for patching and nopping bytes is taken from GuidedHacking, I rewrote it for better error handling
  SIZE_T Patch(uintptr_t address, const std::vector<BYTE>& opcodes)
  {
    DWORD oldProtect;
    SIZE_T bytes;

    std::vector<BYTE> newOpcodes = opcodes;

    if (!VirtualProtectEx(handleManager.GetHandle(), reinterpret_cast<void*>(address), opcodes.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
      OutputDebugString(L"Patch failed: Failed to change protection\n");
      return NULL;
    }

    if (ZwWriteVirtualMemory(handleManager.GetHandle(), reinterpret_cast<void*>(address), reinterpret_cast<void*>(newOpcodes.data()), opcodes.size(), &bytes) != STATUS_SUCCESS)
    {
      VirtualProtectEx(handleManager.GetHandle(), reinterpret_cast<void*>(address), opcodes.size(), oldProtect, &oldProtect);
      OutputDebugString(L"Patch failed: Failed to write memory\n");
      return NULL;
    }

    VirtualProtectEx(handleManager.GetHandle(), reinterpret_cast<void*>(address), opcodes.size(), oldProtect, &oldProtect);
    return bytes;
  }

  /// Patches a region of memory with NOPs
  SIZE_T Nop(uintptr_t address, SIZE_T numBytes)
  {
    std::vector<BYTE> nopArray(numBytes, 0x90);

    return this->Patch(address, nopArray);
  }

  uintptr_t PatternScan(std::vector<BYTE> signature, MODULEENTRY32 module, uintptr_t index)
  {
    std::vector<BYTE> memBuffer(module.modBaseSize);
    if (ZwReadVirtualMemory(handleManager.GetHandle(), module.modBaseAddr, memBuffer.data(), module.modBaseSize, NULL) != STATUS_SUCCESS)
    {
      OutputDebugString(L"Failed to copy memory to buffer\n");
      return NULL;
    }

    for (auto i = 0; i < module.modBaseSize; i++)
    {
      for (uintptr_t j = 0; j < signature.size(); j++)
      {
        if (signature.at(j) != 0x00 && signature.at(j) != memBuffer.at(i + j))
          break;
        if (j + 1 == signature.size())
          return reinterpret_cast<uintptr_t>(module.modBaseAddr + i + index);
      }
    }
  }
};