#ifndef PXE_ENGINE_H
#define PXE_ENGINE_H

#include "cpu.h"
#include "pxo_loader.h"
#include "fls_loader.h"
#include "lg_api.h"
#include <vector>
#include <memory>
#include <string>

namespace PXE {

constexpr uint32_t PXE_MEM_SIZE  = 0x04000000; // 64 MB
constexpr uint32_t PXE_HEAP_BASE = 0x02000000;
constexpr uint32_t PXE_HEAP_END  = 0x03800000;
constexpr uint32_t PXE_STACK_TOP = 0x03EFFF00;
constexpr uint32_t PXE_API_BASE  = 0x03F00000;
constexpr uint32_t PXE_SUB_BASE  = 0x03F01000;
constexpr uint32_t PXE_TRAP_BASE = 0x03F80000;
constexpr uint32_t PXE_RET_TRAP  = 0x03FFFFF0;

constexpr int SCREEN_WIDTH  = 240;
constexpr int SCREEN_HEIGHT = 400;

class PXEEngine {
public:
    std::vector<uint8_t> ram;
    std::unique_ptr<CPU> cpu;
    std::unique_ptr<LGAPIHost> apiHost;

    PXOHeader pxoHeader;
    FLSImageInfo flsInfo;
    bool isLoaded;
    bool isFLSMode;

    std::vector<uint32_t> frameBuffer; // 240x400 32-bit RGBA screen buffer

    PXEEngine();
    ~PXEEngine() = default;

    void reset();

    bool loadPXOFile(const std::vector<uint8_t>& fileBytes);
    bool loadFLSFile(const std::vector<uint8_t>& fileBytes);

    void step(uint32_t maxCycles = 10000);
    void sendTouchEvent(int x, int y, int action); // action: 0 = down, 1 = move, 2 = up

    uint32_t getPC() const { return cpu ? cpu->getPC() : 0; }
    bool isHalted() const { return cpu ? cpu->halted : true; }
};

} // namespace PXE

#endif // PXE_ENGINE_H
