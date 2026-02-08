#include "anatomy_components.hpp"

std::unique_ptr<Component> AnatomyComponent::clone() const {
  auto copy = std::make_unique<AnatomyComponent>();
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

int AnatomyComponent::addBodyPart(const BodyPart& part) {
    body_parts.push_back(part);
    return body_parts.size() - 1;
}

int AnatomyComponent::addChildPart(int parentIndex, const BodyPart& part) {
    if (parentIndex < 0 || parentIndex >= body_parts.size()) return -1;

    int childIndex = addBodyPart(part);
    body_parts[childIndex].parent_index = parentIndex;
    body_parts[parentIndex].children_indices.push_back(childIndex);

    return childIndex;
}

BodyPart *AnatomyComponent::getBodyPart(const std::string &name) {
  for (auto& part : body_parts) {
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
  for (const auto& part : body_parts) {
      if (part.is_vital && !part.canFunction()) {
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
