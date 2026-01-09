#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <bitset>
#include <cstdint>

constexpr uint8_t SCALE = 10;
constexpr uint8_t FONT_ADDRESS_START = 0x50;
constexpr uint16_t ROM_ADDRESS_START = 0x200;

typedef struct hardware
{
    // Order from smallest to biggest for alignment
    uint8_t memory[4096]{};
    // 64 x 32 pixels black or white
    std::bitset<2048> display;
    // Stack for 16-bit addresses
    uint16_t stack[16]{};
    // Registers V0 - VF
    uint8_t registers[16]{};
    // Delay timer decremented at 60Hz until it reaches 0
    double delayTimer{};
    // Sound timer that gives off beeping sound as long as it's not 0
    double soundTimer{};
    // Point to current instruction in memory
    uint16_t programCounter{};
    // Index register
    uint16_t index{};
    // Stack pointer
    uint8_t sp{};
    // Boolean that tracks keypress state for the FX0A Get-Key instruction
    bool beginKeyPress = false;
} Chip8;

// Configuration parameters for execution if any
typedef struct params
{
    bool shift = false;
    bool jumpWithOffset = false;
    bool storeIncrementIndex = false;
    bool loadIncrementIndex = false;
} Params;

void LoadFontsIntoMemory(Chip8 &chip8);

size_t LoadRomIntoMemory(Chip8 &chip8, const std::string &rom_file);

void InitializeLoopWithRendering(uint8_t ups, Chip8 &chip8, const Params &params);

#endif //INTERPRETER_H
