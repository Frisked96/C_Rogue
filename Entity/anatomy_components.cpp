
#include "anatomy_components.hpp"

void AnatomyComponent::addBodyPart(std::unique_ptr<BodyPart> part) {
  body_parts.push_back(std::move(part));
}

BodyPart *AnatomyComponent::getBodyPart(const std::string &name) {
  for (auto &part : body_parts) {
    if (part->name == name) {
      return part.get();
    }
  }
  return nullptr;
}

bool AnatomyComponent::isFunctional() const {
  // Check all vital organs
  for (const auto &part : body_parts) {
    if (part->is_vital && !part->canFunction()) {
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