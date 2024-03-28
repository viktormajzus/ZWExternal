#include <Windows.h>
#include <TlHelp32.h>

#include <iostream>
#include <thread>
#include <chrono>

#include "memtools.h"

int main()
{
  ZwMemTools mem = ZwMemTools(L"sauerbraten.exe");
  uintptr_t moduleBase;
  if (!mem.GetModuleBase(L"sauerbraten.exe", moduleBase))
    return EXIT_FAILURE;

  uintptr_t localPlayer = mem.Read<uintptr_t>(moduleBase + 0x2A2560);

  bool bAmmo = false;
  bool bHealth = false;
  while (true)
  {
    if (GetAsyncKeyState(VK_END) & 1)
      break;

    if (GetAsyncKeyState(VK_NUMPAD1) & 1)
    {
      bAmmo = !bAmmo;
      if (bAmmo)
        mem.Nop((uintptr_t)0x7FF6B0D2B5E0, 8);
      else
        mem.Patch((uintptr_t)0x7FF6B0D2B5E0, { 0x41, 0x89, 0x84, 0x8E, 0x94, 0x01, 0x00, 0x00 });
    }

    if (GetAsyncKeyState(VK_NUMPAD2))
      bHealth = !bHealth;

    if (bHealth)
    {
      uintptr_t healthAddr = localPlayer + 0x178;
      uint32_t healthVal = 1000;
      mem.Write<uint32_t>(healthAddr, healthVal);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return EXIT_SUCCESS;
}