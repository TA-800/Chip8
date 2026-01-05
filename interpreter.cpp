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

void FetchDecodeExecute(Chip8 &chip8, const std::bitset<16> &keypad, const Params &params, const double hz)
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
    if (chip8.delayTimer > 0) { chip8.delayTimer -= hz; }
    if (chip8.soundTimer > 0) { chip8.soundTimer -= hz; }
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
                    chip8.sp--;
                    chip8.programCounter = chip8.stack[chip8.sp];
                    break;
                default:
                    UnknownInstruction(byte1, byte2);
            }
            break;
        // JUMP
        case 1:
        {
            const uint16_t jumpTo = (byte1Half2 << 8) | byte2;
            chip8.programCounter = jumpTo;
            break;
        }
        // SUBROUTINE
        case 2:
        {
            const uint16_t jumpTo = (byte1Half2 << 8) | byte2;
            chip8.stack[chip8.sp] = chip8.programCounter;
            chip8.sp++;
            chip8.programCounter = jumpTo;
            break;
        }
        case 3:
        {
            const uint8_t nn = byte2;
            if (chip8.registers[byte1Half2] == nn)
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 4:
        {
            const uint8_t nn = byte2;
            if (chip8.registers[byte1Half2] != nn)
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 5:
        {
            if (chip8.registers[byte1Half2] == chip8.registers[byte2Half1])
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 6:
            chip8.registers[byte1Half2] = byte2;
            break;
        case 7:
            chip8.registers[byte1Half2] += byte2;
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
                    // Check for overflow reference: https://stackoverflow.com/questions/33948450/detecting-if-an-unsigned-integer-overflow-has-occurred-when-adding-two-numbers
                    const uint8_t vx = chip8.registers[byte1Half2];
                    const uint8_t vy = chip8.registers[byte2Half1];
                    const uint8_t sum = vx + vy;
                    chip8.registers[byte1Half2] = sum;
                    chip8.registers[0xF] = sum < vx ? 1 : 0;
                    break;
                }
                // VX - VY
                case 5:
                {
                    const uint8_t vx = chip8.registers[byte1Half2];
                    const uint8_t vy = chip8.registers[byte2Half1];
                    chip8.registers[byte1Half2] = vx - vy;
                    chip8.registers[0xF] = vx >= vy ? 1 : 0;
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
                    chip8.registers[byte1Half2] = value >> 1;
                    chip8.registers[0xF] = value & 0b1;
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
                    chip8.registers[byte1Half2] = value << 1;
                    chip8.registers[0xF] = (value & 0b10000000) >> 7;
                    break;
                }
                // VY - VX
                case 7:
                {
                    const uint8_t vx = chip8.registers[byte1Half2];
                    const uint8_t vy = chip8.registers[byte2Half1];
                    chip8.registers[byte1Half2] = vy - vx;
                    chip8.registers[0xF] = vy >= vx ? 1 : 0;
                    break;
                }
                default: UnknownInstruction(byte1, byte2);
            }
            break;
        }
        case 9:
        {
            if (chip8.registers[byte1Half2] != chip8.registers[byte2Half1])
            {
                chip8.programCounter += 2;
            }
            break;
        }
        case 0xA:
            chip8.index = (byte1Half2 << 8) | byte2;
            break;
        // JUMP WITH OFFSET
        case 0xB:
        {
            const uint16_t jumpTo = byte1Half2 << 8 | byte2;
            const uint8_t offset = params.jumpWithOffset ? chip8.registers[byte1Half2] : 0;
            chip8.programCounter = jumpTo + offset;
            break;
        }
        // RANDOM
        case 0xC:
        {
            const uint8_t nn = byte2;
            std::mt19937 randomEngine(std::random_device{}());
            std::uniform_int_distribution<uint8_t> distribution(0, 255);
            uint8_t randomNumber = distribution(randomEngine);
            randomNumber &= nn;
            chip8.registers[byte1Half2] = randomNumber;
            break;
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
        // SKIP IF KEY
        case 0xE:
            switch (byte2Half1)
            {
                case 0x9:
                {
                    const uint8_t key = chip8.registers[byte1Half2];
                    if (keypad.test(key))
                    {
                        chip8.programCounter += 2;
                    }
                    break;
                }
                case 0xA:
                {
                    const uint8_t key = chip8.registers[byte1Half2];
                    if (!keypad.test(key))
                    {
                        chip8.programCounter += 2;
                    }
                    break;
                }
                default:
                    UnknownInstruction(byte1, byte2);
            }
            break;
        case 0xF:
            switch (byte2)
            {
                // TIMERS
                case 0x07:
                    chip8.registers[byte1Half2] = chip8.delayTimer;
                    break;
                case 0x15:
                    chip8.delayTimer = chip8.registers[byte1Half2];
                    break;
                case 0x18:
                    chip8.soundTimer = chip8.registers[byte1Half2];
                    break;
                // ADD TO INDEX
                case 0x1E:
                {
                    const uint8_t originalIndexValue = chip8.index;
                    chip8.index += chip8.registers[byte1Half2];
                    // Check for overflow, set VF
                    if (chip8.index < originalIndexValue)
                    {
                        chip8.registers[0xF] = 1;
                    }
                    break;
                }
                // GET KEY (block for input)
                case 0x0A:
                    // Set the PC to this same instruction, this will cause it to loop over and over again
                    chip8.programCounter -= 2;

                    // Resume execution if any key is pressed
                    if (keypad.any())
                    {
                        chip8.registers[byte1Half2] = keypad._Find_first();
                    }
                    break;
                // FONT CHARACTER
                case 0x29:
                {
                    const uint8_t hexChar = chip8.registers[byte1Half2];
                    // Each character from 0 to F is stored in order starting at FONT_ADDRESS_START,
                    // and each is 5 bytes long
                    chip8.index = FONT_ADDRESS_START + (hexChar * 5);
                    break;
                }
                // BINARY-CODED DECIMAL CONVERSION
                case 0x33:
                {
                    const uint8_t number = chip8.registers[byte1Half2];
                    const uint8_t hundreds = number / 100;
                    const uint8_t tens = (number % 100) / 10;
                    const uint8_t ones = number % 10;
                    chip8.memory[chip8.index] = hundreds;
                    chip8.memory[chip8.index + 1] = tens;
                    chip8.memory[chip8.index + 2] = ones;
                    break;
                }
                // STORE AND LOAD MEM
                case 0x55:
                {
                    uint8_t i;
                    for (i = 0; i <= byte1Half2; i++)
                    {
                        chip8.memory[chip8.index + i] = chip8.registers[i];
                    }
                    chip8.index += params.storeIncrementIndex ? i : 0;
                    break;
                }
                case 0x65:
                {
                    uint8_t i;
                    for (i = 0; i <= byte1Half2; i++)
                    {
                        chip8.registers[i] = chip8.memory[chip8.index + i];
                    }
                    chip8.index += params.loadIncrementIndex ? i : 0;
                    break;
                }
                default: UnknownInstruction(byte1, byte2);
            }
            break;

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

    const auto onClose = [&window](const sf::Event::Closed &)
    {
        window.close();
    };

    // Use a bool array to keep track of keys being pressed
    std::bitset<16> keypad{};
    const auto onKeyPress = [&keypad](const sf::Event::KeyPressed &event)
    {
        if (event.scancode == sf::Keyboard::Scan::Num1)
        {
            keypad.set(1);
        }
        if (event.scancode == sf::Keyboard::Scan::Num2)
        {
            keypad.set(2);
        }
        if (event.scancode == sf::Keyboard::Scan::Num3)
        {
            keypad.set(3);
        }
        if (event.scancode == sf::Keyboard::Scan::Num4)
        {
            keypad.set(0xC);
        }

        if (event.scancode == sf::Keyboard::Scan::Q)
        {
            keypad.set(4);
        }
        if (event.scancode == sf::Keyboard::Scan::W)
        {
            keypad.set(5);
        }
        if (event.scancode == sf::Keyboard::Scan::E)
        {
            keypad.set(6);
        }
        if (event.scancode == sf::Keyboard::Scan::R)
        {
            keypad.set(0xD);
        }

        if (event.scancode == sf::Keyboard::Scan::A)
        {
            keypad.set(7);
        }
        if (event.scancode == sf::Keyboard::Scan::S)
        {
            keypad.set(8);
        }
        if (event.scancode == sf::Keyboard::Scan::D)
        {
            keypad.set(9);
        }
        if (event.scancode == sf::Keyboard::Scan::F)
        {
            keypad.set(0xE);
        }


        if (event.scancode == sf::Keyboard::Scan::Z)
        {
            keypad.set(0xA);
        }
        if (event.scancode == sf::Keyboard::Scan::X)
        {
            keypad.set(0);
        }
        if (event.scancode == sf::Keyboard::Scan::C)
        {
            keypad.set(0xB);
        }
        if (event.scancode == sf::Keyboard::Scan::V)
        {
            keypad.set(0xF);
        }
    };
    const auto onKeyRelease = [&keypad](const sf::Event::KeyReleased &event)
    {
        if (event.scancode == sf::Keyboard::Scan::Num1)
        {
            keypad.reset(1);
        }
        if (event.scancode == sf::Keyboard::Scan::Num2)
        {
            keypad.reset(2);
        }
        if (event.scancode == sf::Keyboard::Scan::Num3)
        {
            keypad.reset(3);
        }
        if (event.scancode == sf::Keyboard::Scan::Num4)
        {
            keypad.reset(0xC);
        }

        if (event.scancode == sf::Keyboard::Scan::Q)
        {
            keypad.reset(4);
        }
        if (event.scancode == sf::Keyboard::Scan::W)
        {
            keypad.reset(5);
        }
        if (event.scancode == sf::Keyboard::Scan::E)
        {
            keypad.reset(6);
        }
        if (event.scancode == sf::Keyboard::Scan::R)
        {
            keypad.reset(0xD);
        }

        if (event.scancode == sf::Keyboard::Scan::A)
        {
            keypad.reset(7);
        }
        if (event.scancode == sf::Keyboard::Scan::S)
        {
            keypad.reset(8);
        }
        if (event.scancode == sf::Keyboard::Scan::D)
        {
            keypad.reset(9);
        }
        if (event.scancode == sf::Keyboard::Scan::F)
        {
            keypad.reset(0xE);
        }


        if (event.scancode == sf::Keyboard::Scan::Z)
        {
            keypad.reset(0xA);
        }
        if (event.scancode == sf::Keyboard::Scan::X)
        {
            keypad.reset(0);
        }
        if (event.scancode == sf::Keyboard::Scan::C)
        {
            keypad.reset(0xB);
        }
        if (event.scancode == sf::Keyboard::Scan::V)
        {
            keypad.reset(0xF);
        }
    };

    // By default, chrono::duration is in seconds
    // <double> -> representation
    const auto timeBetweenUpdates = std::chrono::duration<double>(1.0 / ups);
    auto last = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration<double>(0);
    while (window.isOpen())
    {
        window.handleEvents(onClose, onKeyPress, onKeyRelease);
        // Control speed of instruction execution
        if (deltaTime < timeBetweenUpdates)
        {
            auto now = std::chrono::steady_clock::now();
            deltaTime += now - last;
            last = now;
            continue;
        }
        // Timer is decremented at 60Hz
        const auto hz = 60 * deltaTime.count();

        FetchDecodeExecute(chip8, keypad, params, hz);
        Draw(chip8.display, window);

        deltaTime = std::chrono::duration<double>(0);
    }
}
