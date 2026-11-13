#include "chip8.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>
#include <stdio.h>

constexpr int DEBUG = 0;

constexpr int FPS = 60;
constexpr int IPF = 700 / FPS; // Instructions per frame

constexpr int PIXELSIZE = 16;
constexpr int DEBUG_PANEL_WIDTH = 320; // extra window width for the ImGui panel
constexpr float SCREEN_WIDTH = 64 * PIXELSIZE + (DEBUG ? DEBUG_PANEL_WIDTH : 0);
constexpr float SCREEN_HEIGHT = 32 * PIXELSIZE;

int main() {
    // ---- Setup Raylib and imGui ----
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chip-8 Emulator");
    SetTargetFPS(FPS);
    rlImGuiSetup(true);

    // ---- Chip-8 Init ----
    Chip8 c;
    chip8_init(c);

    const char *path = "./ressources/5-quirks.ch8";
    chip8_load(c, path);

    while (!WindowShouldClose()) {
        int steps = 0;

        if (DEBUG) {
            steps = IsKeyPressed(KEY_SPACE) ? 1 : 0;
        } else {
            steps = IPF;
        }

        for (int i = 0; i < steps; ++i) {
            if (!chip8_step(c)) {
                printf("Executing emulator step failed.");
                return 1;
            }
        }
        chip8_tick_timers(c);

        BeginDrawing();
        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 32; y++) {
                if (c.DISP[x][y]) {
                    DrawRectangle(x * PIXELSIZE, y * PIXELSIZE, PIXELSIZE,
                                  PIXELSIZE, WHITE);
                } else {
                    DrawRectangle(x * PIXELSIZE, y * PIXELSIZE, PIXELSIZE,
                                  PIXELSIZE, BLACK);
                }
            }
        }

        if (DEBUG) {
            chip8_draw_debug(c, 64 * PIXELSIZE, 0.0f, DEBUG_PANEL_WIDTH,
                             32 * PIXELSIZE);
        }
        EndDrawing();
    }
    rlImGuiShutdown();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
