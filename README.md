# 💾 SFSE Save Unfinished Ships
Starfield Script Extender plugin that allows the user to bypass spaceship builder limitations. This can be used to save unfinished ships or to build ships that normally wouldn't be possible.

https://www.nexusmods.com/starfield/mods/5236

# How to use
1. Install SFSE (https://www.nexusmods.com/starfield/mods/106)
2. Drop the .dll file in the `<Starfield>\Data\SFSE\Plugins` directory.
3. When in-game, press `F6` to disable all limitations. (You can press `F6` again if you want to re-enable the limitations.)
   F6 only reacts while the Starfield window is focused, and holding it down toggles only once.

# Warning
With the limitations disabled the ship builder lets you save ships the game normally rejects
(no cockpit, no landing gear, unreachable docker or landing bay, too heavy to grav jump, ...).
Such ships can be unflyable, impossible to board or take off with, or cause other problems.
**Make a hard save before using it**, and keep a working ship you can switch back to.

# Troubleshooting
The plugin writes a log next to the DLL (`Data\SFSE\Plugins\SaveUnfinishedShips.log`).
If initialization fails after a game update, the log says which step failed and the plugin stays inactive.
Don't install two copies of the plugin (for example two differently named builds): both would react to F6 and undo each other.

# Building
Requires Visual Studio 2022 (or its Build Tools) with the C++ workload, which includes CMake.
Capstone 5.0.9 is downloaded and verified by hash during configure.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build\Release\SaveUnfinishedShips.dll`.
`build\Release\scan_test.exe <unpacked Starfield.exe>` runs the plugin's scanning code against a game executable
without starting the game, and checks that every patch can be applied and reverted.

# Credits
This was based on https://github.com/SomeCrazyGuy/MinimalSFSEPlugin

I'm using string-references and the Capstone disassembler (https://github.com/capstone-engine/capstone) to find functions or specific instructions in the game's memory.
