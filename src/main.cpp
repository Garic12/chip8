#include <cstdint>
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>
#include <stack>
#include <stdio.h>

constexpr int PIXELSIZE = 16;
constexpr int DEBUG = 0;
constexpr int DEBUG_PANEL_WIDTH = 320; // extra window width for the ImGui panel

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
                   uint16_t I, const __uint8_t *REG) {
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

    ImGui::End();
    rlImGuiEnd();
}

int main() {
    // ---- Setup Chip-8 ----
    // reserve memory
    uint8_t MEM[4096] = {};

    std::stack<__int16_t> STACK;
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

    bool DISP[64][32] = {};

    // ---- Load ROM ----
    const char *path = "./ressources/test_opcode.ch8";

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

    const int IPF = 700 / 60; // Instructions per frame

    while (!WindowShouldClose()) {
        int steps = 0;

        if (DEBUG) {
            steps = IsKeyPressed(KEY_SPACE) ? 1 : 0;
        } else {
            steps = IPF;
        }

        for (int i = 0; i < steps; ++i) {
            // Fetch
            opcode = (PC[0] << 8) | PC[1];
            PC += 2;

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

                    } else {
                        printf("Unsupported Opcode: 0x%04X", opcode);
                        return 1;
                    }
                    break;
                default:
                    // Jump to native assembler subroutine at 0xNNN
                    break;
                }
                break;
            case 0x1:
                // jump to address NNN
                PC = MEM + (opcode & 0x0FFF);
                break;
            case 0x6:
                // Set the register VX to the value NN
                *(REG + nibble_second) = (nibble_third << 4) + nibble_fourth;
                break;
            case 0x7:
                *(REG + nibble_second) += (nibble_third << 4) + nibble_fourth;
                break;
            case 0xA:
                I = opcode & 0x0FFF;
                break;
            case 0xD: {
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
            }
        }

        BeginDrawing();
        ClearBackground({24, 26, 40, 255});

        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 32; y++) {
                if (DISP[x][y])
                    DrawRectangle(x, y, PIXELSIZE, PIXELSIZE, WHITE);
            }
        }

        if (DEBUG) {
            draw_debug_ui(MEM, PC, opcode, I, REG);
        }
        EndDrawing();
    }
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
