#pragma once
#include "object_enums.hpp"
#include <cstdint>

// The NPC "Needs" Module -- a dumbed-down brain.
// No limbs, no complex biology -- just variables for AI to react to.
// Only allocated for objects where prototype->is_living is true.
struct LifeState {
    // Basic Needs (0.0 to 1.0)
    float hunger  = 0.0f;   // 0 = full, 1 = starving
    float fatigue = 0.0f;   // 0 = rested, 1 = exhausted
    float morale  = 1.0f;   // 0 = depressed, 1 = happy

    // Economy
    int wallet = 0;
    ProfessionType profession = ProfessionType::NONE;

    // AI Goal (Simple state machine)
    GoalType current_goal = GoalType::IDLE;
    uint32_t target_uid = 0;  // UID of an object they are interacting with

    // Needs rates (units per turn, set from prototype defaults)
    float hunger_rate  = 0.001f;
    float fatigue_rate = 0.0005f;
    float morale_decay = 0.0002f;
};
