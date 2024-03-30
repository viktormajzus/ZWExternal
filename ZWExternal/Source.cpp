#pragma comment(lib, "ntdll.lib")

#include <Windows.h>
#include <TlHelp32.h>

#include <iostream>
#include <thread>
#include <chrono>

#include "memtools.h"

int main()
{
  ZwMemTools mem = ZwMemTools(L"sauerbraten.exe", PROCESS_ALL_ACCESS);
  uintptr_t moduleBase;
  mem.GetModuleBase(L"sauerbraten.exe", moduleBase);
  moduleBase += 0x346C90;

  int newVal = 50;
  uintptr_t address = 0x1B431AAF25C;

  uintptr_t addressOfBytes = 0x7FF734F3B5E0;
  std::vector<BYTE> opcodes = { 0x41, 0x89, 0x84, 0x8E, 0x94, 0x01, 0x00, 0x00 };

  mem.Write<int>(reinterpret_cast<PULONGLONG>(address), newVal);

  mem.Nop(addressOfBytes, opcodes.size());

  mem.Patch(addressOfBytes, opcodes);

  

  return EXIT_SUCCESS;
}