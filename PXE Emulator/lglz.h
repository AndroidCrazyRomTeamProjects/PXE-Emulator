#ifndef PXE_LGLZ_H
#define PXE_LGLZ_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace PXE {

class LGLZ {
public:
    // Decompress an LGLZ stream (used by LG PXO/DMF files)
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const uint8_t* data, size_t size);

private:
    static std::vector<uint8_t> decodeBlock(const uint8_t* blockData, size_t blockSize);
};

} // namespace PXE

#endif // PXE_LGLZ_H
