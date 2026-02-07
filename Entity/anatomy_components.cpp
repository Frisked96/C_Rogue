#include "anatomy_components.hpp"

// Recursive helper to find a part
BodyPart* findPartRecursive(const std::vector<std::unique_ptr<BodyPart>>& parts, const std::string& name) {
    for (const auto& part : parts) {
        if (part->name == name) {
            return part.get();
        }
        // Check children
        if (!part->internal_parts.empty()) {
            if (auto* found = findPartRecursive(part->internal_parts, name)) {
                return found;
            }
        }
    }
    return nullptr;
}

// Recursive helper to check functionality
bool checkVitalRecursive(const std::vector<std::unique_ptr<BodyPart>>& parts) {
    for (const auto& part : parts) {
        if (part->is_vital && !part->canFunction()) {
            return false;
        }
        if (!checkVitalRecursive(part->internal_parts)) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<Component> AnatomyComponent::clone() const {
  auto copy = std::make_unique<AnatomyComponent>();
  for (const auto &part : body_parts) {
    copy->addBodyPart(part->clonePart());
  }
  return copy;
}

void AnatomyComponent::addBodyPart(std::unique_ptr<BodyPart> part) {
  body_parts.push_back(std::move(part));
}

BodyPart *AnatomyComponent::getBodyPart(const std::string &name) {
  return findPartRecursive(body_parts, name);
}

bool AnatomyComponent::isFunctional() const {
  // Check all vital organs recursively
  return checkVitalRecursive(body_parts);
}

void AnatomyComponent::takeDamageToBodyPart(const std::string &part_name,
                                            int damage) {
  if (auto part = getBodyPart(part_name)) {
    part->takeDamage(damage);
  }
}