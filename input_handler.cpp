#include "input_handler.hpp"
#include <cctype> // For tolower()
#include <iostream>

InputHandler::InputHandler() { init_key_bindings(); }

void InputHandler::init_key_bindings() {
  // Primary WASD movement
  key_bindings['w'] = Action::MOVE_UP;
  key_bindings['a'] = Action::MOVE_LEFT;
  key_bindings['s'] = Action::MOVE_DOWN;
  key_bindings['d'] = Action::MOVE_RIGHT;

  // Alternative arrow keys (if you add terminal arrow support later)
  key_bindings['8'] = Action::MOVE_UP;    // Numpad up
  key_bindings['4'] = Action::MOVE_LEFT;  // Numpad left
  key_bindings['2'] = Action::MOVE_DOWN;  // Numpad down
  key_bindings['6'] = Action::MOVE_RIGHT; // Numpad right

  // Other actions
  key_bindings['q'] = Action::QUIT;
  key_bindings[' '] = Action::WAIT; // Spacebar to wait
  key_bindings['.'] = Action::WAIT; // Traditional roguelike wait
  key_bindings['5'] = Action::WAIT; // Numpad wait
  key_bindings['i'] = Action::INVENTORY;
  key_bindings['l'] = Action::LOOK; // 'l' for look
  key_bindings['?'] = Action::HELP;
  key_bindings['r'] = Action::RESTART;  // Restart game
  key_bindings['+'] = Action::ZOOM_IN;  // Zoom in
  key_bindings['-'] = Action::ZOOM_OUT; // Zoom out
  key_bindings['='] = Action::ZOOM_IN;  // Alternative zoom in
}

Action InputHandler::process_input(char input) const {
  // Convert to lowercase for case-insensitive input
  char lower_input = std::tolower(static_cast<unsigned char>(input));

  // Look up the action in key bindings
  auto it = key_bindings.find(lower_input);
  if (it != key_bindings.end()) {
    return it->second;
  }

  return Action::NONE;
}

Action InputHandler::process_string(const std::string &input) const {
  // For now, just process first character
  if (!input.empty()) {
    return process_input(input[0]);
  }
  return Action::NONE;
}

void InputHandler::bind_key(char key, Action action) {
  key_bindings[std::tolower(static_cast<unsigned char>(key))] = action;
}

std::string InputHandler::get_controls_help() const {
  std::string help = "=== CONTROLS ===\n";
  help += "  Movement:    WASD or Numpad 8246\n";
  help += "  Wait:        Space, ., or 5\n";
  help += "  Inventory:   i\n";
  help += "  Look:        l\n";
  help += "  Help:        ?\n";
  help += "  Quit:        q\n";
  help += "  Restart:     r\n";
  help += "  Zoom:        +/-\n";
  help += "================\n";
  return help;
}

std::string InputHandler::action_to_string(Action action) {
  switch (action) {
  case Action::MOVE_UP:
    return "MOVE_UP";
  case Action::MOVE_DOWN:
    return "MOVE_DOWN";
  case Action::MOVE_LEFT:
    return "MOVE_LEFT";
  case Action::MOVE_RIGHT:
    return "MOVE_RIGHT";
  case Action::QUIT:
    return "QUIT";
  case Action::WAIT:
    return "WAIT";
  case Action::INVENTORY:
    return "INVENTORY";
  case Action::LOOK:
    return "LOOK";
  case Action::HELP:
    return "HELP";
  case Action::RESTART:
    return "RESTART";
  case Action::ZOOM_IN:
    return "ZOOM_IN";
  case Action::ZOOM_OUT:
    return "ZOOM_OUT";
  case Action::NONE:
    return "NONE";
  default:
    return "UNKNOWN";
  }
}
