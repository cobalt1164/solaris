#include "../src/CPU.h"
#include <iostream>

static uint16_t makeFormat1(uint8_t op, uint8_t offset, uint8_t rs, uint8_t rd) {
    return ((op & 3) << 11) | ((offset & 0x1F) << 6) | ((rs & 7) << 3) | (rd & 7);
}

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << " (" #cond ")\n"; \
            return false; \
        } \
    } while (0)

static bool test_LSL_Standard() {
    CPU cpu;
    cpu.setReg(1, 0x00000002);
    uint16_t opcode = makeFormat1(0, 4, 1, 0); // LSL R0, R1, #4
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0x00000020, "R0 should be 0x20");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "Z flag should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_N), "N flag should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_C), "C flag should be 0");
    return true;
}

static bool test_LSL_CarryOut() {
    CPU cpu;
    cpu.setReg(1, 0x80000001);
    uint16_t opcode = makeFormat1(0, 1, 1, 0); // LSL R0, R1, #1
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0x00000002, "R0 should be 0x2");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C flag should be 1");
    return true;
}

static bool test_LSL_ZeroShift() {
    CPU cpu;
    cpu.setReg(1, 0x12345678);
    cpu.setFlag(CPU::FLAG_C, true); // Set C flag initially
    uint16_t opcode = makeFormat1(0, 0, 1, 0); // LSL R0, R1, #0 (MOV R0, R1)
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0x12345678, "R0 should be 0x12345678");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C flag should be unchanged (1)");
    return true;
}

static bool test_LSR_Standard() {
    CPU cpu;
    cpu.setReg(1, 0x0000000B); // Binary: ...1011 (Bit 1 is 1)
    uint16_t opcode = makeFormat1(1, 2, 1, 0); // LSR R0, R1, #2
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0x00000002, "R0 should be 0x2");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C flag should be 1 (bit 1 shifted out last)");
    return true;
}

static bool test_LSR_ZeroShift_32() {
    CPU cpu;
    cpu.setReg(1, 0x80000000);
    uint16_t opcode = makeFormat1(1, 0, 1, 0); // LSR R0, R1, #0 (LSR #32)
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0x00000000, "R0 should be 0");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_Z), "Z flag should be 1");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C flag should be 1 (bit 31)");
    return true;
}

static bool test_ASR_Standard() {
    CPU cpu;
    cpu.setReg(1, 0x80000000); // Negative value
    uint16_t opcode = makeFormat1(2, 2, 1, 0); // ASR R0, R1, #2
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0xE0000000, "R0 should be 0xE0000000");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_N), "N flag should be 1");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_C), "C flag should be 0");
    return true;
}

static bool test_ASR_ZeroShift_32() {
    CPU cpu;
    cpu.setReg(1, 0x80000000); // Negative value
    uint16_t opcode = makeFormat1(2, 0, 1, 0); // ASR R0, R1, #0 (ASR #32)
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0xFFFFFFFF, "R0 should be 0xFFFFFFFF");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_N), "N flag should be 1");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C flag should be 1");
    return true;
}

int main() {
    std::cout << "=======================================\n";
    std::cout << " Solaris CPU Instruction Unit Tests\n";
    std::cout << "=======================================\n";

    int passed = 0;
    int total = 0;

    auto run = [&](const char* name, bool (*func)()) {
        total++;
        std::cout << "Testing " << name << "... ";
        if (func()) {
            std::cout << "PASSED\n";
            passed++;
        }
    };

    run("LSL Standard", test_LSL_Standard);
    run("LSL Carry Out", test_LSL_CarryOut);
    run("LSL #0 (Unchanged Carry)", test_LSL_ZeroShift);
    run("LSR Standard", test_LSR_Standard);
    run("LSR #0 (LSR #32)", test_LSR_ZeroShift_32);
    run("ASR Standard", test_ASR_Standard);
    run("ASR #0 (ASR #32)", test_ASR_ZeroShift_32);

    std::cout << "---------------------------------------\n";
    std::cout << "Results: " << passed << " / " << total << " tests passed.\n";
    std::cout << "=======================================\n";

    return (passed == total) ? 0 : 1;
}
