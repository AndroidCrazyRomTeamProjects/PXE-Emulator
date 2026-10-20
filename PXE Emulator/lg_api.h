#ifndef PXE_LG_API_H
#define PXE_LG_API_H

#include "cpu.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>

namespace PXE {

class PXEEngine;

class LGAPIHost {
public:
    PXEEngine* engine;

    LGAPIHost(PXEEngine* eng);
    ~LGAPIHost() = default;

    void initializeAPITables();
    bool handleTrap(uint32_t trapPC);

    // Common HLE API handlers
    uint32_t sysMalloc(uint32_t size);
    void sysFree(uint32_t ptr);
    uint32_t sysRealloc(uint32_t ptr, uint32_t newSize);

private:
    uint32_t heapPtr;
    std::unordered_map<uint32_t, uint32_t> heapAllocations;
};

} // namespace PXE

#endif // PXE_LG_API_H
