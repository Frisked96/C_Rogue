#include "engine.hpp"
#include <iostream>

Engine::Engine(int width, int height) 
    : is_running(true), screen_width(width), screen_height(height) {
    
    // Initialize map
    map = std::make_unique<Game_map>(width, height);
    map->generate();

    // Initialize renderer
    renderer = std::make_unique<Terminal_renderer>(width, height);

    // Initialize input handler
    input_handler = std::make_unique<InputHandler>();

    // Initialize player at center
    player = std::make_unique<Player>(width / 2, height / 2);
}

void Engine::run() {
    while (is_running) {
        render();
        handle_input();
    }
}

void Engine::render() {
    // Clear screen (ANSI escape code)
    std::cout << "\033[2J\033[1;1H";

    // 1. Draw map
    for (int y = 0; y < map->get_height(); ++y) {
        for (int x = 0; x < map->get_width(); ++x) {
            Tile t = map->get_tile(x, y);
            renderer->set_tile(x, y, t.glyph);
        }
    }

    // 2. Draw player (over map)
    renderer->set_tile(player->get_pos_x(), player->get_pos_y(), player->get_glyph());

    // 3. Render to screen
    renderer->draw();
    
    // Print stats/help
    std::cout << "Player: (" << player->get_pos_x() << ", " << player->get_pos_y() << ")\n";
    std::cout << "Controls: WASD to move, Q to quit.\n";
}

void Engine::handle_input() {
    std::cout << "> ";
    char input;
    std::cin >> input;
    
    Action action = input_handler->process_input(input);
    
    int dx = 0;
    int dy = 0;

    switch (action) {
        case Action::MOVE_UP: dy = -1; break;
        case Action::MOVE_DOWN: dy = 1; break;
        case Action::MOVE_LEFT: dx = -1; break;
        case Action::MOVE_RIGHT: dx = 1; break;
        case Action::QUIT: is_running = false; break;
        default: break;
    }

    if (dx != 0 || dy != 0) {
        int new_x = player->get_pos_x() + dx;
        int new_y = player->get_pos_y() + dy;

        if (map->can_walk(new_x, new_y)) {
            player->move(dx, dy);
        }
    }
}
