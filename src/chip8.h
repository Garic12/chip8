#pragma once
#include <array>
#include <cstdint>
#include <string>

constexpr int DISP_W = 64, DISP_H = 32;

namespace chip8 {
struct Chip8 {
        std::array<uint8_t, 4096> MEM = {};
        std::array<uint8_t, 16> REG = {};
        std::array<uint16_t, 16> STACK = {};
        std::array<std::array<bool, 32>, 64> DISP{};
        std::array<bool, 16> KB = {};
        int16_t SP = 0;
        uint16_t PC = 0x200;
        uint16_t I = 0;
        uint8_t delay = 0, sound = 0;
        uint32_t rng = 12345;
};

void init(Chip8 &c);
bool load(Chip8 &c, std::string &path);
bool step(Chip8 &c);
void tick_timers(Chip8 &c);
void draw_debug(const Chip8 &c, float x, float y, float w, float h);
} // namespace chip8
