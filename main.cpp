#include <cstdint>
#include <iostream>

#include "interpreter.h"

// test roms: https://github.com/Timendus/chip8-test-suite

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        // SHIFT: Set VX to the value of VY when shifting.
        std::cout << "Usage: " << argv[0] <<
                " <rom file> <updates per second> [-shift] -[jumpWithOffset] [-loadIncrementIndex] [-storeIncrementIndex]"
                << std::endl;
        exit(1);
    }

    std::string argvString;
    for (int i = 1; i < argc; ++i)
    {
        argvString += argv[i];
        argvString += " ";
    }

    Params params{};
    if (argc >= 4)
    {
        if (argvString.find("-shift") != std::string::npos)
        {
            params.shift = true;
        }

        if (argvString.find("-jumpWithOffset") != std::string::npos)
        {
            params.jumpWithOffset = true;
        }

        if (argvString.find("-loadIncrementIndex") != std::string::npos)
        {
            params.loadIncrementIndex = true;
        }

        if (argvString.find("-storeIncrementIndex") != std::string::npos)
        {
            params.storeIncrementIndex = true;
        }

        if (argvString.find("-resetFlagOnBitOperations") != std::string::npos)
        {
            params.resetFlagOnBitOperations = true;
        }
    }

    const std::string rom_file = argv[1];
    const uint8_t ups = atoi(argv[2]);

    Chip8 chip8;
    LoadFontsIntoMemory(chip8);
    LoadRomIntoMemory(chip8, rom_file);
    InitializeLoopWithRendering(ups, chip8, params);
}
