#include "chip8.h"
#include <catch2/catch_test_macros.hpp>

// Helper: fresh machine with one opcode at 0x200
static chip8::Chip8 with_opcode(uint16_t opcode) {
    chip8::Chip8 c;
    chip8::init(c);
    c.MEM[0x200] = opcode >> 8;
    c.MEM[0x201] = opcode & 0xFF;
    return c;
}

TEST_CASE("0xFFFF unsupported opcode") {
    auto c = with_opcode(0xFFFF);

    REQUIRE_FALSE(chip8::step(c));
}

TEST_CASE("0x00E0 clear screen") {
    auto c = with_opcode(0x00E0);
    c.DISP[0][0] = true;

    REQUIRE(chip8::step(c));
    REQUIRE(c.DISP[0][0] == false);
    CHECK(c.PC == 0x202);
}

TEST_CASE("0x00EE single return from subroutine") {
    auto c = with_opcode(0x00EE);
    c.SP = 1;
    c.STACK[0] = 0x210;

    REQUIRE(chip8::step(c));
    REQUIRE(c.SP == 0);
    CHECK(c.PC == 0x212);
}

TEST_CASE("0x1NNN jump address") {
    auto c = with_opcode(0x1234);

    REQUIRE(chip8::step(c));
    CHECK(c.PC == 0x234);
}

TEST_CASE("0x2NNN single call subroutine") {
    auto c = with_opcode(0x2210);

    REQUIRE(chip8::step(c));
    REQUIRE(c.STACK.at(0) == 0x200);
    REQUIRE(c.SP == 1);
    CHECK(c.PC == 0x210);
}

TEST_CASE("0x3XNN skip if VX == NN") {
    auto c = with_opcode(0x3000);

    REQUIRE(chip8::step(c));
    CHECK(c.PC == 0x204);
}

TEST_CASE("0x4XNN skip if VX != NN") {
    auto c = with_opcode(0x4001);

    REQUIRE(chip8::step(c));
    CHECK(c.PC == 0x204);
}

TEST_CASE("0x5XNN skip if VX == VY") {
    auto c = with_opcode(0x5000);

    REQUIRE(chip8::step(c));
    CHECK(c.PC == 0x204);
}

TEST_CASE("6XNN sets VX to NN") {
    auto c = with_opcode(0x6A42); // VA = 0x42

    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0x42);
    CHECK(c.PC == 0x202);
}

TEST_CASE("7XNN add NN to VX") {
    auto c = with_opcode(0x70AB);

    REQUIRE(chip8::step(c));
    CHECK(c.REG[0] == 0xAB);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY0 set VX to VY") {
    auto c = with_opcode(0x8AB0);
    c.REG[0xB] = 5;

    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 5);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY1 set VX to VX | VY") {
    auto c = with_opcode(0x8AB1);
    c.REG[0xB] = 1;

    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 1);
    CHECK(c.PC == 0x202);
}
