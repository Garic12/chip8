#include <cstdint>
#include <cstdlib>
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>
#include <stack>
#include <stdio.h>

constexpr int PIXELSIZE = 16;
constexpr int DEBUG = 0;
constexpr int DEBUG_PANEL_WIDTH = 320; // extra window width for the ImGui panel

// Index = CHIP-8 key (0x0–0xF), value = raylib key
constexpr int KEYMAP[16] = {
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

constexpr uint8_t font[80] = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

int load_rom(const char *path, uint8_t *MEM) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        return 1;
    }

    size_t bytes_read = fread(MEM, 1, 4096 - 0x200, file);
    int error = ferror(file);
    fclose(file);

    if (error) {
        return 1;
    }

    printf("Loaded %zu bytes\n", bytes_read);
    return 0;
}

void draw_debug_ui(const uint8_t *MEM, const __uint8_t *PC, __uint16_t opcode,
                   uint16_t I, const __uint8_t *REG,
                   std::stack<uint8_t *> stack) {
    const int pc_addr = static_cast<int>(PC - MEM);

    rlImGuiBegin();
    // Pin the debug window to the panel right of the Chip-8 display
    ImGui::SetNextWindowPos(ImVec2(64 * PIXELSIZE, 0));
    ImGui::SetNextWindowSize(ImVec2(DEBUG_PANEL_WIDTH, 32 * PIXELSIZE));
    ImGui::Begin("Debug", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("PC: 0x%03X", pc_addr);
    ImGui::Text("IR: 0x%04X", I);
    ImGui::Text("Op-Code: 0x%04X", opcode);
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
            ImGui::Text("V%X: 0x%02X", reg,
                        static_cast<unsigned int>(REG[reg]));
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
            const int addr = pc_addr + offset * 2;
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
            ImGui::Text("0x%02X%02X", MEM[addr], MEM[addr + 1]);
        }
        ImGui::EndTable();
    }

    // ---- Stack ----
    // `stack` is a copy, so popping here doesn't touch the emulator's stack
    ImGui::SeparatorText("Stack");
    if (stack.empty()) {
        ImGui::TextDisabled("(empty)");
    } else if (ImGui::BeginTable("StackTable", 2,
                                 ImGuiTableFlags_Borders |
                                     ImGuiTableFlags_RowBg |
                                     ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Level");
        ImGui::TableSetupColumn("Return Addr");
        ImGui::TableHeadersRow();
        // Listed top (most recent call) to bottom
        for (int level = static_cast<int>(stack.size()) - 1; !stack.empty();
             --level, stack.pop()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", level);
            ImGui::TableNextColumn();
            ImGui::Text("0x%03X", static_cast<int>(stack.top() - MEM));
        }
        ImGui::EndTable();
    }

    ImGui::End();
    rlImGuiEnd();
}

void GatherInput(bool *KB) {
    for (int i = 0; i < 16; ++i)
        KB[i] = IsKeyDown(KEYMAP[i]);
}

int main() {
    // ---- Setup Chip-8 ----
    // reserve memory
    uint8_t MEM[4096] = {};

    std::stack<uint8_t *> STACK; // Saves PC adresses
    uint8_t REG[16] = {};

    // Starting addresses from 000 to 1FF are empty
    uint8_t *PC = MEM + 0x200;
    uint16_t I = 0;

    uint16_t opcode = 0;

    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    uint8_t nibble_first = 0x0;
    uint8_t nibble_second = 0x0;
    uint8_t nibble_third = 0x0;
    uint8_t nibble_fourth = 0x0;

    // Display and Keyboard input
    bool DISP[64][32] = {};
    bool KB[16] = {false};

    // Load font to memory
    memcpy(&MEM, font, sizeof(font));

    // ---- Load ROM ----
    const char *path = "./ressources/particle.ch8";

    if (load_rom(path, PC) == 1) {
        printf("Error reading ROM...\n");
        return 1;
    }

    // ---- Setup Raylib ----
    float screenWidth = 64 * PIXELSIZE + (DEBUG ? DEBUG_PANEL_WIDTH : 0);
    float screenHeight = 32 * PIXELSIZE;
    InitWindow(screenWidth, screenHeight, "Chip-8 Emulator");
    SetTargetFPS(60);
    rlImGuiSetup(true);

    InitAudioDevice();
    static short beep_samples[44100];
    for (int i = 0; i < 44100; ++i)
        beep_samples[i] =
            (i % 100 < 50) ? 3000 : -3000; // 100 samples per cycle
    Wave beep_wave = {44100, 44100, 16, 1, beep_samples};
    Sound beep = LoadSoundFromWave(beep_wave);

    const int IPF = 700 / 60; // Instructions per frame

    while (!WindowShouldClose()) {
        int steps = 0;

        if (DEBUG) {
            steps = IsKeyPressed(KEY_SPACE) ? 1 : 0;
        } else {
            steps = IPF;
        }

        for (int i = 0; i < steps; ++i) {
            GatherInput(KB);

            // Fetch
            bool increase_PC = true;
            opcode = (PC[0] << 8) | PC[1];

            // Decode
            nibble_first = (opcode >> 12);
            nibble_second = (opcode >> 8) & 0x0F;
            nibble_third = (opcode >> 4) & 0x00F;
            nibble_fourth = opcode & 0x000F;

            switch (nibble_first) {
            case 0x0:
                switch (nibble_second) {
                case 0x0:
                    if (nibble_third == 0xE && nibble_fourth == 0x0) {
                        memset(DISP, 0, sizeof(DISP));
                    } else if (nibble_third == 0xE && nibble_fourth == 0xE) {
                        // Return from subroutine to address pulled from stack
                        PC = STACK.top();
                        STACK.pop();
                    } else {
                        printf("Unsupported Opcode: 0x%04X", opcode);
                        return 1;
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
                PC = MEM + (opcode & 0x0FFF);
                increase_PC = false;
                break;
            case 0x2:
                // Calls subroutine at NNN
                STACK.push(PC);
                PC = MEM + (opcode & 0x0FFF);
                increase_PC = false;
                break;
            case 0x3:
                // skip next opcode if VX == NN
                if (REG[nibble_second] == (nibble_third << 4) + nibble_fourth)
                    PC += 2;
                break;
            case 0x4:
                // skip next opcode if VX != NN
                if (REG[nibble_second] != (nibble_third << 4) + nibble_fourth)
                    PC += 2;
                break;
            case 0x5:
                // skip next opcode if VX == VY
                if (REG[nibble_second] == REG[nibble_third])
                    PC += 2;
                break;
            case 0x6:
                // Set the register VX to the value NN
                *(REG + nibble_second) = (nibble_third << 4) + nibble_fourth;
                break;
            case 0x7:
                // add NN to VX
                *(REG + nibble_second) += (nibble_third << 4) + nibble_fourth;
                break;
            case 0x8:
                // Logical operations
                switch (nibble_fourth) {
                case 0:
                    // SET
                    REG[nibble_second] = REG[nibble_third];
                    break;
                case 1:
                    // OR
                    REG[nibble_second] = REG[nibble_second] | REG[nibble_third];
                    break;
                case 2:
                    // AND
                    REG[nibble_second] = REG[nibble_second] & REG[nibble_third];
                    break;
                case 3:
                    // XOR
                    REG[nibble_second] = REG[nibble_second] ^ REG[nibble_third];
                    break;
                case 4: {
                    // Add
                    uint8_t flag =
                        (REG[nibble_second] + REG[nibble_third] > 255) ? 1 : 0;
                    REG[nibble_second] = REG[nibble_second] + REG[nibble_third];
                    REG[0xF] = flag;
                    break;
                }
                case 5: {
                    // Sub VX - VY
                    uint8_t flag =
                        (REG[nibble_second] >= REG[nibble_third]) ? 1 : 0;
                    REG[nibble_second] = REG[nibble_second] - REG[nibble_third];
                    REG[0xF] = flag;
                    break;
                }
                case 6: {
                    // Shift 1 bit right
                    REG[nibble_second] = REG[nibble_third]; // This is a quirk
                    uint8_t flag = REG[nibble_second] & 0x1;
                    REG[nibble_second] >>= 1;
                    REG[0xF] = flag;
                } break;
                case 0xE: {
                    // Shift 1 bit left
                    REG[nibble_second] = REG[nibble_third]; // This is a quirk
                    uint8_t flag = REG[nibble_second] >> 7;
                    REG[nibble_second] <<= 1;
                    REG[0xF] = flag;
                } break;
                case 7: {
                    // Sub VY - VX
                    uint8_t flag =
                        (REG[nibble_third] >= REG[nibble_second]) ? 1 : 0;
                    REG[nibble_second] = REG[nibble_third] - REG[nibble_second];
                    REG[0xF] = flag;
                    break;
                }
                }
                break;
            case 0x9:
                // skip next opcode if VX != VY
                if (REG[nibble_second] != REG[nibble_third])
                    PC += 2;
                break;
            case 0xA:
                // set I to NNN
                I = opcode & 0x0FFF;
                break;
            case 0xB:
                // Jump to address NNN + V0
                PC = MEM + (opcode & 0x0FFF) + REG[0];
                increase_PC = false;
                break;
            case 0xC:
                // Random
                REG[nibble_second] =
                    (uint8_t)rand() & ((nibble_third << 4) + nibble_fourth);
                break;
            case 0xD: {
                // Draw to the screen an 8xN sprite located at memory I + row
                uint8_t N = opcode & 0x000F;
                uint8_t x0 = REG[nibble_second] % 64;
                uint8_t y0 = REG[nibble_third] % 32;

                REG[0xF] = 0; // Checker bit for on->off

                for (int row = 0; row < N; row++) {
                    int y = y0 + row;
                    if (y >= 32)
                        break;

                    uint8_t sprite = MEM[I + row];

                    for (int col = 0; col < 8; col++) {
                        int x = x0 + col;
                        if (x >= 64)
                            break;
                        if (sprite & (0x80 >> col)) {
                            if (DISP[x][y])
                                REG[0xF] = 1;
                            DISP[x][y] ^= true;
                        }
                    }
                }
            } break;
            case 0xE:
                if (nibble_third == 9 && nibble_fourth == 0xE) {
                    // Skip if key
                    if (KB[REG[nibble_second]])
                        PC += 2;
                } else if (nibble_third == 0xA && nibble_fourth == 1) {
                    // Skip if not key
                    if (!KB[REG[nibble_second]])
                        PC += 2;
                }
                break;
            case 0xF:
                // Timers
                uint8_t NN = (nibble_third << 4) + nibble_fourth;
                switch (NN) {
                case 0x07:
                    REG[nibble_second] = delay_timer;
                    break;
                case 0x15:
                    delay_timer = REG[nibble_second];
                    break;
                case 0x18:
                    sound_timer = REG[nibble_second];
                    break;
                case 0x1E: {
                    int o = I + REG[nibble_second];
                    if (o > 0x1000)
                        REG[0xF] = 1;
                    I = o;
                } break;
                // Get key released
                case 0x0A: {
                    bool key_released = false;
                    for (int i = 0; i < 16; ++i) {
                        if (IsKeyReleased(KEYMAP[i])) {
                            REG[nibble_second] = i;
                            key_released = true;
                        }
                    }
                    if (!key_released)
                        PC -= 2;
                } break;
                case 0x29:
                    // Font Character
                    I = REG[nibble_second] * 5;
                    break;
                case 0x33:
                    // Binary coded decimal conversion
                    MEM[I] = (REG[nibble_second] / 100) % 10;
                    MEM[I + 1] = (REG[nibble_second] / 10) % 10;
                    MEM[I + 2] = REG[nibble_second] % 10;
                    break;
                case 0x55:
                    // Store memory
                    for (int i = 0; i <= nibble_second; ++i) {
                        MEM[I + i] = REG[i];
                    }
                    break;
                case 0x65:
                    // Read memory
                    for (int i = 0; i <= nibble_second; ++i) {
                        REG[i] = MEM[I + i];
                    }
                    break;
                }
                break;
            }

            if (increase_PC)
                PC += 2;
        }

        // each frame, next to your timer decrement
        if (sound_timer > 0) {
            if (!IsSoundPlaying(beep))
                PlaySound(beep);
        } else if (IsSoundPlaying(beep)) {
            StopSound(beep);
        }

        if (delay_timer > 0)
            --delay_timer;
        if (sound_timer > 0)
            --sound_timer;
        BeginDrawing();
        ClearBackground({24, 26, 40, 255});

        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 32; y++) {
                if (DISP[x][y])
                    DrawRectangle(x * PIXELSIZE, y * PIXELSIZE, PIXELSIZE,
                                  PIXELSIZE, WHITE);
            }
        }

        if (DEBUG) {
            draw_debug_ui(MEM, PC, opcode, I, REG, STACK);
        }
        EndDrawing();
    }
    rlImGuiShutdown();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
