#include "pxo_loader.h"
#include "lglz.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace PXE {

static uint32_t readU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
          (static_cast<uint32_t>(p[1]) << 8) |
          (static_cast<uint32_t>(p[2]) << 16) |
          (static_cast<uint32_t>(p[3]) << 24);
}

bool PXOLoader::isPXO(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return false;
    if (data[0] == 'P' && data[1] == 'X' && data[2] == 'O' && data[3] == '4') return true;
    // Try decompress check header
    try {
        std::vector<uint8_t> decomp = LGLZ::decompress(data);
        if (decomp.size() >= 4 && decomp[0] == 'P' && decomp[1] == 'X' && decomp[2] == 'O' && decomp[3] == '4') {
            return true;
        }
    } catch (...) {}
    return false;
}

bool PXOLoader::loadPXO(
    const std::vector<uint8_t>& fileBytes,
    uint8_t* memory,
    uint32_t memorySize,
    PXOHeader& outHeader
) {
    if (fileBytes.empty()) return false;

    std::vector<uint8_t> bytes = fileBytes;

    if (bytes.size() < 4 || !(bytes[0] == 'P' && bytes[1] == 'X' && bytes[2] == 'O' && bytes[3] == '4')) {
        try {
            bytes = LGLZ::decompress(fileBytes);
        } catch (const std::exception& e) {
            std::cerr << "Failed to uncompress PXO4 file: " << e.what() << std::endl;
            return false;
        }
    }

    if (bytes.size() < 16 || !(bytes[0] == 'P' && bytes[1] == 'X' && bytes[2] == 'O' && bytes[3] == '4')) {
        std::cerr << "Decoded file is not a valid LG PXO4 game" << std::endl;
        return false;
    }

    uint32_t imgOff = readU32(&bytes[0x0C]);
    if (imgOff + 32 > bytes.size()) {
        std::cerr << "PXO header image offset out of bounds" << std::endl;
        return false;
    }

    outHeader.imageOffset = imgOff;
    outHeader.roSize   = readU32(&bytes[imgOff + 4]);
    outHeader.rwBase   = readU32(&bytes[imgOff + 8]);
    outHeader.rwEnd    = readU32(&bytes[imgOff + 12]);
    outHeader.ziBase   = readU32(&bytes[imgOff + 16]);
    outHeader.ziEnd    = readU32(&bytes[imgOff + 20]);
    outHeader.initFunc = readU32(&bytes[imgOff + 24]);
    outHeader.eventFunc= readU32(&bytes[imgOff + 28]);

    // Validate memory boundaries
    if (outHeader.roSize > memorySize) return false;

    // Load RO section at offset 0
    if (imgOff + outHeader.roSize <= bytes.size()) {
        std::memcpy(memory, &bytes[imgOff], outHeader.roSize);
    }

    // Load RW section
    uint32_t rwSize = (outHeader.rwEnd > outHeader.rwBase) ? (outHeader.rwEnd - outHeader.rwBase) : 0;
    if (rwSize > 0 && (outHeader.rwBase + rwSize <= memorySize)) {
        size_t rwFileOff = imgOff + outHeader.roSize;
        if (rwFileOff + rwSize <= bytes.size()) {
            std::memcpy(&memory[outHeader.rwBase], &bytes[rwFileOff], rwSize);
        }
    }

    // Zero ZI (BSS) section
    if (outHeader.ziEnd > outHeader.ziBase && outHeader.ziEnd <= memorySize) {
        std::memset(&memory[outHeader.ziBase], 0, outHeader.ziEnd - outHeader.ziBase);
    }

    return true;
}

} // namespace PXE
