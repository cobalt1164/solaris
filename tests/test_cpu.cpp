#include "../src/CPU.h"
#include <iostream>

static uint16_t makeFormat1(uint8_t op, uint8_t offset, uint8_t rs, uint8_t rd) {
    return ((op & 3) << 11) | ((offset & 0x1F) << 6) | ((rs & 7) << 3) | (rd & 7);
}

static uint16_t makeFormat2(uint8_t op, uint8_t rn_or_nn, uint8_t rs, uint8_t rd) {
    return (3 << 11) | ((op & 3) << 9) | ((rn_or_nn & 7) << 6) | ((rs & 7) << 3) | (rd & 7);
}

static uint16_t makeFormat3(uint8_t op, uint8_t rd, uint8_t nn) {
    return (1 << 13) | ((op & 3) << 11) | ((rd & 7) << 8) | (nn & 0xFF);
}

static uint16_t makeFormat4(uint8_t op, uint8_t rs, uint8_t rd) {
    return 0x4000 | ((op & 0xF) << 6) | ((rs & 7) << 3) | (rd & 7);
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

static bool test_ADD_Register() {
    CPU cpu;
    cpu.setReg(1, 15);
    cpu.setReg(2, 27);
    uint16_t opcode = makeFormat2(0, 2, 1, 0); // ADD R0, R1, R2
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 42, "R0 should be 42");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "Z should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_N), "N should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_C), "C should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_V), "V should be 0");
    return true;
}

static bool test_SUB_Register_Borrow() {
    CPU cpu;
    cpu.setReg(1, 5);
    cpu.setReg(2, 10);
    uint16_t opcode = makeFormat2(1, 2, 1, 0); // SUB R0, R1, R2 (5 - 10 = -5)
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == static_cast<uint32_t>(-5), "R0 should be -5");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_N), "N should be 1");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_C), "C should be 0 (borrow occurred)");
    return true;
}

static bool test_ADD_Immediate() {
    CPU cpu;
    cpu.setReg(1, 100);
    uint16_t opcode = makeFormat2(2, 5, 1, 0); // ADD R0, R1, #5
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 105, "R0 should be 105");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "Z should be 0");
    return true;
}

static bool test_SUB_Immediate_Zero() {
    CPU cpu;
    cpu.setReg(1, 4);
    uint16_t opcode = makeFormat2(3, 4, 1, 0); // SUB R0, R1, #4 (4 - 4 = 0)
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0, "R0 should be 0");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_Z), "Z should be 1");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C should be 1 (no borrow)");
    return true;
}

static bool test_ADD_Overflow() {
    CPU cpu;
    cpu.setReg(1, 0x7FFFFFFF); // Max positive signed 32-bit int
    cpu.setReg(2, 1);
    uint16_t opcode = makeFormat2(0, 2, 1, 0); // ADD R0, R1, R2
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0x80000000, "R0 should be 0x80000000");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_N), "N should be 1");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_V), "V should be 1 (overflow)");
    return true;
}

static bool test_Format3_MOV() {
    CPU cpu;
    cpu.setFlag(CPU::FLAG_C, true); // Set C flag
    cpu.setFlag(CPU::FLAG_V, true); // Set V flag
    uint16_t opcode = makeFormat3(0, 0, 255); // MOV R0, #255
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 255, "R0 should be 255");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "Z should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_N), "N should be 0");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C flag should remain unchanged (1)");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_V), "V flag should remain unchanged (1)");

    uint16_t opcode_zero = makeFormat3(0, 0, 0); // MOV R0, #0
    cpu.executeThumb(opcode_zero);
    TEST_ASSERT(cpu.getReg(0) == 0, "R0 should be 0");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_Z), "Z should be 1");
    return true;
}

static bool test_Format3_CMP() {
    CPU cpu;
    cpu.setReg(0, 50);
    uint16_t opcode = makeFormat3(1, 0, 50); // CMP R0, #50
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 50, "R0 must not be modified by CMP");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_Z), "Z flag should be 1 (50 - 50 = 0)");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C flag should be 1 (no borrow)");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_N), "N flag should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_V), "V flag should be 0");
    return true;
}

static bool test_Format3_ADD() {
    CPU cpu;
    cpu.setReg(0, 100);
    uint16_t opcode = makeFormat3(2, 0, 50); // ADD R0, #50
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 150, "R0 should be 150");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "Z should be 0");
    return true;
}

static bool test_Format3_SUB() {
    CPU cpu;
    cpu.setReg(0, 100);
    uint16_t opcode = makeFormat3(3, 0, 30); // SUB R0, #30
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 70, "R0 should be 70");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "Z should be 0");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "C should be 1 (no borrow)");
    return true;
}

static bool test_Format2_MOV_Alias() {
    CPU cpu;
    cpu.setReg(1, 0x12345678);
    uint16_t opcode = makeFormat2(2, 0, 1, 0); // ADD R0, R1, #0 (MOV Rd, Rs alias)
    cpu.executeThumb(opcode);

    TEST_ASSERT(cpu.getReg(0) == 0x12345678, "R0 should be 0x12345678");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "Z should be 0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_N), "N should be 0");
    return true;
}

static bool test_Format4_Bitwise() {
    CPU cpu;
    cpu.setReg(0, 0x0F0F0F0F);
    cpu.setReg(1, 0xFF00FF00);

    // AND R0, R1 -> R0 = 0x0F000F00
    cpu.executeThumb(makeFormat4(0x0, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 0x0F000F00, "AND failed");

    // EOR R0, R1 -> 0x0F000F00 ^ 0xFF00FF00 = 0xF000F000
    cpu.executeThumb(makeFormat4(0x1, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 0xF000F000, "EOR failed");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_N), "N should be 1 for negative result");

    // BIC R0, R1 -> 0xF000F000 & ~0xFF00FF00 = 0x00000000
    cpu.executeThumb(makeFormat4(0xE, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 0, "BIC failed");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_Z), "Z should be 1 for zero result");

    // MVN R0, R1 -> R0 = ~0xFF00FF00 = 0x00FF00FF
    cpu.executeThumb(makeFormat4(0xF, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 0x00FF00FF, "MVN failed");

    return true;
}

static bool test_Format4_ADC_SBC_NEG() {
    CPU cpu;
    cpu.setReg(0, 10);
    cpu.setReg(1, 20);
    cpu.setFlag(CPU::FLAG_C, true); // Carry = 1

    // ADC R0, R1 -> 10 + 20 + 1 = 31
    cpu.executeThumb(makeFormat4(0x5, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 31, "ADC failed");

    // SBC R0, R1 -> 31 - 20 - 0 = 11 (C was set to 0 by ADC 31)
    cpu.setFlag(CPU::FLAG_C, true); // Set C = 1 (no borrow)
    cpu.executeThumb(makeFormat4(0x6, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 11, "SBC failed");

    // NEG R0, R1 -> R0 = 0 - R1 = 0 - 20 = -20
    cpu.executeThumb(makeFormat4(0x9, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == static_cast<uint32_t>(-20), "NEG failed");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_N), "NEG N flag failed");

    return true;
}

static bool test_Format4_TST_CMP_CMN() {
    CPU cpu;
    cpu.setReg(0, 100);
    cpu.setReg(1, 100);

    // TST R0, R1 -> 100 & 100 != 0, Void result
    cpu.executeThumb(makeFormat4(0x8, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 100, "TST modified R0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "TST Z flag should be 0");

    // CMP R0, R1 -> 100 - 100 = 0, Void result
    cpu.executeThumb(makeFormat4(0xA, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 100, "CMP modified R0");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_Z), "CMP Z flag should be 1");
    TEST_ASSERT(cpu.getFlag(CPU::FLAG_C), "CMP C flag should be 1 (no borrow)");

    // CMN R0, R1 -> 100 + 100 = 200, Void result
    cpu.executeThumb(makeFormat4(0xB, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 100, "CMN modified R0");
    TEST_ASSERT(!cpu.getFlag(CPU::FLAG_Z), "CMN Z flag should be 0");

    return true;
}

static bool test_Format4_MUL_ROR() {
    CPU cpu;
    cpu.setReg(0, 6);
    cpu.setReg(1, 7);

    // MUL R0, R1 -> 6 * 7 = 42
    cpu.executeThumb(makeFormat4(0xD, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 42, "MUL failed");

    // ROR R0, R1 -> 42 rotated right by (7 & 31 = 7)
    // 42 = 0x2A -> (0x2A >> 7) | (0x2A << 25) = 0x54000000
    cpu.executeThumb(makeFormat4(0x7, 1, 0));
    TEST_ASSERT(cpu.getReg(0) == 0x54000000, "ROR failed");

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

    run("ADD Register", test_ADD_Register);
    run("SUB Register (Borrow)", test_SUB_Register_Borrow);
    run("ADD Immediate", test_ADD_Immediate);
    run("SUB Immediate (Zero)", test_SUB_Immediate_Zero);
    run("ADD Overflow", test_ADD_Overflow);

    run("Format 3: MOV Immediate", test_Format3_MOV);
    run("Format 3: CMP Immediate", test_Format3_CMP);
    run("Format 3: ADD Immediate", test_Format3_ADD);
    run("Format 3: SUB Immediate", test_Format3_SUB);
    run("Format 2: MOV Alias (ADD #0)", test_Format2_MOV_Alias);

    run("Format 4: Bitwise (AND, EOR, BIC, MVN)", test_Format4_Bitwise);
    run("Format 4: Arithmetic (ADC, SBC, NEG)", test_Format4_ADC_SBC_NEG);
    run("Format 4: Comparisons (TST, CMP, CMN)", test_Format4_TST_CMP_CMN);
    run("Format 4: Multiply & Rotate (MUL, ROR)", test_Format4_MUL_ROR);

    std::cout << "---------------------------------------\n";
    std::cout << "Results: " << passed << " / " << total << " tests passed.\n";
    std::cout << "=======================================\n";

    return (passed == total) ? 0 : 1;
}
