cmake_minimum_required(VERSION 3.12)
project(PXE_Emulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/PXE Emulator")

set(PXE_CORE_SOURCES
    "PXE Emulator/cpu.cpp"
    "PXE Emulator/lglz.cpp"
    "PXE Emulator/pxo_loader.cpp"
    "PXE Emulator/fls_loader.cpp"
    "PXE Emulator/lg_api.cpp"
    "PXE Emulator/pxe_engine.cpp"
)

add_library(pxe_core STATIC ${PXE_CORE_SOURCES})

# Test suite executable
add_executable(test_pxe tests/test_pxe.cpp)
target_link_libraries(test_pxe PRIVATE pxe_core)

# On Windows, build the GUI app as well
if(WIN32)
    add_executable(PXE_Emulator WIN32
        "PXE Emulator/PXE Emulator.cpp"
        "PXE Emulator/PXE Emulator.rc"
    )
    target_link_libraries(PXE_Emulator PRIVATE pxe_core comdlg32)
endif()
