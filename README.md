# Mirror's Edge Catalyst: Offline Loading Fix (Network Shim)

[![Build and Release](https://github.com/xpawelsky/me-catalyst-offline-menu-fix/actions/workflows/build.yml/badge.svg)](https://github.com/xpawelsky/me-catalyst-offline-menu-fix/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A lightweight, machine-independent native x64 shim DLL that resolves the **infinite startup loading screen hang** in *Mirror's Edge Catalyst* following the official shutdown of its online multiplayer infrastructure.

---

## 🎯 The Problem

In December 2023, the online servers for *Mirror's Edge Catalyst* were officially retired. On startup, the game continues to attempt contacting decommissioned online service endpoints.

Because these remote services no longer respond, the game's engine loop stalls waiting through prolonged socket timeouts, causing the game to hang indefinitely on the post-intro loading screen.

When network requests fail immediately, the engine cleanly detects the offline state and transitions straight into single-player mode.

---

## ⚡ How This Shim Works

1. **Proxy Injection via `Engine.BuildInfo_Win64_retail.dll`:**
   - The game engine loads `Engine.BuildInfo_Win64_retail.dll` directly from the game root at boot.
   - The shim forwards the single export `getBuildInfo` to the original DLL (`Engine.BuildInfo_Win64_retail_original.dll`), resolving the file dynamically relative to its own loaded path.
   - **No Graphics / Input Interception:** Keeps `dxgi.dll`, `d3d11.dll`, `dinput8.dll`, and `xinput*.dll` untouched. Fully compatible with **Frosty Mod Manager**, **ReShade**, and controller wrappers.
   - **No DRM Modification:** Leaves game executables, Denuvo, Steam, and EA App licensing intact.

2. **Network Fail-Fast Interception:**
   - Intercepts Winsock socket connection and name resolution requests directed at the retired online service endpoints.
   - Immediately returns standard offline / connection refused error codes (`WSAHOST_NOT_FOUND` / `WSAECONNREFUSED`) without waiting for network timeouts.
   - The engine instantly recognizes that online services are unavailable and proceeds directly to the main menu.

---

## 🚀 Quick Start (Prebuilt Release)

1. Go to the [**Releases**](https://github.com/xpawelsky/me-catalyst-offline-menu-fix/releases) page and download `MEC-Offline-Loading-Shim.zip`.
2. Extract the archive.
3. Open your *Mirror's Edge Catalyst* installation directory (where `MirrorsEdgeCatalyst.exe` resides).
4. Rename `Engine.BuildInfo_Win64_retail.dll` to:
   ```text
   Engine.BuildInfo_Win64_retail_original.dll
   ```
5. Copy the extracted `Engine.BuildInfo_Win64_retail.dll` into the game directory.
6. Launch the game normally via Steam or EA App!

---

## 🔄 Rollback / Uninstallation

To restore the game to its unmodified state:
1. Delete `Engine.BuildInfo_Win64_retail.dll` from the game folder.
2. Rename `Engine.BuildInfo_Win64_retail_original.dll` back to `Engine.BuildInfo_Win64_retail.dll`.

---

## 🛠️ Building from Source

No proprietary libraries or headers are required. The project is completely self-contained.

### Method 1: Using CMake (MSVC / Clang / MinGW)

```cmd
git clone https://github.com/xpawelsky/me-catalyst-offline-menu-fix.git
cd me-catalyst-offline-menu-fix
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The output DLL will be in `build/Release/Engine.BuildInfo_Win64_retail.dll` (or `build/Engine.BuildInfo_Win64_retail.dll`).

### Method 2: Using the Batch Script (`build.bat`)

Requires [MinGW-w64](https://www.mingw-w64.org/) (GCC) in your `PATH` or standard installation directory:
```cmd
build.bat
```

---

## 📜 Credits & License

- Licensed under the [MIT License](LICENSE).
- Includes [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu (BSD 2-Clause License).

---

## ⚖️ Legal Disclaimer & Notice

- **Unofficial Community Project:** This project is an independent, community-developed open-source modification and is **not** affiliated with, endorsed by, sponsored by, or associated with Electronic Arts Inc., EA Digital Illusions CE AB (DICE), or Valve Corporation.
- **Trademarks:** *"Mirror's Edge"*, *"Mirror's Edge Catalyst"*, *"Frostbite"*, and *"EA"* are trademarks or registered trademarks of Electronic Arts Inc. *"Steam"* is a trademark of Valve Corporation. All trademarks, service marks, and trade names are the property of their respective owners and are used strictly under nominative fair use to identify software compatibility.
- **No DRM Circumvention:** This software does **not** bypass, crack, disable, or tamper with any Digital Rights Management (DRM), anti-cheat, anti-tamper (such as Denuvo), or licensing verification systems. A legitimate, legally purchased copy of *Mirror's Edge Catalyst* is strictly required to play the game.
- **No Copyrighted Assets:** This repository contains only original open-source code and permissive third-party open-source libraries. It does not distribute, host, or link to any copyrighted game binaries, media, or proprietary EA/DICE assets.
- **Warranty Disclaimer:** This software is provided "as is", without warranty of any kind, express or implied. Use at your own risk.
