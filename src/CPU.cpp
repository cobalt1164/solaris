#include "CPU.h"
#include <algorithm>
#include <cstdio>

static int getBankIndex(CPUMode mode) {
  switch (mode) {
  case CPUMode::User:
  case CPUMode::System:
    return 0;
  case CPUMode::FIQ:
    return 1;
  case CPUMode::IRQ:
    return 2;
  case CPUMode::Supervisor:
    return 3;
  case CPUMode::Abort:
    return 4;
  case CPUMode::Undefined:
    return 5;
  }
  return 0;
}

CPU::CPU() { reset(); }

void CPU::reset() {
  // Zero out all registers
  std::fill(std::begin(r), std::end(r), 0);
  std::fill(std::begin(banked_r13), std::end(banked_r13), 0);
  std::fill(std::begin(banked_r14), std::end(banked_r14), 0);
  std::fill(std::begin(banked_spsr), std::end(banked_spsr), 0);
  std::fill(std::begin(banked_r8_r12_fiq), std::end(banked_r8_r12_fiq), 0);
  std::fill(std::begin(r8_r12_usr), std::end(r8_r12_usr), 0);

  // ARM7TDMI starts in Supervisor (SVC) mode with interrupts (IRQ/FIQ)
  // disabled. CPSR mode bits for SVC is 0x13. Bit 7 (I) = 1 (disable IRQ) Bit 6
  // (F) = 1 (disable FIQ)
  cpsr = 0x13 | 0x80 | 0x40;
}

void CPU::step() {
  uint32_t instruction = fetch();
  decode(instruction);
  execute(instruction);
}

uint32_t CPU::fetch() {
  // Stub: return 0 for now.
  // In Thumb mode, fetch 16 bits; in ARM mode, fetch 32 bits.
  return 0;
}

void CPU::decode(uint32_t instruction) {
  // Stub
  (void)instruction;
}

void CPU::execute(uint32_t instruction) {
  // Stub
  (void)instruction;
}

CPUMode CPU::getMode() const { return static_cast<CPUMode>(cpsr & 0x1F); }

bool CPU::isThumb() const {
  return (cpsr & 0x20) != 0; // Check Bit 5 (T bit)
}

void CPU::setCPSR(uint32_t value) {
  CPUMode oldMode = getMode();
  CPUMode newMode = static_cast<CPUMode>(value & 0x1F);

  if (oldMode != newMode) {
    switchMode(oldMode, newMode);
  }

  cpsr = value;
}

uint32_t CPU::getSPSR() const {
  CPUMode mode = getMode();
  int index = getBankIndex(mode);
  if (index == 0) {
    // User/System modes do not have an SPSR
    return cpsr;
  }
  return banked_spsr[index];
}

void CPU::setSPSR(uint32_t value) {
  CPUMode mode = getMode();
  int index = getBankIndex(mode);
  if (index != 0) {
    banked_spsr[index] = value;
  }
}

void CPU::switchMode(CPUMode oldMode, CPUMode newMode) {
  int oldIndex = getBankIndex(oldMode);
  int newIndex = getBankIndex(newMode);

  // Save active SP (R13) and LR (R14) to the old mode's bank
  banked_r13[oldIndex] = r[13];
  banked_r14[oldIndex] = r[14];

  // Load new SP (R13) and LR (R14) from the new mode's bank
  r[13] = banked_r13[newIndex];
  r[14] = banked_r14[newIndex];

  // Handle FIQ specific banked R8-R12 registers
  if (oldMode == CPUMode::FIQ) {
    // Save FIQ specific registers
    for (int i = 0; i < 5; ++i) {
      banked_r8_r12_fiq[i] = r[8 + i];
    }
    // Restore standard User registers for R8-R12
    for (int i = 0; i < 5; ++i) {
      r[8 + i] = r8_r12_usr[i];
    }
  } else if (newMode == CPUMode::FIQ) {
    // Save standard registers
    for (int i = 0; i < 5; ++i) {
      r8_r12_usr[i] = r[8 + i];
    }
    // Restore FIQ specific registers
    for (int i = 0; i < 5; ++i) {
      r[8 + i] = banked_r8_r12_fiq[i];
    }
  }
}
