#include "chip8.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace chip8 {
void init(Chip8 &c) {
    c = Chip8{};
    std::ranges::copy(FONT, c.MEM.begin());
}

bool load(Chip8 &c, std::string &path) {
    FILE *file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }

    constexpr size_t MAX_ROM = 4096 - 0x200;
    fread(c.MEM.data() + 0x200, 1, MAX_ROM, file);
    bool error = ferror(file) != 0;
    fclose(file);

    return !error;
}

void tick_timers(Chip8 &c) {
    if (c.delay > 0) {
        --c.delay;
    }
    if (c.sound > 0) {
        --c.sound;
    }
}

bool step(Chip8 &c) {
    // Fetch
    bool increase_PC = true;
    uint16_t opcode = (c.MEM[c.PC & 0xFFF] << 8) | c.MEM[(c.PC + 1) & 0xFFF];

    // Decode
    int op = (opcode >> 12);
    int x = (opcode >> 8) & 0xF;
    int y = (opcode >> 4) & 0xF;
    int n = opcode & 0xF;
    int nn = opcode & 0xFF;
    int nnn = opcode & 0xFFF;

    switch (op) {
    case 0x0:
        switch (x) {
        case 0x0:
            if (y == 0xE && n == 0x0) {
                c.DISP = {};
            } else if (y == 0xE && n == 0xE) {
                // Return from subroutine to address pulled from stack
                if (c.SP <= 0) {
                    return false;
                }
                c.PC = c.STACK[--c.SP];
            } else {
                return false;
            }
            break;
        default:
            // Jump to native assembler subroutine at 0xNNN
            // This one we don't need for this kind of emulator
            break;
        }
        break;
    case 0x1:
        // jump to address NNN
        c.PC = nnn;
        increase_PC = false;
        break;
    case 0x2:
        // Calls subroutine at NNN
        if (c.SP >= 16) {
            return false;
        }
        c.STACK[c.SP++] = c.PC;
        c.PC = nnn;
        increase_PC = false;
        break;
    case 0x3:
        // skip next opcode if VX == NN
        if (c.REG[x] == nn) {
            c.PC += 2;
        }
        break;
    case 0x4:
        // skip next opcode if VX != NN
        if (c.REG[x] != nn) {
            c.PC += 2;
        }
        break;
    case 0x5:
        // skip next opcode if VX == VY
        if (c.REG[x] == c.REG[y]) {
            c.PC += 2;
        }
        break;
    case 0x6:
        // Set the register VX to the value NN
        c.REG[x] = nn;
        break;
    case 0x7:
        // add NN to VX
        c.REG[x] += nn;
        break;
    case 0x8:
        // Logical operations
        switch (n) {
        case 0:
            // SET
            c.REG[x] = c.REG[y];
            break;
        case 1:
            // OR
            c.REG[x] = c.REG[x] | c.REG[y];
            break;
        case 2:
            // AND
            c.REG[x] = c.REG[x] & c.REG[y];
            break;
        case 3:
            // XOR
            c.REG[x] = c.REG[x] ^ c.REG[y];
            break;
        case 4: {
            // Add
            uint8_t flag = (c.REG[x] + c.REG[y] > 255) ? 1 : 0;
            c.REG[x] = c.REG[x] + c.REG[y];
            c.REG[0xF] = flag;
            break;
        }
        case 5: {
            // Sub VX - VY
            uint8_t flag = (c.REG[x] >= c.REG[y]) ? 1 : 0;
            c.REG[x] = c.REG[x] - c.REG[y];
            c.REG[0xF] = flag;
            break;
        }
        case 6: {
            // Shift 1 bit right
            c.REG[x] = c.REG[y]; // This is a quirk
            uint8_t flag = c.REG[x] & 0x1;
            c.REG[x] >>= 1;
            c.REG[0xF] = flag;
        } break;
        case 0xE: {
            // Shift 1 bit left
            c.REG[x] = c.REG[y]; // This is a quirk
            uint8_t flag = c.REG[x] >> 7;
            c.REG[x] <<= 1;
            c.REG[0xF] = flag;
        } break;
        case 7: {
            // Sub VY - VX
            uint8_t flag = (c.REG[y] >= c.REG[x]) ? 1 : 0;
            c.REG[x] = c.REG[y] - c.REG[x];
            c.REG[0xF] = flag;
        } break;
        default:
            return false;
        }
        break;
    case 0x9:
        // skip next opcode if VX != VY
        if (c.REG[x] != c.REG[y]) {
            c.PC += 2;
        }
        break;
    case 0xA:
        // set c.I to NNN
        c.I = opcode & 0x0FFF;
        break;
    case 0xB:
        // Jump to address NNN + V0
        c.PC = nnn + c.REG[0];
        increase_PC = false;
        break;
    case 0xC:
        // Random xorshift32
        c.rng ^= c.rng << 13;
        c.rng ^= c.rng >> 17;
        c.rng ^= c.rng << 5;
        c.REG[x] = c.rng & nn;
        break;
    case 0xD: {
        // Draw to the screen an 8xN sprite located at memory c.I + row
        int N = opcode & 0x000F;
        int x0 = c.REG[x] % 64;
        int y0 = c.REG[y] % 32;

        c.REG[0xF] = 0; // Checker bit for on->off

        for (int row = 0; row < N; row++) {
            int y = y0 + row;
            if (y >= 32) {
                break;
            }

            uint8_t sprite = c.MEM[(c.I + row) & 0xFFF];

            for (int col = 0; col < 8; col++) {
                int x = x0 + col;
                if (x >= 64) {
                    break;
                }
                if ((sprite & (0x80 >> col)) != 0) {
                    if (c.DISP[x][y]) {
                        c.REG[0xF] = 1;
                    }
                    c.DISP[x][y] ^= 1;
                }
            }
        }
    } break;
    case 0xE:
        if (y == 9 && n == 0xE) {
            // Skip if key
            if (c.KB[c.REG[x]] == 1) {
                c.PC += 2;
            }
        } else if (y == 0xA && n == 1) {
            // Skip if not key
            if (!(c.KB[c.REG[x]] == 1)) {
                c.PC += 2;
            }
        }
        break;
    case 0xF:
        // Timers
        switch (nn) {
        case 0x07:
            c.REG[x] = c.delay;
            break;
        case 0x15:
            c.delay = c.REG[x];
            break;
        case 0x18:
            c.sound = c.REG[x];
            break;
        case 0x1E: {
            int o = c.I + c.REG[x];
            if (o > 0x1000) {
                c.REG[0xF] = 1;
            }
            c.I = o;
        } break;
        // Get key released
        case 0x0A: {
            bool key_released = false;
            for (int i = 0; i < 16; ++i) {
                if (c.KB[i] == 2) {
                    c.REG[x] = i;
                    key_released = true;
                }
            }
            if (!key_released) {
                c.PC -= 2;
            }
        } break;
        case 0x29:
            // Font Character
            c.I = c.REG[x] * 5;
            break;
        case 0x33:
            // Binary coded decimal conversion
            c.MEM[c.I & 0xFFF] = (c.REG[x] / 100) % 10;
            c.MEM[(c.I + 1) & 0xFFF] = (c.REG[x] / 10) % 10;
            c.MEM[(c.I + 2) & 0xFFF] = c.REG[x] % 10;
            break;
        case 0x55:
            // Store memory
            for (int i = 0; i <= x; ++i) {
                c.MEM[(c.I + i) & 0xFFF] = c.REG[i];
            }
            break;
        case 0x65:
            // Read memory
            for (int i = 0; i <= x; ++i) {
                c.REG[i] = c.MEM[(c.I + i) & 0xFFF];
            }
            break;
        default:
            return false;
        }
        break;
    default:
        return false;
    }

    if (increase_PC) {
        c.PC += 2;
    }

    return true;
}
} // namespace chip8
