#include <cstdint>
#include <iostream>
#include "interpreter.h"

// test roms: https://github.com/Timendus/chip8-test-suite

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        // SHIFT: Set VX to the value of VY when shifting.
        std::cout << "Usage: " << argv[0] << " <rom file> <updates per second> [-shift] -[jumpWithOffset] " <<
                std::endl;
        exit(1);
    }

    const std::string rom_file = argv[1];
    const uint8_t ups = atoi(argv[2]); // 12-16 updates (fetch-decode-execute) per second
    if (argc >= 4)
    {
        // TODO: if -shift is in args then params.shift = true
        // TODO: if -jumpWithOffset is in args then params.jumpWithOffset = true
    }

    Chip8 chip8;
    LoadFontsIntoMemory(chip8);
    LoadRomIntoMemory(chip8, rom_file);
    Params params = {
        true,
        false
    };
    InitializeLoopWithRendering(ups, chip8, params);
}
