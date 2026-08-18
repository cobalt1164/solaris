#include "CPU.h"
#include <cstdio>

void CPU::executeThumb(uint16_t opcode) {
    // 1. Format 2: Add/Subtract (00011...) - must check before Format 1
    if ((opcode & 0xF800) == 0x1800) {
        // Will implement executeThumbAddSub(opcode);
    }
    // 2. Format 1: Move shifted register (000...)
    else if ((opcode & 0xE000) == 0x0000) {
        executeThumbShift(opcode);
    }
    else {
        // Invalid or unhandled instruction!
        // On real hardware, this triggers an Undefined Instruction Exception (mode 0x1B, vector 0x04).
        std::fprintf(stderr, "[CPU] Unhandled/Invalid Thumb opcode: 0x%04X at PC=0x%08X\n", opcode, r[15]);
    }
}

ALWAYS_INLINE void CPU::executeThumbShift(uint16_t opcode) {
    uint8_t op     = (opcode >> 11) & 0x3;
    uint8_t offset = (opcode >> 6) & 0x1F;
    uint8_t rs     = (opcode >> 3) & 0x7;
    uint8_t rd     = opcode & 0x7;

    uint32_t val = r[rs];
    uint32_t result = val;

    if (op == 0) {
        // LSL (Logical Shift Left)
        if (offset == 0) {
            // LSL #0 performs no shift and leaves C flag unchanged (acts as MOV Rd, Rs)
            result = val;
        } else {
            bool carryBit = (val & (1u << (32 - offset))) != 0;
            setFlag(FLAG_C, carryBit);
            result = val << offset;
        }
    } else if (op == 1) {
        // LSR (Logical Shift Right)
        if (offset == 0) {
            // LSR #0 is interpreted as LSR #32
            bool carryBit = (val & (1u << 31)) != 0;
            setFlag(FLAG_C, carryBit);
            result = 0;
        } else {
            bool carryBit = (val & (1u << (offset - 1))) != 0;
            setFlag(FLAG_C, carryBit);
            result = val >> offset;
        }
    } else if (op == 2) {
        // ASR (Arithmetic Shift Right)
        int32_t sval = static_cast<int32_t>(val);
        if (offset == 0) {
            // ASR #0 is interpreted as ASR #32
            bool carryBit = (val & (1u << 31)) != 0;
            setFlag(FLAG_C, carryBit);
            result = (sval < 0) ? 0xFFFFFFFF : 0;
        } else {
            bool carryBit = (val & (1u << (offset - 1))) != 0;
            setFlag(FLAG_C, carryBit);
            result = static_cast<uint32_t>(sval >> offset);
        }
    }

    r[rd] = result;

    // Update N and Z flags (V flag is unaffected by shift operations)
    setFlag(FLAG_N, (result & (1u << 31)) != 0);
    setFlag(FLAG_Z, result == 0);
}
