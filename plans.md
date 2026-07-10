
  ---

  1. The Core Philosophy: "Flyweight Actors"
  Just like tiles, we split objects into Static Definitions (What it is) and Dynamic Instances (Where/How it is).

   * Voxel (Tile): Static terrain, millions of them.
   * Object: Dynamic "props" or "actors," thousands of them.

  2. The Data Structure

  A. ObjectPrototype (The "Dumb" Template)
  Stored in a central database (Object_db). This is read-only during gameplay.

    1 struct ObjectPrototype {
    2     uint16_t id;
    3     std::string name;
    4     char glyph;
    5     int fg_color;
    6
    7     // Physical Properties
    8     float mass_kg;
    9     float max_health;
   10     MaterialType primary_material; // Links back to your material system
   11
   12     // Capabilities (Flags)
   13     bool is_living;      // Does it need a LifeState ticker?
   14     bool is_static;      // Trees/Rocks (true) vs NPCs/Items (false)
   15     bool is_container;   // Can it hold other objects?
   16
   17     // Economy/Social (Dumbed down)
   18     int base_value;      // Monetary value
   19     ProfessionType default_job;
   20 };

  B. ObjectInstance (The "Live" State)
  This is the "Tile-like" struct that exists in the world.

    1 struct ObjectInstance {
    2     uint32_t uid;         // Unique ID for referencing
    3     uint16_t prototype_id;
    4
    5     // Spatial
    6     float x, y, z;        // Floating point for smooth movement, or int for grid-locking
    7
    8     // Vitality
    9     float health;
   10     uint32_t owner_uid;   // For economy/theft logic
   11
   12     // Living State (Sparse Pointer)
   13     // Only allocated if prototype->is_living is true
   14     std::unique_ptr<LifeState> life;
   15 };

  C. LifeState (The NPC "Needs" Module)
  This is the "dumbed-down" brain. No limbs, no complex biology—just variables for the AI to react to.

    1 struct LifeState {
    2     // Basic Needs (0.0 to 1.0)
    3     float hunger = 0.0f;
    4     float fatigue = 0.0f;
    5     float morale = 1.0f;
    6
    7     // Economy
    8     int wallet = 0;
    9     ProfessionType profession;
   10
   11     // AI Goal (Simple state machine)
   12     GoalType current_goal = GoalType::IDLE;
   13     uint32_t target_uid = 0; // UID of an object they are interacting with
   14 };

  ---

  3. The Management Systems

  To make this work with your 3D grid, you need two specialized managers:

  1. The Spatial Grid (Registry)
  Instead of every tile knowing what object is on it (which wastes memory), use a Spatial Hash or a std::multimap<VoxelIndex, ObjectUID>.
   * When an NPC moves, you only update the map for the old and new voxel.
   * When rendering a tile, you query the map: "Are there any objects at (x,y,z)?"

  2. The Ticker (Interaction Engine)
  You run three different "frequencies" to save CPU:
   * High Frequency (Every Frame): Only for the Player and nearby moving NPCs.
   * Medium Frequency (Every 10-30 turns): Update LifeState (Hunger increases, NPC decides to go to work).
   * Low Frequency (Every 100+ turns): Vegetation growth, tree spreading, item decay.

  ---

  4. Why this is "Smart" for your Economy Plan

  By treating NPCs as "Objects with a LifeState," the economy becomes a simple Resource Interaction:

   1. NPC Needs: The AI sees LifeState.hunger > 0.8.
   2. World Search: It looks for an ObjectInstance where prototype->name == "Apple" or prototype->is_harvestable.
   3. Action: It moves to the object.
   4. Transaction:
       * If the Apple is in a "Shop" (Owner UID is a Merchant), the NPC triggers a TradeEvent.
       * NPC.wallet -= Apple.value; Merchant.wallet += Apple.value.
       * The Apple Object is moved into the NPC.inventory.

  5. Integration with your Detailed Player Entity
  The Player remains a "Detailed Entity" (with limbs, blood, oxygen). When the Player hits an NPC "Object":
   * The Combat Module translates the Player's detailed swing into a simple Damage value.
   * The NPC Object simply subtracts Damage from health.
   * If the NPC dies, it swaps its prototype_id to Human_Corpse and drops its inventory objects.

  Summary of Benefits:
   1. Uniformity: Moving a rock, a tree, or a human uses the same ObjectInstance logic.
   2. Memory: A tree doesn't need a LifeState, so it only costs ~32-64 bytes.
   3. Simplicity: You can simulate a village of 200 NPCs using simple "Need" variables rather than thousands of individual organs and bones.
   4. Scalability: You can easily add "Animals" just by giving them a LifeState with profession = PROFESSION_WILD.