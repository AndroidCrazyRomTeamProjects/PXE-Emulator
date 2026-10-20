#ifndef PXE_FLS_LOADER_H
#define PXE_FLS_LOADER_H

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>

namespace PXE {

#pragma pack(push, 1)
struct FLSHeader {
    char magic[4];          // "FLS1", "FLSH", "FL20", etc.
    uint16_t version;       // Header format version
    uint16_t numBlocks;     // Number of blocks/sections
    uint32_t entryPoint;    // Execution entry point address
    uint32_t flags;        // Format flags (e.g., compressed, arch)
    uint32_t checksum;     // Header CRC32/checksum
    uint8_t reserved[12];   // Reserved padding
};

enum FLSBlockType : uint32_t {
    FLS_BLOCK_CODE_KERNEL = 0,
    FLS_BLOCK_DATA = 1,
    FLS_BLOCK_BOOTLOADER = 2,
    FLS_BLOCK_NVRAM_FS = 3
};

struct FLSBlockHeader {
    uint32_t blockType;     // FLSBlockType
    uint32_t flashAddress;  // Load target address in memory/flash
    uint32_t dataOffset;    // File offset of payload
    uint32_t dataSize;      // Size in file
    uint32_t memSize;       // Allocation size in memory
    uint32_t checksum;      // Block checksum
};
#pragma pack(pop)

struct FLSImageInfo {
    bool isFLSFormat;
    uint16_t version;
    uint32_t entryPoint;
    uint32_t stackPointer;
    uint32_t codeLoadAddr;
    uint32_t totalLoadedBytes;
    std::vector<FLSBlockHeader> blocks;
    std::string description;
};

class FLSLoader {
public:
    // Check if byte stream is a valid FLS kernel or executable image
    static bool isFLS(const std::vector<uint8_t>& data);

    // Load full FLS container file into emulator memory
    static bool loadFLS(
        const std::vector<uint8_t>& fileBytes,
        uint8_t* memory,
        uint32_t memorySize,
        FLSImageInfo& outInfo
    );

    // Placeholder to load a real kernel image in FLS format
    static bool loadKernel(
        const std::vector<uint8_t>& kernelBytes,
        uint8_t* memory,
        uint32_t memorySize,
        uint32_t& outEntryPoint,
        uint32_t loadAddress = 0x00000000
    );

    // Placeholder to load an executable module in FLS format
    static bool loadExecutable(
        const std::vector<uint8_t>& execBytes,
        uint8_t* memory,
        uint32_t memorySize,
        uint32_t& outEntryPoint,
        uint32_t loadAddress = 0x00100000
    );
};

} // namespace PXE

#endif // PXE_FLS_LOADER_H
