#pragma once
#include <cstdint>
#include <raylib.h>

constexpr int DISP_W = 64, DISP_H = 32;

struct Chip8 {
        uint8_t MEM[4096];
        uint8_t REG[16];
        uint16_t STACK[16];
        int16_t SP;
        uint16_t PC;
        uint16_t I;
        uint8_t delay, sound;
        bool DISP[64][32];
        bool KB[16];
        uint32_t rng;
        Sound beep;
};

void chip8_init(Chip8 &c);
bool chip8_load(Chip8 &c, const char *path);
bool chip8_step(Chip8 &c);
void chip8_tick_timers(Chip8 &c);
void chip8_draw_debug(const Chip8 &c, float x, float y, float w, float h);
