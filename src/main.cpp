#include "audio.h"
#include "chip8.h"
#include "debug_ui.h"
#include <cstdio>
#include <raylib.h>
#include <rlImGui.h>
#include <span>
#include <string>
#include <string_view>

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

constexpr int FPS = 60;
constexpr int IPF = 700 / FPS; // Instructions per frame

constexpr float PIXELSIZE = 16.0;
constexpr float DEBUG_PANEL_WIDTH =
    320.0; // extra window width for the ImGui panel

int main(int argc, char *argv[]) {
    std::string path;
    bool audio_enabled = true;
    bool debug = false;

    auto args = std::span(argv, argc);
    for (std::string_view arg : args) {
        if (arg == "--no-audio") {
            audio_enabled = false;
        } else if (arg == "--debug") {
            debug = true;
        } else if (arg.starts_with("--")) {
            printf("Usage: %s [--no-audio|--debug] <rom.ch8>\n", args[0]);
            return 1;
        } else {
            path = arg;
        }
    }

    if (path.empty()) {
        printf("Usage: %s [--no-audio|--debug] <rom.ch8>\n", args[0]);
        return 1;
    }

    // ---- Setup Raylib and imGui ----
    const float screen_width =
        (64 * PIXELSIZE) + (debug ? DEBUG_PANEL_WIDTH : 0);
    const float screen_height = 32 * PIXELSIZE;

    InitWindow(static_cast<int>(screen_width), static_cast<int>(screen_height),
               "Chip-8 Emulator");
    if (!IsWindowReady()) {
        printf("Failed to create window\n");
        return 1;
    }
    SetTargetFPS(FPS);
    rlImGuiSetup(true);

    Audio audio;
    if (audio_enabled) {
        audio = audio_init();
    }

    // ---- Chip-8 Init ----
    chip8::Chip8 c;
    chip8::init(c);
    if (!chip8::load(c, path)) {
        printf("Failed to load ROM\n");
    }

    while (!WindowShouldClose()) {
        int steps = 0;

        if (debug) {
            steps = IsKeyPressed(KEY_SPACE) ? 1 : 0;
        } else {
            steps = IPF;
        }

        for (int i = 0; i < steps; ++i) {
            for (int i = 0; i < 16; ++i) {
                if (IsKeyReleased(KEYMAP[i])) {
                    c.KB[i] = 2;
                } else if (IsKeyDown(KEYMAP[i])) {
                    c.KB[i] = 1;
                }
            }
            if (!chip8::step(c)) {
                return 1;
            }
            if (audio_enabled) {
                audio_update(audio, c.sound > 0);
            }
        }
        chip8::tick_timers(c);

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

        if (debug) {
            draw_debug(c, 64 * PIXELSIZE, 0.0, DEBUG_PANEL_WIDTH,
                       32 * PIXELSIZE);
        }
        EndDrawing();
    }

    rlImGuiShutdown();
    if (audio_enabled) {
        audio_shutdown();
    }
    CloseWindow();
    return 0;
}
