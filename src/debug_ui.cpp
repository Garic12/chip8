#include "debug_ui.h"

void draw_debug(const chip8::Chip8 &c, float x, float y, float w, float h) {
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
