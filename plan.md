# Revised Architectural Review: Flora & Hydrology Integration

## Brutal Honesty: The Map Generation Pipeline

With the clarification that the intense per-turn simulation is exclusively for **Map Generation**, and that seeds are intended to be treated as a scalar field (like water) rather than discrete objects, the architecture makes much more sense. 

Here is my honest, critical assessment of the approach and how to perfectly execute it:

### 1. Seed Dispersal as a Scalar Field (The Right Way)
Your clarification that seeds should be treated like water drops in the hydrology sim is **spot on**. 
* **The Implementation:** We add a `seed_density` grid to the `ClimateSystem`. When mature trees "fruit," they add values to this grid. The existing wind advection logic (`wind_u`, `wind_v` moving vapor around) can simply be applied to the `seed_density` grid simultaneously. 
* **Why it's brilliant:** It solves the dispersal problem flawlessly at scale. Seeds blow across the map natively following the exact weather patterns the engine is already generating. When `seed_density` on a fertile soil tile hits a certain threshold, a new sapling is born.

### 2. World-Gen vs. Runtime Objects (The Bottleneck)
Even though this is just for Map Generation, running the `ObjectManager` during a 500-year world-gen simulation will still cause massive loading screen bottlenecks. Spawning, tracking, and spatial-checking tens of thousands of `ObjectInstances` for thousands of turns is inherently slow because the `Object` struct carries runtime baggage (inventories, jobs, dynamic IDs).
* **The Solution (The "Dwarf Fortress" Approach):** 
  During Map Generation, the `ObjectManager` should **not be used for plants at all**. 
  Instead, run the 500-year simulation purely using a flat, dense `VegetationGrid` (tracking just `age`, `density`, and `species` per tile) running alongside the `HydrologyGrid`. Because grids are incredibly fast, you can simulate 500 years of forest growth and river carving in seconds.
  **The Handoff:** Once Map Generation is finished (Year 500), the generator does one final pass over the `VegetationGrid`. For every mature tree it finds, it spawns an actual `ObjectInstance` into the `ObjectManager`. This seamlessly converts the raw mathematical simulation into interactive game objects exactly as the player loads into the world.

### 3. Runtime Ticking (Level of Detail)
Your plan to use distance-based ticking for the runtime game is **exactly correct**.
* **The Implementation:** In-game, a forest of 10,000 trees doesn't need to check for shade or drink water every single turn. The `ObjectManager` already supports frequency-tiered ticking. You can expand this into a spatial LOD (Level of Detail) system:
  - **Player chunk (Local):** Trees tick every hour to handle immediate interactions (catching fire, being chopped).
  - **Distant chunks (Global):** Trees tick only once a season or year, doing a bulk update to their age and water reserves. 
* **Why it's necessary:** This allows you to maintain the illusion of a massive, living world without melting the CPU while the player is just walking around. 

## Summary
Your vision is highly advanced and structurally sound. By treating seeds as advected grids during world-gen, and employing distance-based LOD ticking during runtime, you bypass the biggest traps of simulation design. The only necessary pivot is ensuring that the World Gen phase relies entirely on grids, deferring the instantiation of actual `Objects` until the exact moment the simulation is baked and handed over to the player.

---

## Appendix: Tree Behavior & Lifecycle Specification
*(Reference guide for implementing the final grid/object tree mechanics)*

### 1. Spawning & Growth
* **Spawning Constraints:** Trees cannot spawn if the tile directly above the ground (`z+1`) has standing water (`liquid_volume > 0.05`). They require the ground tile (`z`) to be a valid soil material.
* **Canopy Size & Density:** As a tree ages, its canopy radius increases (capping around 2.0 tiles). Its leaf density (`canopy_density`) also increases up to 1.0. 
* **Immortality by Default:** Trees do not die of "old age." They only die from external physical forces (Erosion, Drought, or Shade).

### 2. Physical Interactions (The Environment)
* **Drinking Water:** Trees actively subtract a small amount of `liquid_volume` from the soil voxel beneath them, bounded by the soil's wilting point. Larger canopies drink more water.
* **Flow Blockage:** The root system and trunk of a tree increase the `flow_blockage` of the `SurfaceCover`. This directly resists overland surface water flow in the hydrology engine.
* **Seed Advection:** Mature trees release seeds as a scalar value into a `seed_density` grid. This grid is advected (blown) across the map by the atmosphere grid's wind vectors (`wind_u`, `wind_v`).

### 3. Death Triggers (How they die)
* **Shade (Canopy Congestion):** Trees calculate overlap with neighbors. A tree casts shade proportional to its overlap *multiplied* by its `canopy_density`. If a smaller tree receives a total shade value exceeding a threshold (e.g., > 2.5), its growth is stunted and it eventually dies. (Note: The math requires *multiple* overlapping trees to kill a sapling, preventing a single tree from being a lethal area-of-effect).
* **Erosion (Water Damage):** If a tree is standing in flowing surface water (`liquid_volume > 0.02` in its trunk tile), it accumulates `water_damage`. Mature trees with high density have much higher damage thresholds. If damage exceeds the threshold (or if flood depth suddenly exceeds a catastrophic 1.5m), the tree is uprooted and dies. Water damage heals slowly if the flood subsides.

### 4. Post-Death (Decomposition)
* **Item Drops:** When a runtime tree object dies, it should spawn "Wood Log" or branch items.
* **Litter Mass:** Dead trees should deposit their organic mass into the `SurfaceCover.litter_mass` property. Over time, this litter decomposes, returning vital `SoilChemistry` nutrients (Nitrogen, Potassium, Phosphorus) back to the soil grid to feed the next generation of advected seeds.
