#pragma once

#include <map>
#include <string>

// Define possible actions
enum class Action {
  NONE,
  MOVE_UP,
  MOVE_DOWN,
  MOVE_LEFT,
  MOVE_RIGHT,
  QUIT,
  WAIT,
  INVENTORY,
  LOOK,
  HELP,
  RESTART, // Added for variety
  ZOOM_IN,
  ZOOM_OUT
};

class InputHandler {
private:
  // Key bindings - map from input character to action
  std::map<char, Action> key_bindings;

  // Initialize default key bindings (WASD focus)
  void init_key_bindings();

public:
  InputHandler();

  // Process a single character input
  Action process_input(char input) const;

  // Process a string (could be useful for commands later)
  Action process_string(const std::string &input) const;

  // Change a key binding
  void bind_key(char key, Action action);

  // Get help text for controls
  std::string get_controls_help() const;

  // Get action name (for debugging)
  static std::string action_to_string(Action action);
};
