# C_Rogue: Detailed Roguelike Engine

C_Rogue is a 3D grid-based roguelike engine written in C++17. It features a sophisticated Entity Component System (ECS) and a highly detailed anatomy and physiology simulation system.

## Project Overview

- **Language:** C++17
- **Compiler:** clang++ (configured in Makefile)
- **Architecture:** custom Entity Component System (ECS)
- **World Model:** 3D Grid (x, y, z)
- **Rendering:** Terminal-based character grid
- **Key Feature:** Detailed anatomy/physiology simulation (blood, oxygen, pain, hierarchical body parts).

## Building and Running

### Prerequisites
- LLVM/Clang++ (path configured as `C:\Program Files\LLVM\bin\clang++.exe`)
- `make` (or `mingw32-make` on Windows)

### Build Commands
- **Build All:** `make`
- **Clean Build:** `make clean; make`
