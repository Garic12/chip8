#pragma once
#include "chip8.h"
#include <imgui.h>
#include <rlImGui.h>

void draw_debug(const chip8::Chip8 &c, float x, float y, float w, float h);
