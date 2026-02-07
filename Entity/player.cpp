#include "player.hpp"
#include "components.hpp"
#include "anatomy_components.hpp"

Player::Player(int x, int y, const std::string &name, char glyph) {
  // components for player
  addComponent<PositionComponent>(x, y);
  addComponent<RenderComponent>(glyph);
  addComponent<NameComponent>(name);
  addComponent<BlockingComponent>();

  // Setup Anatomy
  auto& anatomy = addComponent<AnatomyComponent>();

  // 1. Torso (Root Container) - Center of body
  // Size: 0.6x0.8, Centered at 0,0
  auto torso = std::make_unique<BodyPart>("Torso", 50, true, false, 0.0f, 0.0f, 0.6f, 0.8f);

    // Add Heart inside Torso (vital)
    // Positioned slightly left and up relative to torso center
    auto heart = std::make_unique<Organ>("Heart", Organ::Type::Heart, 10, -0.1f, -0.1f, 0.15f, 0.15f, true);
    torso->addInternalPart(std::move(heart));

    // Add Lungs inside Torso
    auto lungs = std::make_unique<Organ>("Lungs", Organ::Type::Lung, 15, 0.1f, -0.1f, 0.2f, 0.2f, true);
    torso->addInternalPart(std::move(lungs));

  anatomy.addBodyPart(std::move(torso));

  // 2. Head (Root Container) - Above Torso
  // Positioned at 0, -0.5 (assuming y-down? or y-up? standard roguelike usually y-down)
  // Let's assume y-down (0 is top). So Head is 'above' visually, meaning lower Y? Or negative Y?
  // Usually negative Y is up in Cartesian, but roguelikes often use positive Y down.
  // I'll stick to 0,0 center. If Torso is height 0.8 (from -0.4 to 0.4), Head should be at -0.5 or -0.6.
  auto head = std::make_unique<Limb>("Head", Limb::Type::HEAD, 20, 0.0f, -0.6f, 0.3f, 0.3f, 1.0f, 1.0f, true);
    
    // Brain inside Head
    auto brain = std::make_unique<Organ>("Brain", Organ::Type::Brain, 5, 0.0f, 0.0f, 0.1f, 0.1f, true);
    head->addInternalPart(std::move(brain));

  anatomy.addBodyPart(std::move(head));

  // 3. Arms
  auto leftArm = std::make_unique<Limb>("Left Arm", Limb::Type::ARM, 30, -0.5f, 0.0f, 0.2f, 0.7f);
  anatomy.addBodyPart(std::move(leftArm));

  auto rightArm = std::make_unique<Limb>("Right Arm", Limb::Type::ARM, 30, 0.5f, 0.0f, 0.2f, 0.7f);
  anatomy.addBodyPart(std::move(rightArm));

  // 4. Legs
  auto leftLeg = std::make_unique<Limb>("Left Leg", Limb::Type::LEG, 30, -0.2f, 0.6f, 0.2f, 0.8f);
  anatomy.addBodyPart(std::move(leftLeg));

  auto rightLeg = std::make_unique<Limb>("Right Leg", Limb::Type::LEG, 30, 0.2f, 0.6f, 0.2f, 0.8f);
  anatomy.addBodyPart(std::move(rightLeg));
}

// when we add health component
void Player::takeDamage(int amount) {
  // empty for now
}

void Player::heal(int amount) {
  // empty for now
}