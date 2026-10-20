#include "lg_api.h"
#include "pxe_engine.h"
#include <iostream>
#include <cstring>

namespace PXE {

LGAPIHost::LGAPIHost(PXEEngine* eng)
    : engine(eng), heapPtr(PXE_HEAP_BASE) {}

void LGAPIHost::initializeAPITables() {
    if (!engine) return;

    // Build master API table at PXE_API_BASE
    for (uint32_t sub = 0; sub < 16; ++sub) {
        uint32_t subTableAddr = PXE_SUB_BASE + (sub * 0x400);
        engine->cpu->w32(PXE_API_BASE + (sub * 4), subTableAddr);

        for (uint32_t fn = 0; fn < 256; ++fn) {
            uint32_t trapAddr = PXE_TRAP_BASE + ((sub * 256 + fn) * 4);
            engine->cpu->w32(subTableAddr + (fn * 4), trapAddr);
        }
    }
}

uint32_t LGAPIHost::sysMalloc(uint32_t size) {
    if (size == 0) return 0;
    // Align to 8 bytes
    uint32_t aligned = (size + 7) & ~7u;
    if (heapPtr + aligned >= PXE_HEAP_END) {
        std::cerr << "LGAPIHost: Out of heap memory allocating " << size << " bytes" << std::endl;
        return 0;
    }
    uint32_t addr = heapPtr;
    heapPtr += aligned;
    heapAllocations[addr] = aligned;
    return addr;
}

void LGAPIHost::sysFree(uint32_t ptr) {
    if (ptr == 0) return;
    heapAllocations.erase(ptr);
}

uint32_t LGAPIHost::sysRealloc(uint32_t ptr, uint32_t newSize) {
    if (ptr == 0) return sysMalloc(newSize);
    if (newSize == 0) { sysFree(ptr); return 0; }

    uint32_t newAddr = sysMalloc(newSize);
    if (newAddr != 0 && engine) {
        auto it = heapAllocations.find(ptr);
        uint32_t oldSize = (it != heapAllocations.end()) ? it->second : newSize;
        uint32_t copyLen = (oldSize < newSize) ? oldSize : newSize;
        std::memcpy(&engine->ram[newAddr], &engine->ram[ptr], copyLen);
        sysFree(ptr);
    }
    return newAddr;
}

bool LGAPIHost::handleTrap(uint32_t trapPC) {
    if (trapPC < PXE_TRAP_BASE) return false;

    uint32_t slot = (trapPC - PXE_TRAP_BASE) / 4;
    uint32_t sub = slot / 256;
    uint32_t fn = slot % 256;

    CPU& cpu = *engine->cpu;
    uint32_t arg0 = cpu.r[0];
    uint32_t arg1 = cpu.r[1];
    uint32_t arg2 = cpu.r[2];
    (void)arg2;

    uint32_t retVal = 0;

    // Sub 0: Core / Memory API
    if (sub == 0) {
        switch (fn) {
            case 0: retVal = sysMalloc(arg0); break;
            case 1: sysFree(arg0); retVal = 0; break;
            case 2: retVal = sysRealloc(arg0, arg1); break;
            default: retVal = 0; break;
        }
    }
    // Sub 1: Graphics API
    else if (sub == 1) {
        switch (fn) {
            case 0: // GetScreenResolution
                if (arg0) cpu.w32(arg0, SCREEN_WIDTH);
                if (arg1) cpu.w32(arg1, SCREEN_HEIGHT);
                retVal = 1;
                break;
            case 1: // FlushFrameBuffer
                retVal = 0;
                break;
            default: retVal = 0; break;
        }
    }
    // Sub 2: System / Timer / Random
    else if (sub == 2) {
        switch (fn) {
            case 0: retVal = 1000; break; // GetTickCount
            case 1: retVal = rand(); break;
            default: retVal = 0; break;
        }
    }

    cpu.r[0] = retVal;

    // Return from HLE trap using LR
    uint32_t lr = cpu.getLR();
    cpu.setT((lr & 1) != 0);
    cpu.setPC(lr & ~1u);

    return true;
}

} // namespace PXE
