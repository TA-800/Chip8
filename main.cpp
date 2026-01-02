#include <bitset>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <SFML/Graphics.hpp>

uint16_t characters[16 * 5] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

constexpr uint8_t SCALE = 10;

typedef struct hardware {
    // Order from smallest to biggest for alignment
    uint8_t memory[4096]{};
    // 64 x 32 pixels black or white
    std::bitset<2048> display;
    // Point to current instruction in memory
    uint16_t program_counter{};
    // Registers V0 - VF
    uint8_t registers[16]{};
    // Stack for 16-bit addresses
    uint16_t stack[16]{};
    uint8_t sp{};
    // Index register
    uint16_t index{};
    // Delay timer decremented at 60Hz until it reaches 0
    uint8_t delay_timer{};
    // Sound timer that gives off beeping sound as long as it's not 0
    uint8_t sound_timer{};
} Chip8;


// For example, the character F is 0xF0, 0x80, 0xF0, 0x80, 0x80. Binary representation:
// 1111 0000
// 1 0000000
// 1111 0000
// 1 0000000
// 1 0000000
void LoadFontsIntoMemory(uint8_t *memory) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 5; j++) {
            memory[0x50 + (i * 5 + j)] = characters[i * 5 + j];
        }
    }
}

void LoadRomIntoMemory(uint8_t *memory, const std::string& rom_file) {
    std::ifstream rom  (rom_file, std::ios::binary);
    if (!rom.is_open()) {
        std::cout << "Failed to read file" << std::endl;
        exit(1);
    }
    rom.seekg(0, std::ios::end);
    const auto size = rom.tellg();
    rom.seekg(0, std::ios::beg);
    rom.read(reinterpret_cast<char *>(memory + 0x200), size);
    rom.close();
}


void Execute() {
    std::cout << "Executing..." << std::endl;
}

void Draw(const std::bitset<2048> & display, sf::RenderWindow & window) {
    // Loop through the display pixel 2D array and draw it to the SFML window (and scale it up)
    window.clear(sf::Color::Black);
    // 64 x 32 -> Rows: i = 0 to 32 (height), Columns: j = 0 to 64 (width)
    for (uint8_t i = 0; i < 32; i++) {
        for (uint8_t j = 0; j < 64; j++) {
            if (display[i * 64 + j]) {
                // Draw to hidden buffer
                sf::RectangleShape rectangle;
                rectangle.setSize(sf::Vector2f(SCALE, SCALE));
                rectangle.setFillColor(sf::Color::White);
                rectangle.setPosition(sf::Vector2f(j * SCALE, i * SCALE));
                window.draw(rectangle);
            }
        }
    }
    // Copy buffer to window (double-buffering)
    window.display();
}


void GameLoop(const uint8_t ups, Chip8 &chip8) {
    sf::RenderWindow window(sf::VideoMode({64 * SCALE, 32 * SCALE}), "Chip 8", sf::Style::Titlebar | sf::Style::Close);

    // By default, chrono::duration is in seconds
    // <double> -> representation
    const auto time_between_updates = std::chrono::duration<double>(1.0 / ups);
    while (window.isOpen()) {

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        // Fetch();
        // Decode();
        // Execute();
        Draw(chip8.display, window);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        while (elapsed_seconds < time_between_updates) {
            elapsed_seconds = std::chrono::steady_clock::now() - start;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <rom file> <updates per second>" << std::endl;
        exit(1);
    }

    const std::string rom_file = argv[1];
    const uint8_t ups = atoi(argv[2]); // 12-16 updates (fetch-decode-execute) per second

    Chip8 chip8;
    LoadFontsIntoMemory(chip8.memory);
    LoadRomIntoMemory(chip8.memory, rom_file);
    GameLoop(ups, chip8);
}
