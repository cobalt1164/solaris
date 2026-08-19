#pragma once
#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

enum class CPUMode : uint8_t {
  User = 0x10,
  FIQ = 0x11,
  IRQ = 0x12,
  Supervisor = 0x13,
  Abort = 0x17,
  Undefined = 0x1B,
  System = 0x1F
};

class CPU {
public:
  CPU();
  ~CPU() = default;

  // Run one CPU instruction cycle
  void step();

  // Reset CPU to power-on state
  void reset();

  // Register access
  uint32_t getReg(uint8_t index) const { return r[index & 0xF]; }
  void setReg(uint8_t index, uint32_t value) { r[index & 0xF] = value; }

  uint32_t getCPSR() const { return cpsr; }
  void setCPSR(uint32_t value);

  uint32_t getSPSR() const;
  void setSPSR(uint32_t value);

  CPUMode getMode() const;
  bool isThumb() const;

  // CPSR Flag Bit Definitions
  static constexpr uint32_t FLAG_N = 1u << 31;
  static constexpr uint32_t FLAG_Z = 1u << 30;
  static constexpr uint32_t FLAG_C = 1u << 29;
  static constexpr uint32_t FLAG_V = 1u << 28;
  static constexpr uint32_t FLAG_I = 1u << 7;
  static constexpr uint32_t FLAG_F = 1u << 6;
  static constexpr uint32_t FLAG_T = 1u << 5;

  bool getFlag(uint32_t flag) const { return (cpsr & flag) != 0; }
  void setFlag(uint32_t flag, bool condition) {
    if (condition)
      cpsr |= flag;
    else
      cpsr &= ~flag;
  }

  // Decoder entry points
  void executeARM(uint32_t opcode);
  void executeThumb(uint16_t opcode);

private:
  // Core pipeline functions
  uint32_t fetch();
  void decode(uint32_t instruction);
  void execute(uint32_t instruction);

  // Thumb Instruction Handlers (Inlined into executeThumb)
  ALWAYS_INLINE void executeThumbShift(uint16_t opcode);
  ALWAYS_INLINE void executeThumbAddSub(uint16_t opcode);
  ALWAYS_INLINE void executeThumbMove(uint16_t opcode);
  ALWAYS_INLINE void executeThumbALU(uint16_t opcode);

  // Swap banked registers when CPU mode changes
  void switchMode(CPUMode oldMode, CPUMode newMode);

  // Active registers: R0-R12, R13 (SP), R14 (LR), R15 (PC)
  uint32_t r[16];

  // Current Program Status Register
  uint32_t cpsr;

  // Banked Registers
  // Indexes correspond to:
  // 0 = User/System, 1 = FIQ, 2 = IRQ, 3 = Supervisor, 4 = Abort, 5 = Undefined
  uint32_t banked_r13[6];  // Stack pointer (SP)
  uint32_t banked_r14[6];  // Link register (LR)
  uint32_t banked_spsr[6]; // Saved Program Status Register (User/System does
                           // not have one)

  // FIQ has its own R8-R12 banked registers
  uint32_t banked_r8_r12_fiq[5];
  // User/System/others share the standard R8-R12
  uint32_t r8_r12_usr[5];
};
