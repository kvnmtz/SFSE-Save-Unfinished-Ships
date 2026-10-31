cmake_minimum_required(VERSION 3.24)
project(SaveUnfinishedShips LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# static CRT so the DLL doesn't depend on a particular VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# Capstone disassembler, pinned to a release tarball + hash; only the x86 backend is built
set(CAPSTONE_ARCHITECTURE_DEFAULT OFF CACHE BOOL "" FORCE)
set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "" FORCE)
set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CAPSTONE_BUILD_CSTOOL OFF CACHE BOOL "" FORCE)
set(CAPSTONE_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
FetchContent_Declare(capstone
    URL https://github.com/capstone-engine/capstone/archive/refs/tags/5.0.9.tar.gz
    URL_HASH SHA256=0619da31af08152600af95c481527ef6d756c0a8404fca7544a4fdf6dfc2c0f9
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(capstone)

add_library(SaveUnfinishedShips SHARED
    main.cpp
    init.cpp
    log.cpp
)
target_link_libraries(SaveUnfinishedShips PRIVATE capstone_static)
target_compile_definitions(SaveUnfinishedShips PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(SaveUnfinishedShips PRIVATE /W3 /utf-8)

# offline test that runs the scanning code against a Starfield.exe mapped from disk
add_executable(scan_test tests/scan_test.cpp init.cpp log.cpp)
target_link_libraries(scan_test PRIVATE capstone_static)
target_compile_definitions(scan_test PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(scan_test PRIVATE /W3 /utf-8)
