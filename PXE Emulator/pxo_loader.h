#ifndef PXE_PXO_LOADER_H
#define PXE_PXO_LOADER_H

#include <vector>
#include <cstdint>
#include <string>

namespace PXE {

struct PXOHeader {
    uint32_t imageOffset;
    uint32_t roSize;
    uint32_t rwBase;
    uint32_t rwEnd;
    uint32_t ziBase;
    uint32_t ziEnd;
    uint32_t initFunc;
    uint32_t eventFunc;
};

class PXOLoader {
public:
    static bool isPXO(const std::vector<uint8_t>& data);

    static bool loadPXO(
        const std::vector<uint8_t>& fileBytes,
        uint8_t* memory,
        uint32_t memorySize,
        PXOHeader& outHeader
    );
};

} // namespace PXE

#endif // PXE_PXO_LOADER_H
