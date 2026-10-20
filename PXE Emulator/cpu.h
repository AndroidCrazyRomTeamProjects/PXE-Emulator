#ifndef PXE_CPU_H
#define PXE_CPU_H

#include <cstdint>
#include <vector>
#include <functional>
#include <string>
#include <iostream>

namespace PXE {

enum CPSRFlags {
    FLAG_N = 1 << 31,
    FLAG_Z = 1 << 30,
    FLAG_C = 1 << 29,
    FLAG_V = 1 << 28,
    FLAG_Q = 1 << 27,
    FLAG_T = 1 << 5
};

class CPU {
public:
    uint8_t* memory;
    uint32_t memorySize;

    uint32_t r[16]; // R0-R15
    uint32_t cpsr;  // Current Program Status Register
    bool halted;
    uint64_t cycles;

    uint32_t trapBase;

    std::function<bool(CPU&, uint32_t)> onTrap;
    std::function<void(CPU&, uint32_t)> onSwi;
    std::function<void(const std::string&)> onFault;

    CPU(uint8_t* mem, uint32_t size, uint32_t trapBaseAddr = 0x03F80000);
    ~CPU() = default;

    void reset();

    // Registers access helpers
    uint32_t getPC() const { return r[15]; }
    void setPC(uint32_t pc) { r[15] = pc; }
    uint32_t getSP() const { return r[13]; }
    void setSP(uint32_t sp) { r[13] = sp; }
    uint32_t getLR() const { return r[14]; }
    void setLR(uint32_t lr) { r[14] = lr; }

    bool getT() const { return (cpsr & FLAG_T) != 0; }
    void setT(bool t) { if (t) cpsr |= FLAG_T; else cpsr &= ~FLAG_T; }

    bool getN() const { return (cpsr & FLAG_N) != 0; }
    void setN(bool n) { if (n) cpsr |= FLAG_N; else cpsr &= ~FLAG_N; }

    bool getZ() const { return (cpsr & FLAG_Z) != 0; }
    void setZ(bool z) { if (z) cpsr |= FLAG_Z; else cpsr &= ~FLAG_Z; }

    bool getC() const { return (cpsr & FLAG_C) != 0; }
    void setC(bool c) { if (c) cpsr |= FLAG_C; else cpsr &= ~FLAG_C; }

    bool getV() const { return (cpsr & FLAG_V) != 0; }
    void setV(bool v) { if (v) cpsr |= FLAG_V; else cpsr &= ~FLAG_V; }

    // Memory access
    uint8_t r8(uint32_t addr);
    uint16_t r16(uint32_t addr);
    uint32_t r32(uint32_t addr);

    void w8(uint32_t addr, uint8_t val);
    void w16(uint32_t addr, uint16_t val);
    void w32(uint32_t addr, uint32_t val);

    // Execution
    void step();
    void run(uint32_t maxCycles);

    // Helpers
    void updateFlagsNZ(uint32_t result);
    bool checkCondition(uint8_t cond);

private:
    void stepARM();
    void stepThumb();

    void badAccess(uint32_t addr, bool write, const char* kind);
};

} // namespace PXE

#endif // PXE_CPU_H
