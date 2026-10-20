#include "lglz.h"
#include <algorithm>
#include <cstring>

namespace PXE {

struct BitReader {
    const uint8_t* data;
    size_t size;
    size_t pos;
    uint32_t control;

    BitReader(const uint8_t* d, size_t s) : data(d), size(s), pos(0), control(0x80000000u) {}

    uint8_t readByte() {
        if (pos >= size) {
            throw std::runtime_error("LGLZ: Truncated compressed data reading byte");
        }
        return data[pos++];
    }

    uint8_t getBit() {
        uint32_t carry = 0;
        if ((control & 0x7FFFFFFF) == 0) {
            carry = control >> 31;
            if (pos + 4 > size) {
                throw std::runtime_error("LGLZ: Truncated control word");
            }
            control = static_cast<uint32_t>(data[pos]) |
                     (static_cast<uint32_t>(data[pos + 1]) << 8) |
                     (static_cast<uint32_t>(data[pos + 2]) << 16) |
                     (static_cast<uint32_t>(data[pos + 3]) << 24);
            pos += 4;
        }
        uint8_t bit = static_cast<uint8_t>(control >> 31);
        control = ((control << 1) + carry) & 0xFFFFFFFFu;
        return bit;
    }
};

std::vector<uint8_t> LGLZ::decodeBlock(const uint8_t* blockData, size_t blockSize) {
    BitReader reader(blockData, blockSize);
    std::vector<uint8_t> output;
    output.reserve(8192);

    uint32_t lastOffset = 0;
    uint32_t currentDistance = 0;

    auto copyFromDistance = [&](uint32_t dist, uint32_t count) {
        if (dist == 0 || dist > output.size()) {
            throw std::runtime_error("LGLZ: Invalid back-reference distance");
        }
        for (uint32_t n = 0; n < count; ++n) {
            uint8_t b = output[output.size() - dist];
            output.push_back(b);
        }
    };

    while (true) {
        // Literal runs
        uint32_t run = 1;
        while (reader.getBit()) {
            for (uint32_t n = 0; n < run; ++n) {
                output.push_back(reader.readByte());
            }
            if (reader.getBit()) {
                for (uint32_t n = 0; n < run; ++n) {
                    output.push_back(reader.readByte());
                }
            }
            run *= 2;
        }

        // Back reference command
        uint32_t code = reader.readByte();
        if (code & 0x80) {
            code = ((code << 8) | reader.readByte()) & 0x7FFF;
            if (code == 0) {
                // End of block
                return output;
            }
            lastOffset = code;
            currentDistance = code;
            copyFromDistance(code, 3);
        } else if (code == 0) {
            currentDistance = lastOffset;
            copyFromDistance(currentDistance, 3);
        } else {
            if (code >= 0x10) {
                code = (code << 1) - 0x10;
            }
            currentDistance = code;
            copyFromDistance(code, 2);
        }

        // Repeat match run
        run = 1;
        while (reader.getBit()) {
            for (uint32_t n = 0; n < run; ++n) {
                copyFromDistance(currentDistance, 1);
            }
            if (reader.getBit()) {
                for (uint32_t n = 0; n < run; ++n) {
                    copyFromDistance(currentDistance, 1);
                }
            }
            run *= 2;
        }
    }
}

std::vector<uint8_t> LGLZ::decompress(const uint8_t* data, size_t size) {
    if (size < 4) {
        throw std::runtime_error("LGLZ: Input shorter than header size");
    }

    uint32_t expectedSize = static_cast<uint32_t>(data[0]) |
                           (static_cast<uint32_t>(data[1]) << 8) |
                           (static_cast<uint32_t>(data[2]) << 16) |
                           (static_cast<uint32_t>(data[3]) << 24);

    std::vector<uint8_t> result;
    result.reserve(expectedSize);

    const size_t blockPayloadSize = 8188;
    size_t start = 4;

    while (start < size && result.size() < expectedSize) {
        size_t end = std::min(start + blockPayloadSize, size);
        std::vector<uint8_t> chunk = decodeBlock(data + start, end - start);
        if (result.size() + chunk.size() > expectedSize) {
            throw std::runtime_error("LGLZ: Compressed data expands beyond header expected size");
        }
        result.insert(result.end(), chunk.begin(), chunk.end());
        start += blockPayloadSize;

        if (result.size() < expectedSize) {
            if (start + 4 > size) {
                throw std::runtime_error("LGLZ: Truncated block size header");
            }
            uint32_t blockSize = static_cast<uint32_t>(data[start]) |
                                (static_cast<uint32_t>(data[start + 1]) << 8) |
                                (static_cast<uint32_t>(data[start + 2]) << 16) |
                                (static_cast<uint32_t>(data[start + 3]) << 24);
            if (blockSize == 0) {
                throw std::runtime_error("LGLZ: Invalid compressed block size 0");
            }
            start += 4;
        }
    }

    if (result.size() != expectedSize) {
        throw std::runtime_error("LGLZ: Uncompressed size mismatch");
    }

    return result;
}

std::vector<uint8_t> LGLZ::decompress(const std::vector<uint8_t>& input) {
    return decompress(input.data(), input.size());
}

} // namespace PXE
