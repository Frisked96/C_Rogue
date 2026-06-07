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
- **Clean Build:** `make clean && make`

### Running
- Execute the generated binary: `./main.exe`

## Development Conventions

### Entity Component System (ECS)
- **Entities:** Managed by `EntityManager`. Entities are essentially IDs with a map of components and a bitset `Signature` for fast system filtering.
- **Components:** Must inherit from `BaseComponent<T>` (in `Entity/component.hpp`). This provides static type IDs and bitset IDs.
- **Systems:** Logic is encapsulated in system classes (e.g., `AnatomySystem`, `PhysiologySystem`, `SpatialSystem`). Systems are managed by `SystemManager`.
- **Entity Creation:** Use `EntityFactory` to create complex entities (like the player or NPCs) with predefined component sets.

### Anatomy & Physiology System
This is the core of the game's simulation:
- **`AnatomyComponent`:** Stores a list of `BodyPart` objects.
- **`BodyPart`:** Represents a limb or organ. They are hierarchical (`parent_index`, `children_indices`).
- **Simulated Metrics:**
    - `blood_volume`
    - `oxygen_saturation`
    - `stored_energy`
    - `accumulated_pain`
- **Systems:** `AnatomySystem` handles limb status and vitals, while `PhysiologySystem` (implied) likely handles the metabolic processes.

### Map and Rendering
- **`Game_map`:** A 3D vector of `Tile` objects. Coordinates are `(x, y, z)`.
- **`Terminal_renderer`:** Manages a `view_grid` and renders it to the console. It separates map rendering, entity rendering, and UI.

### Input Handling
- Managed by `InputHandler`.
- Default bindings are WASD for movement.
- Supports 3D movement (Level Up/Down) and various game actions (Inventory, Look, etc.).

## Directory Structure
- `Entity/`: ECS core, components, and systems.
- `Entity/components/`: Specific component definitions for anatomy (limb, heart, etc.).
- Root: Engine, Map, Renderer, and Input handling.
- `FastNoiseLite.h`: Used for procedural map generation.

