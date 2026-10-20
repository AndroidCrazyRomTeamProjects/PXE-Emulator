#include "cpu.h"
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace PXE {

CPU::CPU(uint8_t* mem, uint32_t size, uint32_t trapBaseAddr)
    : memory(mem), memorySize(size), cpsr(0x00000013), halted(false), cycles(0), trapBase(trapBaseAddr) {
    reset();
}

void CPU::reset() {
    std::memset(r, 0, sizeof(r));
    cpsr = 0x00000013; // SVC mode
    halted = false;
    cycles = 0;
}

void CPU::badAccess(uint32_t addr, bool write, const char* kind) {
    std::ostringstream ss;
    ss << "Bad " << kind << " " << (write ? "write" : "read") << " @ 0x"
       << std::hex << addr << " PC=0x" << getPC() << " T=" << getT();
    if (onFault) {
        onFault(ss.str());
    } else {
        std::cerr << ss.str() << std::endl;
    }
}

uint8_t CPU::r8(uint32_t addr) {
    if (addr >= memorySize) {
        badAccess(addr, false, "u8");
        return 0;
    }
    return memory[addr];
}

uint16_t CPU::r16(uint32_t addr) {
    if (addr + 1 >= memorySize) {
        badAccess(addr, false, "u16");
        return 0;
    }
    return static_cast<uint16_t>(memory[addr]) | (static_cast<uint16_t>(memory[addr + 1]) << 8);
}

uint32_t CPU::r32(uint32_t addr) {
    if (addr + 3 >= memorySize) {
        badAccess(addr, false, "u32");
        return 0;
    }
    if (addr & 3) { // Unaligned rotated load
        uint32_t aligned = addr & ~3u;
        uint32_t val = static_cast<uint32_t>(memory[aligned]) |
                      (static_cast<uint32_t>(memory[aligned + 1]) << 8) |
                      (static_cast<uint32_t>(memory[aligned + 2]) << 16) |
                      (static_cast<uint32_t>(memory[aligned + 3]) << 24);
        uint32_t shift = (addr & 3u) * 8;
        return (val >> shift) | (val << (32 - shift));
    }
    return static_cast<uint32_t>(memory[addr]) |
           (static_cast<uint32_t>(memory[addr + 1]) << 8) |
           (static_cast<uint32_t>(memory[addr + 2]) << 16) |
           (static_cast<uint32_t>(memory[addr + 3]) << 24);
}

void CPU::w8(uint32_t addr, uint8_t val) {
    if (addr >= memorySize) {
        badAccess(addr, true, "u8");
        return;
    }
    memory[addr] = val;
}

void CPU::w16(uint32_t addr, uint16_t val) {
    if (addr + 1 >= memorySize) {
        badAccess(addr, true, "u16");
        return;
    }
    memory[addr] = val & 0xFF;
    memory[addr + 1] = (val >> 8) & 0xFF;
}

void CPU::w32(uint32_t addr, uint32_t val) {
    if (addr + 3 >= memorySize) {
        badAccess(addr, true, "u32");
        return;
    }
    memory[addr] = val & 0xFF;
    memory[addr + 1] = (val >> 8) & 0xFF;
    memory[addr + 2] = (val >> 16) & 0xFF;
    memory[addr + 3] = (val >> 24) & 0xFF;
}

void CPU::updateFlagsNZ(uint32_t result) {
    setN((result & 0x80000000u) != 0);
    setZ(result == 0);
}

bool CPU::checkCondition(uint8_t cond) {
    bool N = getN();
    bool Z = getZ();
    bool C = getC();
    bool V = getV();

    switch (cond) {
        case 0x0: return Z;                     // EQ
        case 0x1: return !Z;                    // NE
        case 0x2: return C;                     // CS / HS
        case 0x3: return !C;                    // CC / LO
        case 0x4: return N;                     // MI
        case 0x5: return !N;                    // PL
        case 0x6: return V;                     // VS
        case 0x7: return !V;                    // VC
        case 0x8: return C && !Z;               // HI
        case 0x9: return !C || Z;               // LS
        case 0xA: return N == V;                // GE
        case 0xB: return N != V;                // LT
        case 0xC: return !Z && (N == V);        // GT
        case 0xD: return Z || (N != V);         // LE
        case 0xE: return true;                  // AL
        case 0xF: return true;                  // NV / Reserved
        default: return true;
    }
}

void CPU::step() {
    if (halted) return;

    uint32_t pc = getPC();

    // Check for HLE trap
    if (pc >= trapBase && onTrap) {
        if (onTrap(*this, pc)) {
            cycles++;
            return;
        }
    }

    if (getT()) {
        stepThumb();
    } else {
        stepARM();
    }
    cycles++;
}

void CPU::run(uint32_t maxCycles) {
    uint32_t executed = 0;
    while (!halted && executed < maxCycles) {
        step();
        executed++;
    }
}

void CPU::stepThumb() {
    uint32_t pc = getPC();
    uint16_t ins = r16(pc);
    r[15] = pc + 2; // Advance PC before executing

    // Format 1: Shift by immediate (LSL, LSR, ASR)
    if ((ins & 0xE000) == 0x0000) {
        uint8_t op = (ins >> 11) & 0x3;
        uint8_t offset = (ins >> 6) & 0x1F;
        uint8_t rs = (ins >> 3) & 0x7;
        uint8_t rd = ins & 0x7;

        if (op == 0) { // LSL
            uint32_t val = r[rs];
            if (offset > 0) {
                setC((val & (1u << (32 - offset))) != 0);
                val <<= offset;
            }
            r[rd] = val;
            updateFlagsNZ(val);
        } else if (op == 1) { // LSR
            uint32_t val = r[rs];
            if (offset == 0) offset = 32;
            setC((val & (1u << (offset - 1))) != 0);
            val = (offset >= 32) ? 0 : (val >> offset);
            r[rd] = val;
            updateFlagsNZ(val);
        } else if (op == 2) { // ASR
            int32_t val = static_cast<int32_t>(r[rs]);
            if (offset == 0) offset = 32;
            setC((val & (1 << (offset - 1))) != 0);
            val = (offset >= 32) ? (val < 0 ? -1 : 0) : (val >> offset);
            r[rd] = static_cast<uint32_t>(val);
            updateFlagsNZ(r[rd]);
        } else { // Format 2: Add/subtract
            uint8_t i = (ins >> 10) & 1;
            uint8_t sub = (ins >> 9) & 1;
            uint8_t rn_imm = (ins >> 6) & 0x7;
            uint32_t op2 = i ? rn_imm : r[rn_imm];
            uint32_t op1 = r[rs];
            uint32_t res = sub ? (op1 - op2) : (op1 + op2);
            r[rd] = res;
            updateFlagsNZ(res);
        }
        return;
    }

    // Format 3: Move/compare/add/subtract immediate
    if ((ins & 0xE000) == 0x2000) {
        uint8_t op = (ins >> 11) & 0x3;
        uint8_t rd = (ins >> 8) & 0x7;
        uint32_t imm = ins & 0xFF;
        uint32_t val = r[rd];

        if (op == 0) { // MOV
            r[rd] = imm;
            updateFlagsNZ(imm);
        } else if (op == 1) { // CMP
            uint32_t res = val - imm;
            updateFlagsNZ(res);
            setC(val >= imm);
        } else if (op == 2) { // ADD
            uint32_t res = val + imm;
            r[rd] = res;
            updateFlagsNZ(res);
        } else if (op == 3) { // SUB
            uint32_t res = val - imm;
            r[rd] = res;
            updateFlagsNZ(res);
            setC(val >= imm);
        }
        return;
    }

    // Format 5: High register operations / branch exchange (BX / BLX)
    if ((ins & 0xFC00) == 0x4400) {
        uint8_t op = (ins >> 8) & 0x3;
        uint8_t h1 = (ins >> 7) & 1;
        uint8_t h2 = (ins >> 6) & 1;
        uint8_t rs = (h2 << 3) | ((ins >> 3) & 0x7);
        uint8_t rd = (h1 << 3) | (ins & 0x7);

        if (op == 0) { // ADD
            r[rd] += r[rs];
            if (rd == 15) r[15] &= ~1u;
        } else if (op == 1) { // CMP
            uint32_t res = r[rd] - r[rs];
            updateFlagsNZ(res);
            setC(r[rd] >= r[rs]);
        } else if (op == 2) { // MOV
            r[rd] = r[rs];
            if (rd == 15) r[15] &= ~1u;
        } else if (op == 3) { // BX / BLX
            uint32_t target = r[rs];
            setT((target & 1) != 0);
            setPC(target & ~1u);
        }
        return;
    }

    // Format 9: Load/store with immediate offset
    if ((ins & 0xE000) == 0x6000) {
        uint8_t B = (ins >> 12) & 1;
        uint8_t L = (ins >> 11) & 1;
        uint8_t offset = (ins >> 6) & 0x1F;
        uint8_t rb = (ins >> 3) & 0x7;
        uint8_t rd = ins & 0x7;

        uint32_t addr = r[rb] + (B ? offset : (offset << 2));
        if (L) {
            r[rd] = B ? r8(addr) : r32(addr);
        } else {
            if (B) w8(addr, static_cast<uint8_t>(r[rd]));
            else w32(addr, r[rd]);
        }
        return;
    }

    // Format 11: SP-relative load/store
    if ((ins & 0xF000) == 0x9000) {
        uint8_t L = (ins >> 11) & 1;
        uint8_t rd = (ins >> 8) & 0x7;
        uint32_t imm = (ins & 0xFF) << 2;
        uint32_t addr = getSP() + imm;

        if (L) r[rd] = r32(addr);
        else w32(addr, r[rd]);
        return;
    }

    // Format 14: Push/Pop registers
    if ((ins & 0xF000) == 0xB000) {
        if ((ins & 0x0600) == 0x0400) { // PUSH/POP
            uint8_t L = (ins >> 11) & 1;
            uint8_t Rbit = (ins >> 8) & 1;
            uint8_t rlist = ins & 0xFF;

            if (L) { // POP
                uint32_t sp = getSP();
                for (int i = 0; i < 8; ++i) {
                    if (rlist & (1 << i)) {
                        r[i] = r32(sp);
                        sp += 4;
                    }
                }
                if (Rbit) {
                    uint32_t pcVal = r32(sp);
                    sp += 4;
                    setT((pcVal & 1) != 0);
                    setPC(pcVal & ~1u);
                }
                setSP(sp);
            } else { // PUSH
                uint32_t count = 0;
                for (int i = 0; i < 8; ++i) if (rlist & (1 << i)) count++;
                if (Rbit) count++;

                uint32_t sp = getSP() - (count * 4);
                setSP(sp);
                for (int i = 0; i < 8; ++i) {
                    if (rlist & (1 << i)) {
                        w32(sp, r[i]);
                        sp += 4;
                    }
                }
                if (Rbit) {
                    w32(sp, getLR());
                }
            }
            return;
        }
    }

    // Format 16: Conditional branch
    if ((ins & 0xF000) == 0xD000) {
        uint8_t cond = (ins >> 8) & 0xF;
        if (cond == 0xF) { // SWI
            if (onSwi) onSwi(*this, ins & 0xFF);
            return;
        }
        if (checkCondition(cond)) {
            int8_t offset = static_cast<int8_t>(ins & 0xFF);
            int32_t disp = static_cast<int32_t>(offset) * 2;
            setPC(pc + 4 + disp);
        }
        return;
    }

    // Format 18: Unconditional branch
    if ((ins & 0xF800) == 0xE000) {
        int16_t offset = ins & 0x7FF;
        if (offset & 0x400) offset |= 0xF800; // Sign extend
        int32_t disp = static_cast<int32_t>(offset) * 2;
        setPC(pc + 4 + disp);
        return;
    }

    // Format 19: Long branch with link (BL / BLX)
    if ((ins & 0xF000) == 0xF000) {
        uint8_t H = (ins >> 11) & 1;
        uint32_t offset = ins & 0x7FF;

        if (H == 0) { // First instruction of BL
            if (offset & 0x400) offset |= 0xFFF80000; // Sign extend
            setLR(pc + 4 + (offset << 12));
        } else { // Second instruction of BL
            uint32_t nextPC = pc + 2;
            uint32_t target = getLR() + (offset << 1);
            setLR(nextPC | 1);
            setPC(target & ~1u);
        }
        return;
    }
}

void CPU::stepARM() {
    uint32_t pc = getPC();
    uint32_t ins = r32(pc);
    r[15] = pc + 4; // Advance PC before executing

    uint8_t cond = (ins >> 28) & 0xF;
    if (!checkCondition(cond)) return;

    // Software Interrupt (SWI)
    if ((ins & 0x0F000000) == 0x0F000000) {
        if (onSwi) onSwi(*this, ins & 0x00FFFFFF);
        return;
    }

    // Branch / Branch with Link (B / BL / BLX)
    if ((ins & 0x0E000000) == 0x0A000000) {
        bool L = (ins & (1 << 24)) != 0;
        int32_t imm24 = ins & 0x00FFFFFF;
        if (imm24 & 0x00800000) imm24 |= 0xFF000000; // Sign extend
        int32_t target = pc + 8 + (imm24 << 2);

        if (L) setLR(pc + 4);
        setPC(target);
        return;
    }

    // Branch Exchange (BX)
    if ((ins & 0x0FFFFFF0) == 0x012FFF10) {
        uint8_t rm = ins & 0xF;
        uint32_t target = r[rm];
        setT((target & 1) != 0);
        setPC(target & ~1u);
        return;
    }

    // Data processing / MOV / ADD / SUB / CMP
    if ((ins & 0x0C000000) == 0x00000000) {
        bool I = (ins & (1 << 25)) != 0;
        uint8_t opcode = (ins >> 21) & 0xF;
        bool S = (ins & (1 << 20)) != 0;
        uint8_t rn = (ins >> 16) & 0xF;
        uint8_t rd = (ins >> 12) & 0xF;

        uint32_t op2 = 0;
        if (I) { // Immediate
            uint32_t imm = ins & 0xFF;
            uint8_t rot = ((ins >> 8) & 0xF) * 2;
            op2 = (imm >> rot) | (imm << (32 - rot));
        } else {
            op2 = r[ins & 0xF];
        }

        uint32_t res = 0;
        uint32_t op1 = r[rn];

        switch (opcode) {
            case 0x0: res = op1 & op2; break;         // AND
            case 0x1: res = op1 ^ op2; break;         // EOR
            case 0x2: res = op1 - op2; break;         // SUB
            case 0x4: res = op1 + op2; break;         // ADD
            case 0xA: res = op1 - op2; break;         // CMP
            case 0xC: res = op1 | op2; break;         // ORR
            case 0xD: res = op2; break;               // MOV
            case 0xE: res = op1 & ~op2; break;        // BIC
            case 0xF: res = ~op2; break;              // MVN
            default: res = op2; break;
        }

        if (opcode != 0xA && rd != 15) {
            r[rd] = res;
        }

        if (S) {
            updateFlagsNZ(res);
            if (opcode == 0x2 || opcode == 0xA) { // SUB / CMP
                setC(op1 >= op2);
            }
        }
        return;
    }

    // Single Data Transfer (LDR / STR)
    if ((ins & 0x0C000000) == 0x04000000) {
        bool I = (ins & (1 << 25)) != 0;
        bool P = (ins & (1 << 24)) != 0;
        bool U = (ins & (1 << 23)) != 0;
        bool B = (ins & (1 << 22)) != 0;
        bool L = (ins & (1 << 20)) != 0;
        uint8_t rn = (ins >> 16) & 0xF;
        uint8_t rd = (ins >> 12) & 0xF;

        uint32_t offset = I ? r[ins & 0xF] : (ins & 0xFFF);
        uint32_t addr = r[rn];

        if (P) addr = U ? (addr + offset) : (addr - offset);

        if (L) {
            r[rd] = B ? r8(addr) : r32(addr);
        } else {
            if (B) w8(addr, static_cast<uint8_t>(r[rd]));
            else w32(addr, r[rd]);
        }
        return;
    }
}

} // namespace PXE
