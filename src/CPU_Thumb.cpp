#include "CPU.h"
#include <cstdio>

void CPU::executeThumb(uint16_t opcode) {
    // 1. Format 2: Add/Subtract (00011...) - must check before Format 1
    if ((opcode & 0xF800) == 0x1800) {
        executeThumbAddSub(opcode);
    }
    // 2. Format 1: Move shifted register (000...)
    else if ((opcode & 0xE000) == 0x0000) {
        executeThumbShift(opcode);
    }
    // 3. Format 3: Move/Compare/Add/Subtract Immediate (001...)
    else if ((opcode & 0xE000) == 0x2000) {
        executeThumbMove(opcode);
    }
    // 4. Format 4: ALU Operations (010000...)
    else if ((opcode & 0xFC00) == 0x4000) {
        executeThumbALU(opcode);
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

ALWAYS_INLINE void CPU::executeThumbAddSub(uint16_t opcode) {
    uint8_t op       = (opcode >> 9) & 0x3;
    uint8_t rn_or_nn = (opcode >> 6) & 0x7;
    uint8_t rs       = (opcode >> 3) & 0x7;
    uint8_t rd       = opcode & 0x7;

    // Note: When op == 2 and rn_or_nn == 0, this is the alias MOV{ADDS} Rd, Rs (Rd = Rs + 0)
    uint32_t op1 = r[rs];
    uint32_t op2 = (op & 2) ? rn_or_nn : r[rn_or_nn];
    uint32_t result = 0;
    bool carry = false;
    bool overflow = false;

    if ((op & 1) == 0) {
        // Opcode 0 & 2: ADD (Rd = Rs + Rn/nn)
        result = op1 + op2;
        carry = (static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2)) > 0xFFFFFFFFULL;
        overflow = ((~(op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
    } else {
        // Opcode 1 & 3: SUB (Rd = Rs - Rn/nn)
        result = op1 - op2;
        carry = op1 >= op2; // ARM Carry bit set when no borrow occurs
        overflow = (((op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
    }

    r[rd] = result;

    setFlag(FLAG_N, (result & (1u << 31)) != 0);
    setFlag(FLAG_Z, result == 0);
    setFlag(FLAG_C, carry);
    setFlag(FLAG_V, overflow);
}

ALWAYS_INLINE void CPU::executeThumbMove(uint16_t opcode) {
    // Format 3: Move/Compare/Add/Subtract Immediate (001...)
    uint8_t op = (opcode >> 11) & 0x3;
    uint8_t rd = (opcode >> 8) & 0x7;
    uint8_t nn = opcode & 0xFF;

    uint32_t op1 = r[rd];
    uint32_t op2 = nn;
    uint32_t result = 0;

    if (op == 0) {
        // 00b: MOV{S} Rd, #nn (Rd = #nn)
        // Affects only N and Z flags (C and V are unaffected)
        r[rd] = nn;
        setFlag(FLAG_N, (nn & (1u << 31)) != 0);
        setFlag(FLAG_Z, nn == 0);
    } else if (op == 1) {
        // 01b: CMP Rd, #nn (Void = Rd - #nn)
        // Affects N, Z, C, V flags. Does NOT store result to Rd
        result = op1 - op2;
        bool carry = op1 >= op2; // Not Borrow
        bool overflow = (((op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
        setFlag(FLAG_N, (result & (1u << 31)) != 0);
        setFlag(FLAG_Z, result == 0);
        setFlag(FLAG_C, carry);
        setFlag(FLAG_V, overflow);
    } else if (op == 2) {
        // 10b: ADD{S} Rd, #nn (Rd = Rd + #nn)
        // Affects N, Z, C, V flags. Stores result to Rd
        result = op1 + op2;
        bool carry = (static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2)) > 0xFFFFFFFFULL;
        bool overflow = ((~(op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
        r[rd] = result;
        setFlag(FLAG_N, (result & (1u << 31)) != 0);
        setFlag(FLAG_Z, result == 0);
        setFlag(FLAG_C, carry);
        setFlag(FLAG_V, overflow);
    } else if (op == 3) {
        // 11b: SUB{S} Rd, #nn (Rd = Rd - #nn)
        // Affects N, Z, C, V flags. Stores result to Rd
        result = op1 - op2;
        bool carry = op1 >= op2; // Not Borrow
        bool overflow = (((op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
        r[rd] = result;
        setFlag(FLAG_N, (result & (1u << 31)) != 0);
        setFlag(FLAG_Z, result == 0);
        setFlag(FLAG_C, carry);
        setFlag(FLAG_V, overflow);
    }
}

ALWAYS_INLINE void CPU::executeThumbALU(uint16_t opcode) {
    uint8_t op = (opcode >> 6) & 0xF;
    uint8_t rs = (opcode >> 3) & 0x7;
    uint8_t rd = opcode & 0x7;

    uint32_t op1 = r[rd];
    uint32_t op2 = r[rs];
    uint32_t result = 0;

    switch (op) {
        case 0x0: // AND Rd, Rs
            result = op1 & op2;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;

        case 0x1: // EOR Rd, Rs (XOR)
            result = op1 ^ op2;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;

        case 0x2: { // LSL Rd, Rs
            uint8_t shift = op2 & 0xFF;
            if (shift == 0) {
                result = op1;
            } else if (shift <= 32) {
                bool carry = (op1 & (1u << (32 - shift))) != 0;
                setFlag(FLAG_C, carry);
                result = (shift < 32) ? (op1 << shift) : 0;
            } else {
                setFlag(FLAG_C, false);
                result = 0;
            }
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;
        }

        case 0x3: { // LSR Rd, Rs
            uint8_t shift = op2 & 0xFF;
            if (shift == 0) {
                result = op1;
            } else if (shift < 32) {
                bool carry = (op1 & (1u << (shift - 1))) != 0;
                setFlag(FLAG_C, carry);
                result = op1 >> shift;
            } else if (shift == 32) {
                bool carry = (op1 & (1u << 31)) != 0;
                setFlag(FLAG_C, carry);
                result = 0;
            } else {
                setFlag(FLAG_C, false);
                result = 0;
            }
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;
        }

        case 0x4: { // ASR Rd, Rs
            uint8_t shift = op2 & 0xFF;
            int32_t sval = static_cast<int32_t>(op1);
            if (shift == 0) {
                result = op1;
            } else if (shift < 32) {
                bool carry = (op1 & (1u << (shift - 1))) != 0;
                setFlag(FLAG_C, carry);
                result = static_cast<uint32_t>(sval >> shift);
            } else {
                bool carry = (op1 & (1u << 31)) != 0;
                setFlag(FLAG_C, carry);
                result = (sval < 0) ? 0xFFFFFFFF : 0;
            }
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;
        }

        case 0x5: { // ADC Rd, Rs (Rd = Rd + Rs + C)
            uint32_t carryIn = getFlag(FLAG_C) ? 1 : 0;
            uint64_t sum = static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2) + carryIn;
            result = static_cast<uint32_t>(sum);
            bool carry = sum > 0xFFFFFFFFULL;
            bool overflow = ((~(op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, carry);
            setFlag(FLAG_V, overflow);
            break;
        }

        case 0x6: { // SBC Rd, Rs (Rd = Rd - Rs - !C)
            uint32_t borrowIn = getFlag(FLAG_C) ? 0 : 1;
            result = op1 - op2 - borrowIn;
            bool carry = static_cast<uint64_t>(op1) >= (static_cast<uint64_t>(op2) + borrowIn);
            bool overflow = (((op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, carry);
            setFlag(FLAG_V, overflow);
            break;
        }

        case 0x7: { // ROR Rd, Rs
            uint8_t rawShift = op2 & 0xFF;
            uint8_t shift = rawShift & 31;
            if (rawShift == 0) {
                result = op1;
            } else if (shift == 0) {
                result = op1;
                bool carry = (op1 & (1u << 31)) != 0;
                setFlag(FLAG_C, carry);
            } else {
                result = (op1 >> shift) | (op1 << (32 - shift));
                bool carry = (op1 & (1u << (shift - 1))) != 0;
                setFlag(FLAG_C, carry);
            }
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;
        }

        case 0x8: // TST Rd, Rs (Void = Rd & Rs)
            result = op1 & op2;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;

        case 0x9: { // NEG Rd, Rs (Rd = 0 - Rs)
            result = 0 - op2;
            bool carry = (op2 == 0);
            bool overflow = (((0 ^ op2) & (0 ^ result)) & 0x80000000u) != 0;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, carry);
            setFlag(FLAG_V, overflow);
            break;
        }

        case 0xA: { // CMP Rd, Rs (Void = Rd - Rs)
            result = op1 - op2;
            bool carry = op1 >= op2;
            bool overflow = (((op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, carry);
            setFlag(FLAG_V, overflow);
            break;
        }

        case 0xB: { // CMN Rd, Rs (Void = Rd + Rs)
            result = op1 + op2;
            bool carry = (static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2)) > 0xFFFFFFFFULL;
            bool overflow = ((~(op1 ^ op2) & (op1 ^ result)) & 0x80000000u) != 0;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            setFlag(FLAG_C, carry);
            setFlag(FLAG_V, overflow);
            break;
        }

        case 0xC: // ORR Rd, Rs
            result = op1 | op2;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;

        case 0xD: // MUL Rd, Rs
            result = op1 * op2;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;

        case 0xE: // BIC Rd, Rs (Rd = Rd & ~Rs)
            result = op1 & (~op2);
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;

        case 0xF: // MVN Rd, Rs (Rd = ~Rs)
            result = ~op2;
            r[rd] = result;
            setFlag(FLAG_N, (result & (1u << 31)) != 0);
            setFlag(FLAG_Z, result == 0);
            break;
    }
}
