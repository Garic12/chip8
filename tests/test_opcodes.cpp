#include "chip8.h"
#include <catch2/catch_test_macros.hpp>

// Helper: fresh machine with one opcode at 0x200
static chip8::Chip8 with_opcode(const uint16_t opcode) {
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

    c.REG[0xA] = 0;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 1);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 0;
    c.REG[0xB] = 0;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 1;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 1);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY2 set VX to VX & VY") {
    auto c = with_opcode(0x8AB2);

    c.REG[0xA] = 0;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 0;
    c.REG[0xB] = 0;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 1;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 1);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY3 set VX to VX XOR VY") {
    auto c = with_opcode(0x8AB3);

    c.REG[0xA] = 0;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 1);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 0;
    c.REG[0xB] = 0;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 1;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY4 set VX to VX + VY") {
    auto c = with_opcode(0x8AB4);

    c.REG[0xA] = 2;
    c.REG[0xB] = 3;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 5);
    CHECK(c.REG[0xF] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 255;
    c.REG[0xB] = 0;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 255);
    CHECK(c.REG[0xF] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 255;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.REG[0xF] == 1);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY5 set VX to VX - VY") {
    auto c = with_opcode(0x8AB5);

    c.REG[0xA] = 5;
    c.REG[0xB] = 3;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 2);
    CHECK(c.REG[0xF] == 1);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xA] = 5;
    c.REG[0xB] = 7;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0xFE);
    CHECK(c.REG[0xF] == 0);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY7 set VX to VY - VX") {
    auto c = with_opcode(0x8AB7);

    c.REG[0xB] = 5;
    c.REG[0xA] = 3;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 2);
    CHECK(c.REG[0xF] == 1);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xB] = 5;
    c.REG[0xA] = 7;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0xFE);
    CHECK(c.REG[0xF] == 0);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XY6 set VX to VY shifted 1 bit to the right") {
    auto c = with_opcode(0x8AB6);

    c.REG[0xB] = 4;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 2);
    CHECK(c.REG[0xF] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.REG[0xF] == 1);
    CHECK(c.PC == 0x202);
}

TEST_CASE("8XYE set VX to VY shifted 1 bit to the left") {
    auto c = with_opcode(0x8ABE);

    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 2);
    CHECK(c.REG[0xF] == 0);
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xB] = 0b10000000;
    REQUIRE(chip8::step(c));
    CHECK(c.REG[0xA] == 0);
    CHECK(c.REG[0xF] == 1);
    CHECK(c.PC == 0x202);
}

TEST_CASE("0x9XY0 skip if VX != VY") {
    auto c = with_opcode(0x9AB0);

    REQUIRE(chip8::step(c));
    CHECK(c.PC == 0x202);

    c.PC = 0x200;
    c.REG[0xB] = 1;
    REQUIRE(chip8::step(c));
    CHECK(c.PC == 0x204);
}

TEST_CASE("0xANNN set I to NNN") {
    auto c = with_opcode(0xA123);

    REQUIRE(chip8::step(c));
    CHECK(c.I == 0x123);
    CHECK(c.PC == 0x202);
}

TEST_CASE("0xBNNN jump to address NNN + V0") {
    auto c = with_opcode(0xB200);

    c.REG[0] = 0x10;

    REQUIRE(chip8::step(c));
    CHECK(c.PC == 0x210);
}

TEST_CASE("0xCXNN set VX to random byte") {
    auto c = with_opcode(0xC3FF);
    c.rng = 1;

    REQUIRE(chip8::step(c));
    CHECK(c.rng == 0x00042021);
    CHECK(c.REG[3] == 0x21);
    CHECK(c.PC == 0x202);
}

TEST_CASE("0xDXYN draw sprite") {
    SECTION("draw single row") {
        auto c = with_opcode(0xD011);

        c.REG[0] = 0;
        c.REG[1] = 1;
        c.I = 0x300;
        c.MEM[0x300] = 0b10000000;

        REQUIRE(chip8::step(c));
        CHECK_FALSE(c.DISP[0][0]);
        CHECK(c.DISP[0][1]);
        CHECK_FALSE(c.DISP[1][1]);
    }
    SECTION("draw twice erases and sets VF") {
        auto c = with_opcode(0xD011);

        c.REG[0] = 0;
        c.REG[1] = 1;
        c.I = 0x300;
        c.MEM[0x300] = 0b10000000;

        REQUIRE(chip8::step(c));
        CHECK(c.DISP[0][1]);

        c.PC = 0x200;
        REQUIRE(chip8::step(c));
        CHECK_FALSE(c.DISP[0][1]);
        CHECK(c.REG[0xF] == 1);
    }
    SECTION("multiple rows") {
        auto c = with_opcode(0xD012);

        c.I = 0x300;
        c.MEM[0x300] = 0b10000000;
        c.MEM[0x301] = 0b01000000;

        REQUIRE(chip8::step(c));
        CHECK(c.DISP[0][0]);
        CHECK(c.DISP[1][1]);
    }
    SECTION("starting position wraps") {
        auto c = with_opcode(0xD011);

        c.REG[0] = 64;
        c.REG[1] = 32;

        c.I = 0x300;
        c.MEM[0x300] = 0b10000000;
        REQUIRE(chip8::step(c));
        CHECK(c.DISP[0][0]);
    }
}

TEST_CASE("0xEX key related skipping") {
    SECTION("0xEX9E skip if key VX is pressed") {
        auto c = with_opcode(0xE09E);

        c.KB[0] = 1;
        REQUIRE(chip8::step(c));
        CHECK(c.PC == 0x204);

        c.PC = 0x200;
        c.KB[0] = 0;
        REQUIRE(chip8::step(c));
        CHECK(c.PC == 0x202);

        c.PC = 0x200;
        c.KB[0] = 2;
        REQUIRE(chip8::step(c));
        CHECK(c.PC == 0x202);
    }

    SECTION("0xEXA1 skip if key VX is not pressed") {
        auto c = with_opcode(0xE0A1);

        c.KB[0] = 0;
        REQUIRE(chip8::step(c));
        CHECK(c.PC == 0x204);

        c.PC = 0x200;
        c.KB[0] = 1;
        REQUIRE(chip8::step(c));
        CHECK(c.PC == 0x202);

        c.PC = 0x200;
        c.KB[0] = 2;
        REQUIRE(chip8::step(c));
        CHECK(c.PC == 0x202);
    }
}

TEST_CASE("0xF functionality") {
    SECTION("0xFX07 get delay to VX") {
        auto c = with_opcode(0xF007);
        c.delay = 10;

        REQUIRE(chip8::step(c));
        CHECK(c.REG[0] == 10);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX15 set VX to delay") {
        auto c = with_opcode(0xF015);
        c.REG[0] = 10;

        REQUIRE(chip8::step(c));
        CHECK(c.delay == 10);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX18 set VX to sound timer") {
        auto c = with_opcode(0xF018);
        c.REG[0] = 10;

        REQUIRE(chip8::step(c));
        CHECK(c.sound == 10);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX1E add VX to I") {
        auto c = with_opcode(0xF01E);
        c.REG[0] = 0x10;
        c.I = 0x10;

        REQUIRE(chip8::step(c));
        CHECK(c.I == 0x20);
        CHECK(c.REG[0xF] == 0);
        CHECK(c.PC == 0x202);

        c.PC = 0x200;
        c.REG[0] = 0x02;
        c.I = 0xFFF;
        REQUIRE(chip8::step(c));
        CHECK(c.I == 0x1001);
        CHECK(c.REG[0xF] == 1);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX0A get key released") {
        auto c = with_opcode(0xF00A);

        c.KB[0xA] = 2;
        REQUIRE(chip8::step(c));
        CHECK(c.REG[0] == 0xA);
        CHECK(c.PC == 0x202);

        c.PC = 0x200;
        c.KB[0xA] = 0;
        REQUIRE(chip8::step(c));
        CHECK(c.PC == 0x200);
    }
    SECTION("0xFX29 get font character") {
        auto c = with_opcode(0xF029);

        c.REG[0] = 0x1;
        REQUIRE(chip8::step(c));
        CHECK(c.I == 0x5);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX33 binary coded decimal conversion") {
        auto c = with_opcode(0xF033);

        c.REG[0] = 125;
        c.I = 0x300;

        REQUIRE(chip8::step(c));
        CHECK(c.MEM[c.I] == 1);
        CHECK(c.MEM[c.I + 1] == 2);
        CHECK(c.MEM[c.I + 2] == 5);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX55 store memory") {
        auto c = with_opcode(0xF033);

        c.REG[0] = 125;
        c.I = 0x300;

        REQUIRE(chip8::step(c));
        CHECK(c.MEM[c.I] == 1);
        CHECK(c.MEM[c.I + 1] == 2);
        CHECK(c.MEM[c.I + 2] == 5);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX55 store V0..VX at I") {
        auto c = with_opcode(0xF255);
        c.REG[0] = 0xAA;
        c.REG[1] = 0xBB;
        c.REG[2] = 0xCC;
        c.REG[3] = 0xDD;
        c.I = 0x300;

        REQUIRE(chip8::step(c));
        CHECK(c.MEM[0x300] == 0xAA);
        CHECK(c.MEM[0x301] == 0xBB);
        CHECK(c.MEM[0x302] == 0xCC);
        CHECK(c.MEM[0x303] == 0x00);
        CHECK(c.I == 0x300);
        CHECK(c.PC == 0x202);
    }
    SECTION("0xFX65 load V0..VX from I") {
        auto c = with_opcode(0xF265);
        c.MEM[0x300] = 0xAA;
        c.MEM[0x301] = 0xBB;
        c.MEM[0x302] = 0xCC;
        c.MEM[0x303] = 0xDD;
        c.I = 0x300;

        REQUIRE(chip8::step(c));
        CHECK(c.REG[0] == 0xAA);
        CHECK(c.REG[1] == 0xBB);
        CHECK(c.REG[2] == 0xCC);
        CHECK(c.REG[3] == 0x00);
        CHECK(c.I == 0x300);
        CHECK(c.PC == 0x202);
    }
}