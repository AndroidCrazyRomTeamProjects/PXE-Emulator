#include "fls_loader.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace PXE {

bool FLSLoader::isFLS(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(FLSHeader)) return false;

    // Check common FLS signatures: FLS1, FLSH, FL20, FL10, or generic FLS
    if ((data[0] == 'F' && data[1] == 'L' && data[2] == 'S') ||
        (data[0] == 'F' && data[1] == 'L' && data[2] == '1' && data[3] == '0') ||
        (data[0] == 'F' && data[1] == 'L' && data[2] == '2' && data[3] == '0')) {
        return true;
    }

    return false;
}

bool FLSLoader::loadFLS(
    const std::vector<uint8_t>& fileBytes,
    uint8_t* memory,
    uint32_t memorySize,
    FLSImageInfo& outInfo
) {
    outInfo = FLSImageInfo{};

    if (fileBytes.empty()) {
        std::cerr << "FLSLoader: empty file bytes" << std::endl;
        return false;
    }

    // Check if raw binary or container FLS format
    if (!isFLS(fileBytes)) {
        // Fallback placeholder loader for raw kernel binary in FLS format without header
        std::cout << "FLSLoader: No standard FLS header found, using raw kernel fallback loader." << std::endl;
        uint32_t loadAddr = 0x00000000;
        uint32_t sizeToCopy = static_cast<uint32_t>(fileBytes.size());
        if (loadAddr + sizeToCopy > memorySize) {
            sizeToCopy = memorySize - loadAddr;
        }
        std::memcpy(&memory[loadAddr], fileBytes.data(), sizeToCopy);

        outInfo.isFLSFormat = false;
        outInfo.entryPoint = loadAddr;
        outInfo.stackPointer = 0x03EFFF00;
        outInfo.codeLoadAddr = loadAddr;
        outInfo.totalLoadedBytes = sizeToCopy;
        outInfo.description = "Raw FLS Kernel / Executable Binary Image";
        return true;
    }

    const FLSHeader* hdr = reinterpret_cast<const FLSHeader*>(fileBytes.data());
    outInfo.isFLSFormat = true;
    outInfo.version = hdr->version;
    outInfo.entryPoint = hdr->entryPoint;
    outInfo.stackPointer = 0x03EFFF00;

    size_t blockTableOff = sizeof(FLSHeader);
    size_t requiredHeaderSize = blockTableOff + (hdr->numBlocks * sizeof(FLSBlockHeader));

    if (fileBytes.size() < requiredHeaderSize) {
        std::cerr << "FLSLoader: file too small for declared " << hdr->numBlocks << " blocks." << std::endl;
        return false;
    }

    uint32_t totalBytes = 0;
    const FLSBlockHeader* blockHeaders = reinterpret_cast<const FLSBlockHeader*>(fileBytes.data() + blockTableOff);

    for (uint16_t i = 0; i < hdr->numBlocks; ++i) {
        const FLSBlockHeader& block = blockHeaders[i];
        outInfo.blocks.push_back(block);

        if (block.dataOffset + block.dataSize > fileBytes.size()) {
            std::cerr << "FLSLoader: block " << i << " offset out of file range" << std::endl;
            continue;
        }

        if (block.flashAddress + block.memSize > memorySize) {
            std::cerr << "FLSLoader: block " << i << " address 0x" << std::hex << block.flashAddress
                      << " exceeds emulator RAM size." << std::endl;
            continue;
        }

        // Copy section payload into memory
        std::memcpy(&memory[block.flashAddress], &fileBytes[block.dataOffset], block.dataSize);

        // Zero-fill remaining allocation if memSize > dataSize
        if (block.memSize > block.dataSize) {
            std::memset(&memory[block.flashAddress + block.dataSize], 0, block.memSize - block.dataSize);
        }

        if (i == 0) outInfo.codeLoadAddr = block.flashAddress;
        totalBytes += block.memSize;
    }

    outInfo.totalLoadedBytes = totalBytes;
    std::ostringstream ss;
    ss << "FLS Container Kernel (v" << hdr->version << ", " << hdr->numBlocks << " blocks)";
    outInfo.description = ss.str();

    std::cout << "FLSLoader: Successfully loaded " << outInfo.description
              << " EntryPoint: 0x" << std::hex << outInfo.entryPoint << std::endl;

    return true;
}

bool FLSLoader::loadKernel(
    const std::vector<uint8_t>& kernelBytes,
    uint8_t* memory,
    uint32_t memorySize,
    uint32_t& outEntryPoint,
    uint32_t loadAddress
) {
    FLSImageInfo info;
    bool success = loadFLS(kernelBytes, memory, memorySize, info);
    if (success) {
        outEntryPoint = (info.entryPoint != 0) ? info.entryPoint : loadAddress;
    }
    return success;
}

bool FLSLoader::loadExecutable(
    const std::vector<uint8_t>& execBytes,
    uint8_t* memory,
    uint32_t memorySize,
    uint32_t& outEntryPoint,
    uint32_t loadAddress
) {
    FLSImageInfo info;
    bool success = loadFLS(execBytes, memory, memorySize, info);
    if (success) {
        outEntryPoint = (info.entryPoint != 0) ? info.entryPoint : loadAddress;
    }
    return success;
}

} // namespace PXE
