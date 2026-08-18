#include "CPU.h"
#include <cstdio>

void CPU::executeARM(uint32_t opcode) {
    // Stub for ARM mode instruction decoder
    // Will handle 32-bit ARM instructions (Data processing, Branch, Load/Store, etc.)
    std::fprintf(stderr, "[CPU] Unhandled ARM opcode: 0x%08X at PC=0x%08X\n", opcode, r[15]);
}
