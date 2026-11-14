#include "chip8.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <imgui.h>
#include <rlImGui.h>

constexpr std::array<uint8_t, 80> FONT = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

// Index = CHIP-8 key (0x0–0xF), value = raylib key
constexpr std::array<int, 16> KEYMAP = {
    KEY_X,     // 0
    KEY_ONE,   // 1
    KEY_TWO,   // 2
    KEY_THREE, // 3
    KEY_Q,     // 4
    KEY_W,     // 5
    KEY_E,     // 6
    KEY_A,     // 7
    KEY_S,     // 8
    KEY_D,     // 9
    KEY_Z,     // A
    KEY_C,     // B
    KEY_FOUR,  // C
    KEY_R,     // D
    KEY_F,     // E
    KEY_V,     // F
};

namespace chip8 {
void draw_debug(const Chip8 &c, float x, float y, float w, float h) {
    // The instruction about to execute (PC hasn't advanced past it yet)
    const uint16_t opcode =
        (c.MEM[c.PC & 0xFFF] << 8) | c.MEM[(c.PC + 1) & 0xFFF];

    rlImGuiBegin();
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("Debug", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("PC: 0x%03X", c.PC);
    ImGui::Text("I:  0x%03X", c.I);
    ImGui::Text("SP: %d", c.SP);
    ImGui::Text("Timers: delay %u  sound %u", c.delay, c.sound);
    ImGui::Text("Next op: 0x%04X", opcode);
    ImGui::Text("Nibbles: %X %X %X %X", (opcode >> 12) & 0xF,
                (opcode >> 8) & 0xF, (opcode >> 4) & 0xF, opcode & 0xF);

    // ---- Registers ----
    ImGui::SeparatorText("Registers");
    // 4x4 grid: V0-V3 / V4-V7 / V8-VB / VC-VF
    if (ImGui::BeginTable("RegisterTable", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchSame)) {
        for (int reg = 0; reg < 16; ++reg) {
            ImGui::TableNextColumn();
            ImGui::Text("V%X: 0x%02X", reg, c.REG[reg] & 0xF);
        }
        ImGui::EndTable();
    }

    // ---- Memory around PC ----
    // 7 instructions (2 bytes each): 3 before PC, PC, 3 after PC
    ImGui::SeparatorText("Memory around PC");
    if (ImGui::BeginTable("MemoryTable", 2,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Address");
        ImGui::TableSetupColumn("Opcode");
        ImGui::TableHeadersRow();
        for (int offset = -3; offset <= 3; ++offset) {
            const int addr = c.PC + (offset * 2);
            ImGui::TableNextRow();
            if (offset == 0) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                       IM_COL32(90, 70, 20, 255));
            }
            ImGui::TableNextColumn();
            if (addr < 0 || addr + 1 >= 4096) {
                ImGui::TextDisabled("---");
                ImGui::TableNextColumn();
                ImGui::TextDisabled("----");
                continue;
            }
            ImGui::Text("%s0x%03X", offset == 0 ? "> " : "  ", addr);
            ImGui::TableNextColumn();
            ImGui::Text("0x%02X%02X", c.MEM[addr], c.MEM[addr + 1]);
        }
        ImGui::EndTable();
    }

    // ---- Stack ----
    // Slots [0, SP) are in use; SP is the next free slot
    ImGui::SeparatorText("Stack");
    const int depth = c.SP < 16 ? c.SP : 16; // don't trust a corrupted SP
    if (depth == 0) {
        ImGui::TextDisabled("(empty)");
    } else if (ImGui::BeginTable("StackTable", 2,
                                 ImGuiTableFlags_Borders |
                                     ImGuiTableFlags_RowBg |
                                     ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Level");
        ImGui::TableSetupColumn("Call Addr");
        ImGui::TableHeadersRow();
        // Listed top (most recent call) to bottom
        for (int level = depth - 1; level >= 0; --level) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", level);
            ImGui::TableNextColumn();
            ImGui::Text("0x%03X", c.STACK[level]);
        }
        ImGui::EndTable();
    }

    ImGui::End();
    rlImGuiEnd();
}

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

void get_input(Chip8 &c) {
    for (int i = 0; i < 16; ++i) {
        c.KB[i] = IsKeyDown(KEYMAP[i]);
    }
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
    get_input(c);
    //
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
                    printf("Stack underflow: 00EE with empty stack at 0x%03X\n",
                           c.PC);
                    return false;
                }
                c.PC = c.STACK[--c.SP];
            } else {
                printf("Unsupported Opcode: 0x%04X\n", opcode);
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
            printf("Stack overflow: 2NNN with full stack at 0x%03X\n", c.PC);
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
            if (c.KB[c.REG[x]]) {
                c.PC += 2;
            }
        } else if (y == 0xA && n == 1) {
            // Skip if not key
            if (!c.KB[c.REG[x]]) {
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
                if (IsKeyReleased(KEYMAP[i])) {
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
            printf("Unsupported Opcode: 0x%04X\n", opcode);
            return false;
        }
        break;
    default:
        printf("Unsupported Opcode: 0x%04X\n", opcode);
        return false;
    }

    if (increase_PC) {
        c.PC += 2;
    }

    return true;
}
} // namespace chip8
