#include <bitset>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>

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

void GameLoop(const uint8_t ups) {
    const float time_between_updates = 1.0 / ups;
    while (true) {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        // Fetch();
        // Decode();
        Execute();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_time = end - start;

        while (elapsed_time.count() < time_between_updates) {
            elapsed_time = std::chrono::steady_clock::now() - start;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <rom file> <updates per second>" << std::endl;
        exit(1);
    }

    std::string rom_file = argv[1];
    uint8_t ups = atoi(argv[2]); // 12-16

    Chip8 chip8;
    LoadFontsIntoMemory(chip8.memory);
    LoadRomIntoMemory(chip8.memory, rom_file);
    GameLoop(ups);
}
