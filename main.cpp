#include <cstdint>
#include <iostream>
#include "interpreter.h"

// test roms: https://github.com/Timendus/chip8-test-suite

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <rom file> <updates per second>" << std::endl;
        exit(1);
    }

    const std::string rom_file = argv[1];
    const uint8_t ups = atoi(argv[2]); // 12-16 updates (fetch-decode-execute) per second

    Chip8 chip8;
    LoadFontsIntoMemory(chip8);
    LoadRomIntoMemory(chip8, rom_file);
    InitializeLoopWithRendering(ups, chip8);
}
