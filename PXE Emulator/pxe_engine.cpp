#include "pxe_engine.h"
#include <iostream>
#include <cstring>

namespace PXE {

PXEEngine::PXEEngine()
    : isLoaded(false), isFLSMode(false) {
    ram.resize(PXE_MEM_SIZE, 0);
    frameBuffer.resize(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFF000000); // Black screen default

    cpu = std::make_unique<CPU>(ram.data(), PXE_MEM_SIZE, PXE_TRAP_BASE);
    apiHost = std::make_unique<LGAPIHost>(this);

    cpu->onTrap = [this](CPU& /*cpu_ref*/, uint32_t pc) -> bool {
        return apiHost ? apiHost->handleTrap(pc) : false;
    };

    reset();
}

void PXEEngine::reset() {
    std::fill(ram.begin(), ram.end(), 0);
    std::fill(frameBuffer.begin(), frameBuffer.end(), 0xFF120E1F); // Dark background default

    if (cpu) {
        cpu->reset();
        cpu->setSP(PXE_STACK_TOP);
    }

    if (apiHost) {
        apiHost->initializeAPITables();
    }

    isLoaded = false;
    isFLSMode = false;
    pxoHeader = PXOHeader{};
    flsInfo = FLSImageInfo{};
}

bool PXEEngine::loadPXOFile(const std::vector<uint8_t>& fileBytes) {
    reset();

    if (!PXOLoader::loadPXO(fileBytes, ram.data(), PXE_MEM_SIZE, pxoHeader)) {
        std::cerr << "PXEEngine: Failed to load PXO file." << std::endl;
        return false;
    }

    isLoaded = true;
    isFLSMode = false;

    // Set initial execution state
    cpu->setSP(PXE_STACK_TOP);
    cpu->setLR(PXE_RET_TRAP);

    // Entry point: init function
    uint32_t entry = pxoHeader.initFunc;
    cpu->setT((entry & 1) != 0);
    cpu->setPC(entry & ~1u);

    std::cout << "PXEEngine: Loaded PXO game successfully. InitPC = 0x"
              << std::hex << entry << std::dec << std::endl;

    return true;
}

bool PXEEngine::loadFLSFile(const std::vector<uint8_t>& fileBytes) {
    reset();

    if (!FLSLoader::loadFLS(fileBytes, ram.data(), PXE_MEM_SIZE, flsInfo)) {
        std::cerr << "PXEEngine: Failed to load FLS kernel/executable file." << std::endl;
        return false;
    }

    isLoaded = true;
    isFLSMode = true;

    // Set initial execution state
    uint32_t sp = (flsInfo.stackPointer != 0) ? flsInfo.stackPointer : PXE_STACK_TOP;
    cpu->setSP(sp);
    cpu->setLR(PXE_RET_TRAP);

    uint32_t entry = flsInfo.entryPoint;
    cpu->setT((entry & 1) != 0);
    cpu->setPC(entry & ~1u);

    std::cout << "PXEEngine: Loaded FLS Kernel/Executable successfully. EntryPC = 0x"
              << std::hex << entry << std::dec << std::endl;

    return true;
}

void PXEEngine::step(uint32_t maxCycles) {
    if (!isLoaded || !cpu || cpu->halted) return;
    cpu->run(maxCycles);
}

void PXEEngine::sendTouchEvent(int x, int y, int action) {
    if (!isLoaded || !cpu) return;

    // Dispatch touch event parameters
    cpu->r[0] = static_cast<uint32_t>(action);
    cpu->r[1] = static_cast<uint32_t>(x);
    cpu->r[2] = static_cast<uint32_t>(y);

    if (!isFLSMode && pxoHeader.eventFunc != 0) {
        uint32_t eventPC = pxoHeader.eventFunc;
        cpu->setT((eventPC & 1) != 0);
        cpu->setPC(eventPC & ~1u);
    }
}

} // namespace PXE
