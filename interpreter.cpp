#include "interpreter.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
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
    0xF0, 0x80, 0xF0, 0x80, 0x80 // F
};

// For example, the character F is 0xF0, 0x80, 0xF0, 0x80, 0x80. Binary representation:
// 1111 0000
// 1 0000000
// 1111 0000
// 1 0000000
// 1 0000000
void LoadFontsIntoMemory(Chip8 &chip8)
{
    for (uint8_t i = 0; i < 16; i++)
    {
        for (uint8_t j = 0; j < 5; j++)
        {
            chip8.memory[FONT_ADDRESS_START + (i * 5 + j)] = characters[i * 5 + j];
        }
    }
}

size_t LoadRomIntoMemory(Chip8 &chip8, const std::string &rom_file)
{
    std::ifstream rom(rom_file, std::ios::binary);
    if (!rom.is_open())
    {
        std::cout << "Failed to read file" << std::endl;
        exit(1);
    }
    rom.seekg(0, std::ios::end);
    const auto size = rom.tellg();
    rom.seekg(0, std::ios::beg);
    rom.read(reinterpret_cast<char *>(chip8.memory + ROM_ADDRESS_START), size);
    rom.close();

    // Set PC to first instruction
    chip8.programCounter = ROM_ADDRESS_START;

    return size;
}

void UnknownInstruction(const uint8_t byte1, const uint8_t byte2)
{
    const uint16_t fullInstruction = ((byte1) << 8) | byte2;
    std::cerr << "Unknown Instruction: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') <<
            fullInstruction << "\n";
    exit(1);
}

void FetchDecodeExecute(Chip8 &chip8, const Params &params, const double hz)
{
    // Fetch
    const uint8_t byte1 = chip8.memory[chip8.programCounter];
    const uint8_t byte2 = chip8.memory[chip8.programCounter + 1];
    chip8.programCounter += 2;

    // Decode
    // 00E0 (example) => 0 0 E 0 => 0000 0000 1110 0000
    // 0b1111 => 0000 0000 0000 1111 << 4 => 0000 0000 1111 0000 << 4 => 0000 1111 0000 0000 << 4 => 1111 0000 0000 0000
    // '0'0E0 & 0b1111 << (4 * 3) => 0 (first number)
    // These are 4 bits long only but smallest datatype is uint8_t
    const uint8_t byte1Half1 = (byte1 & (0b11110000)) >> 4;
    const uint8_t byte1Half2 = byte1 & (0b00001111);
    const uint8_t byte2Half1 = (byte2 & (0b11110000)) >> 4;
    const uint8_t byte2Half2 = byte2 & (0b00001111);

    // Execute
    // TODO: Decrement timers by hz
    switch (byte1Half1)
    {
        case 0:
            switch (byte2Half2)
            {
                // CLEAR SCREEN
                case 0:
                    chip8.display.reset();
                    break;
                // RETURN FROM SUBROUTINE
                case 0xE:
                    chip8.sp -= 1;
                    chip8.programCounter = chip8.stack[chip8.sp];
                    break;
                default:
                    UnknownInstruction(byte1, byte2);
            }
            break;
        // JUMP
        case 1:
        {
            const uint16_t jumpTo = (byte1Half2 << 8) | (byte2Half1 << 4) | (byte2Half2);
            chip8.programCounter = jumpTo;
            break;
        }
        // SUBROUTINE
        case 2:
        {
            const uint16_t jumpTo = (byte1Half2 << 8) | (byte2Half1 << 4) | (byte2Half2);
            chip8.stack[chip8.sp] = chip8.programCounter;
            chip8.sp++;
            chip8.programCounter = jumpTo;
            break;
        }
        case 3:
        {
            const uint8_t nn = (byte2Half1 << 4) | byte2Half2;
            if (chip8.registers[byte1Half2] == nn)
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 4:
        {
            const uint8_t nn = (byte2Half1 << 4) | byte2Half2;
            if (chip8.registers[byte1Half2] != nn)
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 5:
        {
            if (chip8.registers[byte1Half1] == chip8.registers[byte2Half1])
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 6:
            chip8.registers[byte1Half2] = (byte2Half1 << 4) | (byte2Half2);
            break;
        case 7:
            chip8.registers[byte1Half2] += (byte2Half1 << 4) | (byte2Half2);
            break;
        // ARITHMETIC / LOGICAL OPERATIONS
        case 8:
        {
            switch (byte2Half2)
            {
                case 0:
                    chip8.registers[byte1Half2] = chip8.registers[byte2Half1];
                    break;
                case 1:
                    chip8.registers[byte1Half2] |= chip8.registers[byte2Half1];
                    break;
                case 2:
                    chip8.registers[byte1Half2] &= chip8.registers[byte2Half1];
                    break;
                case 3:
                    chip8.registers[byte1Half2] ^= chip8.registers[byte2Half1];
                    break;
                case 4:
                {
                    const uint8_t originalXValue = chip8.registers[byte1Half2];
                    chip8.registers[byte1Half2] += chip8.registers[byte2Half1];
                    // Check for overflow reference: https://stackoverflow.com/questions/33948450/detecting-if-an-unsigned-integer-overflow-has-occurred-when-adding-two-numbers
                    chip8.registers[0xF] = chip8.registers[byte1Half2] < originalXValue ? 1 : 0;
                    break;
                }
                // VX - VY
                case 5:
                {
                    const uint8_t vx = chip8.registers[byte1Half2];
                    const uint8_t vy = chip8.registers[byte2Half1];
                    chip8.registers[0xF] = vx > vy ? 1 : 0;
                    chip8.registers[byte1Half2] = vx - vy;
                    break;
                }
                // RIGHT SHIFT 1 bit
                case 6:
                {
                    if (params.shift)
                    {
                        chip8.registers[byte1Half2] = chip8.registers[byte2Half1];
                    }

                    const uint8_t value = chip8.registers[byte1Half2];
                    chip8.registers[0xF] = value & 0b1;
                    chip8.registers[byte1Half2] = value >> 1;
                    break;
                }
                // LEFT SHIFT 1 bit
                case 0xE:
                {
                    if (params.shift)
                    {
                        chip8.registers[byte1Half2] = chip8.registers[byte2Half1];
                    }

                    const uint8_t value = chip8.registers[byte1Half2];
                    chip8.registers[0xF] = value & 0b10000000;
                    chip8.registers[byte1Half2] = value << 1;
                    break;
                }
                // VY - VX
                case 7:
                {
                    const uint8_t vx = chip8.registers[byte1Half2];
                    const uint8_t vy = chip8.registers[byte2Half1];
                    chip8.registers[0xF] = vy - vx ? 1 : 0;
                    chip8.registers[byte1Half2] = vy - vx;
                    break;
                }
                default: UnknownInstruction(byte1, byte2);
            }
        }
        case 9:
        {
            if (chip8.registers[byte1Half1] != chip8.registers[byte2Half1])
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 0xA:
            chip8.index = (byte1Half2 << 8) | (byte2Half1 << 4) | (byte2Half2);
            break;
        // JUMP WITH OFFSET
        case 0xB:
        {
            const uint16_t jumpTo = ((byte1Half2 << 8) | byte2Half1 << 4) | byte2Half2;
            const uint8_t offset = params.jumpWithOffset ? chip8.registers[byte1Half2] : 0;
            chip8.programCounter = jumpTo + offset;
            break;
        }
        // RANDOM
        case 0xC:
        {
            const uint8_t nn = (byte2Half1 << 4) | byte2Half2;
            std::mt19937 randomEngine(std::random_device{}());
            std::uniform_int_distribution<uint8_t> distribution(0, 255);
            uint8_t randomNumber = distribution(randomEngine);
            randomNumber &= nn;
            chip8.registers[byte1Half2] = randomNumber;
        }
        // DRAW
        case 0xD:
        {
            uint8_t x = chip8.registers[byte1Half2] & 63;
            uint8_t y = chip8.registers[byte2Half1] & 31;
            chip8.registers[0xF] = 0;
            const uint8_t n = byte2Half2;

            for (uint8_t i = 0; i < n; i++)
            {
                const uint8_t nthByteSpriteData = chip8.memory[chip8.index + i];
                for (uint8_t j = 0; j < 8; j++)
                {
                    const uint8_t currentPixel = (nthByteSpriteData & (0b1 << (7 - j))) >> (7 - j);
                    if (currentPixel == 1)
                    {
                        if (chip8.display.test(y * 64 + x))
                        {
                            chip8.display.reset(y * 64 + x);
                            chip8.registers[0xF] = 1;
                        }
                        else
                        {
                            chip8.display.set(y * 64 + x);
                        }
                    }
                    x++;
                    if (x >= 64)
                    {
                        break;
                    }
                }
                y++;
                if (y >= 32)
                {
                    break;
                }
                x = chip8.registers[byte1Half2] & 63;
            }
            break;
        }
        default:
            UnknownInstruction(byte1, byte2);
    }
}


void Draw(const std::bitset<2048> &display, sf::RenderWindow &window)
{
    // Loop through the display pixel 2D array and draw it to the SFML window (and scale it up)
    window.clear(sf::Color::Black);
    // 64 x 32 -> Rows: i = 0 to 32 (height), Columns: j = 0 to 64 (width)
    for (uint8_t i = 0; i < 32; i++)
    {
        for (uint8_t j = 0; j < 64; j++)
        {
            if (display[i * 64 + j])
            {
                // Draw to hidden buffer
                // Use a square of size SCALE to represent a pixel on the scaled up display
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


void InitializeLoopWithRendering(const uint8_t ups, Chip8 &chip8, const Params &params)
{
    sf::RenderWindow window(sf::VideoMode({64 * SCALE, 32 * SCALE}), "Chip 8", sf::Style::Titlebar | sf::Style::Close);

    // By default, chrono::duration is in seconds
    // <double> -> representation
    const auto timeBetweenUpdates = std::chrono::duration<double>(1.0 / ups);
    std::chrono::duration<double> deltaTime{};
    auto deltaStart = std::chrono::steady_clock::now();
    while (window.isOpen())
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<double>(start - deltaStart);
        deltaStart = std::chrono::steady_clock::now();
        // Timer is decremented at 60Hz
        const auto hz = 60 * deltaTime.count();

        FetchDecodeExecute(chip8, params, hz);
        Draw(chip8.display, window);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed_seconds = end - start;
        while (elapsed_seconds < timeBetweenUpdates)
        {
            elapsed_seconds = std::chrono::steady_clock::now() - start;
        }
    }
}
