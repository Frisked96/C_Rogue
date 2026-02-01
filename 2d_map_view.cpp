/*PURELY A STUDY MODULE WITH NO RELATION TO THE PROJECT*/




#include <iostream>
#include <vector>
#include <iomanip>  // For setw()

// Function to visualize the 2D vector map
void visualizeMap(const std::vector<std::vector<int>>& map) {
    std::cout << "=== 8x8 Vector Map Visualization ===\n\n";
    
    // Print column headers (0-7)
    std::cout << "     ";  // Space for row labels
    for (int col = 0; col < map[0].size(); col++) {
        std::cout << std::setw(4) << "C" << col;
    }
    std::cout << "\n";
    
    // Print top border
    std::cout << "     +";
    for (int col = 0; col < map[0].size(); col++) {
        std::cout << "----+";
    }
    std::cout << "\n";
    
    // Print each row with borders
    for (int row = 0; row < map.size(); row++) {
        // Row label
        std::cout << " R" << row << " |";
        
        // Row data
        for (int col = 0; col < map[row].size(); col++) {
            std::cout << std::setw(4) << map[row][col] << "|";
        }
        
        // Bottom border
        std::cout << "\n     +";
        for (int col = 0; col < map[row].size(); col++) {
            std::cout << "----+";
        }
        std::cout << "\n";
    }
}

// Function to visualize with position indices (row, col)
void visualizeMapWithIndices(const std::vector<std::vector<int>>& map) {
    std::cout << "\n=== Map with (row, col) indices ===\n\n";
    
    for (int row = 0; row < map.size(); row++) {
        for (int col = 0; col < map[row].size(); col++) {
            std::cout << "(" << row << "," << col << "):" 
                      << std::setw(3) << map[row][col] << "  ";
        }
        std::cout << "\n";
    }
}

// Function to visualize with sequential numbering
void visualizeSequentialMap() {
    std::cout << "\n=== Sequential Tile Numbers (0-63) ===\n\n";
    
    int tileNumber = 0;
    for (int row = 0; row < 8; row++) {
        std::cout << "|";
        for (int col = 0; col < 8; col++) {
            std::cout << std::setw(3) << tileNumber++ << " |";
        }
        std::cout << "\n";
    }
}

// Alternative: ASCII art style visualization
void visualizeASCIIMap(const std::vector<std::vector<int>>& map) {
    std::cout << "\n=== ASCII Grid Visualization ===\n\n";
    
    for (int row = 0; row < map.size(); row++) {
        // Top border
        for (int col = 0; col < map[row].size(); col++) {
            std::cout << "+-----";
        }
        std::cout << "+\n";
        
        // Middle row with numbers
        for (int col = 0; col < map[row].size(); col++) {
            std::cout << "| " << std::setw(3) << map[row][col] << " ";
        }
        std::cout << "|\n";
    }
    
    // Bottom border
    for (int col = 0; col < map[0].size(); col++) {
        std::cout << "+-----";
    }
    std::cout << "+\n";
}

int main() {
    // Create an 8x8 vector map
    const int ROWS = 8;
    const int COLS = 8;
    
    // Initialize the map with position numbers (row * 8 + col)
    std::vector<std::vector<int>> map(ROWS, std::vector<int>(COLS));
    
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            map[row][col] = row * COLS + col;
        }
    }
    
    // Visualize using different methods
    visualizeMap(map);
    visualizeMapWithIndices(map);
    visualizeASCIIMap(map);
    visualizeSequentialMap();
    
    // Display some useful information
    std::cout << "\n=== Map Information ===\n";
    std::cout << "Dimensions: " << ROWS << " rows × " << COLS << " columns\n";
    std::cout << "Total tiles: " << ROWS * COLS << "\n";
    std::cout << "Tile calculation: position = row × " << COLS << " + col\n\n";
    
    // Example: Accessing a specific tile
    std::cout << "Example access:\n";
    std::cout << "map[3][4] = " << map[3][4] << " (row 3, col 4)\n";
    std::cout << "map[7][7] = " << map[7][7] << " (last tile)\n";
    
    return 0;
}
