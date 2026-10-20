#include "cpu.h"
#include "lglz.h"
#include "pxo_loader.h"
#include "fls_loader.h"
#include "pxe_engine.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

void test_cpu_basic() {
    std::cout << "[TEST] CPU Basic Instructions..." << std::endl;
    std::vector<uint8_t> mem(0x100000, 0);
    PXE::CPU cpu(mem.data(), static_cast<uint32_t>(mem.size()));

    // ARM instruction: MOV R0, #42 -> 0xE3A0002A
    cpu.w32(0, 0xE3A0002A);
    // ARM instruction: ADD R1, R0, #10 -> 0xE280100A
    cpu.w32(4, 0xE280100A);

    cpu.setPC(0);
    cpu.step(); // MOV R0, 42
    assert(cpu.r[0] == 42);

    cpu.step(); // ADD R1, R0, 10
    assert(cpu.r[1] == 52);

    std::cout << "  -> PASSED" << std::endl;
}

void test_fls_loader() {
    std::cout << "[TEST] FLS Kernel Loader Placeholder..." << std::endl;
    std::vector<uint8_t> mem(0x100000, 0);

    // Construct dummy FLS Container
    PXE::FLSHeader hdr = {};
    hdr.magic[0] = 'F'; hdr.magic[1] = 'L'; hdr.magic[2] = 'S'; hdr.magic[3] = '1';
    hdr.version = 1;
    hdr.numBlocks = 1;
    hdr.entryPoint = 0x00001000;

    PXE::FLSBlockHeader block = {};
    block.blockType = PXE::FLS_BLOCK_CODE_KERNEL;
    block.flashAddress = 0x00001000;
    block.dataOffset = sizeof(PXE::FLSHeader) + sizeof(PXE::FLSBlockHeader);
    block.dataSize = 8;
    block.memSize = 8;

    std::vector<uint8_t> fileBytes(sizeof(PXE::FLSHeader) + sizeof(PXE::FLSBlockHeader) + 8, 0);
    std::memcpy(fileBytes.data(), &hdr, sizeof(hdr));
    std::memcpy(fileBytes.data() + sizeof(hdr), &block, sizeof(block));

    // Dummy kernel instructions at data offset: MOV R0, #99
    uint32_t movInst = 0xE3A00063;
    std::memcpy(fileBytes.data() + block.dataOffset, &movInst, 4);

    PXE::FLSImageInfo info;
    bool loaded = PXE::FLSLoader::loadFLS(fileBytes, mem.data(), static_cast<uint32_t>(mem.size()), info);
    assert(loaded);
    assert(info.isFLSFormat == true);
    assert(info.entryPoint == 0x00001000);

    // Verify memory was mapped
    uint32_t loadedWord = static_cast<uint32_t>(mem[0x1000]) |
                         (static_cast<uint32_t>(mem[0x1001]) << 8) |
                         (static_cast<uint32_t>(mem[0x1002]) << 16) |
                         (static_cast<uint32_t>(mem[0x1003]) << 24);
    assert(loadedWord == movInst);

    std::cout << "  -> PASSED" << std::endl;
}

void test_pxe_engine_fls() {
    std::cout << "[TEST] PXE Engine FLS Mode..." << std::endl;
    PXE::PXEEngine engine;

    std::vector<uint8_t> rawKernel = { 0x2A, 0x00, 0xA0, 0xE3 }; // MOV R0, #42
    bool ok = engine.loadFLSFile(rawKernel);
    assert(ok);
    assert(engine.isFLSMode);
    assert(engine.getPC() == 0);

    engine.step(1);
    assert(engine.cpu->r[0] == 42);

    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running PXE Emulator Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    test_cpu_basic();
    test_fls_loader();
    test_pxe_engine_fls();

    std::cout << "All tests completed successfully!" << std::endl;
    return 0;
}
