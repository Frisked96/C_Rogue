#pragma once
#include <cstdint>

namespace BioTags {
    using TagType = uint64_t;

    // Vital Functions
    constexpr TagType CIRCULATION  = 1ULL << 0;
    constexpr TagType RESPIRATION  = 1ULL << 1;
    constexpr TagType NEURAL       = 1ULL << 2;

    // Sustainment Functions
    constexpr TagType DIGESTION    = 1ULL << 3;
    constexpr TagType FILTRATION   = 1ULL << 4;
    constexpr TagType ENDOCRINE    = 1ULL << 5;

    // Capability Functions
    constexpr TagType MOTILITY     = 1ULL << 6;
    constexpr TagType MANIPULATION = 1ULL << 7;
    constexpr TagType SIGHT        = 1ULL << 8;

    // Exotic / Mutation Functions
    constexpr TagType REGENERATION = 1ULL << 9;
    constexpr TagType THERMAL_GEN  = 1ULL << 10;
    constexpr TagType TOXIN_GEN    = 1ULL << 11;

    // Helper to get tag from string if needed (legacy or data loading)
    // For now we just use the constants.
}