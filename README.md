# ZWExternal

DISCLAIMER: I am not responsible for any damage caused by this code. This is purely educational and should not be used to violate any laws and/or copyright restrictions. You are responsible for anything you do with this code, I cannot be held responsible.

A proof of concept for my new memory tools class. This directly calls the ZwWriteVirtualMemory, ZwReadVirtualMemory and ZwOpenProcess from ntdll.dll, this means that usermode anti-cheats should not be able to hook these functions and see whether you've called them or not.

This example uses two different methods to change the code of the game. The first method is to patch the assembly responsible for decrementing the player's ammo with nop instructions, and the second is writing to the address which stores the player's health. These features won't work in multiplayer, because in multiplayer the server stores these variables instead of the client.

The example mod in this code is for the game Sauerbraten Cube 2. This mod does not violate the license for Sauerbraten Cube 2, as modifications to the game are allowed.

I aim to improve this code further, my next step is to improve the pattern scanning function and to get rid of vectors for offsets and use std::array instead.
