#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>
#include <stack>
#include <stdio.h>

constexpr int PIXELSIZE = 16;
constexpr int DEBUG = 1;

int load_rom(const char *path, __uint8_t *MEM) {
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

int main() {
    // ---- Setup Chip-8 ----
    // reserve memory
    __uint8_t MEM[4096] = {};

    std::stack<__int16_t> STACK;
    __uint8_t REG[16] = {};

    // Starting addresses from 000 to 1FF are empty
    __uint8_t *PC = MEM + 0x200;
    __uint16_t IR = 0;

    __uint8_t delay_timer = 0;
    __uint8_t sound_timer = 0;

    __uint8_t nibble_first = 0x0;
    __uint8_t nibble_second = 0x0;
    __uint8_t nibble_third = 0x0;
    __uint8_t nibble_fourth = 0x0;

    // ---- Load ROM ----
    const char *path = "./ressources/test_opcode.ch8";

    if (load_rom(path, PC) == 1) {
        printf("Error reading ROM...\n");
        return 1;
    }

    // ---- Setup Raylib ----
    float screenWidth = 64 * PIXELSIZE;
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
            IR = *PC;
            IR = IR << 8;
            PC++;
            IR = IR | *PC;
            PC++;

            // Decode
            nibble_first = (IR >> 12);
            nibble_second = (IR >> 8) & 0x0F;
            nibble_third = (IR >> 4) & 0x00F;
            nibble_fourth = IR & 0x000F;

            switch (nibble_first) {
            case 0x0:
                switch (nibble_second) {
                case 0x0:
                    if (nibble_third == 0xE && nibble_fourth == 0x0) {
                        // Clear screen

                    } else if (nibble_third == 0xE && nibble_fourth == 0xE) {
                        // Return from subroutine to address pulled from stack

                    } else {
                        printf("Unsupported Opcode: 0x%04X", IR);
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
                PC = MEM + (IR & 0x0FFF);
                break;
            }
        }

        BeginDrawing();
        ClearBackground({24, 26, 40, 255});

        if (DEBUG) {
            rlImGuiBegin();

            ImGui::Begin("Debug");
            ImGui::Text("PC: 0x%03X", (unsigned int)(PC - MEM));
            ImGui::Text("IR: 0x%04X", IR);
            ImGui::Text("First Nib: 0x%04X", nibble_first);
            ImGui::Text("Second Nib: 0x%04X", nibble_second);
            ImGui::Text("Third Nib: 0x%04X", nibble_third);
            ImGui::Text("Fourth Nib: 0x%04X", nibble_fourth);
            ImGui::End();

            rlImGuiEnd();
        }
        EndDrawing();
    }
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
