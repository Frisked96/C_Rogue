#include "anatomy_components.hpp"
#include "biological_tags.hpp"
#include "components.hpp"
#include <algorithm>
#include <limits>

std::unique_ptr<Component> AnatomyComponent::clone() const {
  auto copy = std::make_unique<AnatomyComponent>();
  copy->physiology_config = physiology_config;
  copy->blood_volume = blood_volume;
  copy->max_blood_volume = max_blood_volume;
  copy->oxygen_saturation = oxygen_saturation;
  copy->stored_energy = stored_energy;
  copy->hydration = hydration;
  copy->accumulated_pain = accumulated_pain;

  // Directly copy the vector since BodyPart is now a simple struct-like class
  copy->body_parts = body_parts;
  return copy;
}

void AnatomyComponent::updateSpatialProfile(SpatialProfileComponent &profile) {
  float total_volume = 0.0f;
  float total_area = 0.0f;
  float min_y = std::numeric_limits<float>::max();
  float max_y = std::numeric_limits<float>::lowest();

  bool has_parts = false;

  for (const auto &part : body_parts) {
    // Skip organs for spatial profile (they are inside)
    if (part.type == BodyPartType::ORGAN) {
        continue;
    }

    has_parts = true;
    
    // Volume: Width * Height * Depth
    total_volume += part.width * part.height * part.depth;

    // Coverage (Projected Area): Width * Height
    // Note: This assumes no overlap, which is an overestimation, but acceptable for now.
    total_area += part.width * part.height;

    // Height Calculation (Simple bounding box on Y axis)
    // Assuming relative_y is effectively the center offset from entity root
    float part_top = part.relative_y - (part.height / 2.0f);
    float part_bottom = part.relative_y + (part.height / 2.0f);

    if (part_top < min_y) min_y = part_top;
    if (part_bottom > max_y) max_y = part_bottom;
  }

  if (has_parts) {
    profile.volume_occupancy = total_volume; // In cubic meters (approx)
    profile.cross_section_coverage = total_area; // In square meters
    profile.height_meters = max_y - min_y;
  } else {
    // Fallback if no structural parts
    profile.volume_occupancy = 0.0f;
    profile.cross_section_coverage = 0.0f;
    profile.height_meters = 0.0f;
  }

  // Adjust for Stance
  switch (profile.current_stance) {
    case SpatialProfileComponent::Stance::CROUCHING:
      profile.height_meters *= 0.6f;
      profile.cross_section_coverage *= 0.7f; // Harder to hit
      break;
    case SpatialProfileComponent::Stance::PRONE:
    case SpatialProfileComponent::Stance::CRAWLING:
      profile.height_meters *= 0.2f;
      profile.cross_section_coverage *= 0.4f; // Very hard to hit from side
      // Volume remains the same
      break;
    case SpatialProfileComponent::Stance::FLYING:
    case SpatialProfileComponent::Stance::STANDING:
    default:
      break;
  }
}


int AnatomyComponent::addBodyPart(const BodyPart &part) {
  body_parts.push_back(part);
  return body_parts.size() - 1;
}

int AnatomyComponent::addChildPart(int parentIndex, const BodyPart &part) {
  if (parentIndex < 0 || parentIndex >= body_parts.size())
    return -1;

  int childIndex = addBodyPart(part);
  body_parts[childIndex].parent_index = parentIndex;
  body_parts[parentIndex].children_indices.push_back(childIndex);

  return childIndex;
}

BodyPart *AnatomyComponent::getBodyPart(const std::string &name) {
  for (auto &part : body_parts) {
    if (part.name == name) {
      return &part;
    }
  }
  return nullptr;
}

int AnatomyComponent::getBodyPartIndex(const std::string &name) {
  for (int i = 0; i < body_parts.size(); ++i) {
    if (body_parts[i].name == name) {
      return i;
    }
  }
  return -1;
}

bool AnatomyComponent::isFunctional() const {
  for (const auto &part : body_parts) {
    if (part.is_vital && !part.canFunction()) {
      // If the part is vital for a function that is not needed, ignore it
      if (part.hasTag(BioTags::RESPIRATION) &&
          !physiology_config.needs_oxygen) {
        continue;
      }
      if (part.hasTag(BioTags::CIRCULATION) && !physiology_config.has_blood) {
        continue;
      }
      if (part.hasTag(BioTags::NEURAL) &&
          !physiology_config.has_nervous_system) {
        continue;
      }
      return false;
    }
  }
  return true;
}

void AnatomyComponent::takeDamageToBodyPart(const std::string &part_name,
                                            int damage) {
  if (auto part = getBodyPart(part_name)) {
    part->takeDamage(damage);
  }
}

void AnatomyComponent::replaceBodyPart(int index, const BodyPart &newPart) {
  if (index < 0 || index >= body_parts.size())
    return;

  // Capture existing hierarchy
  int oldParent = body_parts[index].parent_index;
  std::vector<int> oldChildren = body_parts[index].children_indices;

  // Overwrite
  body_parts[index] = newPart;

  // Restore hierarchy
  body_parts[index].parent_index = oldParent;
  body_parts[index].children_indices = oldChildren;
}
