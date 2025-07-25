# Note that we are using the zig compiler as a drop-in replacement for
# gcc. This allows the unit tests to be compiled across different platforms
# without having to worry about the underlying compiler.

cmake_minimum_required(VERSION 3.10)
project(FastLED_Tests)

# Enforce C++17 globally for all targets.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force creation of thin archives (instead of full .a files) only for non apple builds
if(NOT APPLE)
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> rcT <TARGET> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> rT <TARGET> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> <TARGET>")
    message(STATUS "Using thin archives for Clang build")
endif()

# Enable parallel compilation with aggressive parallelization
include(ProcessorCount)
ProcessorCount(CPU_COUNT)

# Allow override via environment variable
if(DEFINED ENV{FASTLED_PARALLEL_JOBS})
    set(PARALLEL_JOBS $ENV{FASTLED_PARALLEL_JOBS})
    message(STATUS "Using custom parallel jobs: ${PARALLEL_JOBS}")
elseif(CPU_COUNT)
    # Use more jobs than CPU cores for better I/O utilization
    # This is especially effective with fast storage and when some jobs are I/O bound
    math(EXPR PARALLEL_JOBS "${CPU_COUNT} * 2")
    message(STATUS "Setting parallel build level to ${PARALLEL_JOBS} (${CPU_COUNT} CPU cores)")
else()
    # Fallback to 8 jobs if we can't detect CPU count
    set(PARALLEL_JOBS 8)
    message(STATUS "CPU count not detected, setting parallel build level to 8")
endif()

set(CMAKE_BUILD_PARALLEL_LEVEL ${PARALLEL_JOBS})

# Check for ccache to speed up compilation
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    message(STATUS "Using ccache: ${CCACHE_FOUND}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
    # Set ccache options for better performance
    set(ENV{CCACHE_SLOPPINESS} time_macros)
    set(ENV{CCACHE_COMPRESS} true)
    # Additional ccache optimizations for faster builds
    set(ENV{CCACHE_MAXSIZE} 10G)
    set(ENV{CCACHE_DIR} ${CMAKE_BINARY_DIR}/ccache)
else()
    message(STATUS "ccache not found. Install ccache for faster incremental builds.")
endif()

# Enable parallel compilation within individual files
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND COMMON_COMPILE_FLAGS "-pipe")
    # Enable parallel compilation within files (for template-heavy code)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND COMMON_COMPILE_FLAGS "-ftree-parallelize-loops=auto")
    endif()
endif()

# Check if mold linker is available
find_program(MOLD_EXECUTABLE mold)

if(MOLD_EXECUTABLE)
    # Set mold as the default linker
    message(STATUS "Using mold linker: ${MOLD_EXECUTABLE}")
    
    # Add mold linker flags to the common flags
    list(APPEND COMMON_COMPILE_FLAGS "-fuse-ld=mold")
    
    # Set linker flags globally
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=mold")
else()
    find_program(LLDLINK_EXECUTABLE lld-link)
    if(LLDLINK_EXECUTABLE)
        message(STATUS "Using lld-link linker: ${LLDLINK_EXECUTABLE}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    else()
        message(STATUS "Neither mold nor lld-link found. Using system default linker.")
    endif()
endif()

# Set build type to Debug
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)

# Output the current build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Additional build optimizations for faster compilation
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FAST_DEBUG_OPT -O0)
    # Use minimal debug info for faster compilation
    set(FAST_DEBUG_INFO -gline-tables-only)
    message(STATUS "Using optimized debug build settings for faster compilation")
else()
    # Full debugging settings (enables step-through debugging)
    set(FAST_DEBUG_OPT -O0)  # No optimization for accurate debugging
    # Generate both debug formats: DWARF for GDB and CodeView for stack traces
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # On Windows with Clang: generate both DWARF (for GDB) and CodeView (for stack traces)
        set(FAST_DEBUG_INFO -g3 -gdwarf-4)  # DWARF for GDB compatibility + full debug info
        message(STATUS "Using dual debug format: DWARF for GDB + CodeView for stack traces")
    else()
        # Non-Windows or non-Clang: standard DWARF debug info
        set(FAST_DEBUG_INFO -g3)  # Full debug info including macros
        message(STATUS "Using DWARF debug format")
    endif()
endif()

# Optimization suggestions for even faster builds (uncomment to test):
# 1. Use -Og instead of -O1 for potentially faster compilation:
#    set(FAST_DEBUG_OPT -Og)
# 2. Reduce debug info further for fastest builds (use only for CI):
#    set(FAST_DEBUG_INFO -g0)  # No debug info
# 3. Disable frame pointers if stack traces aren't needed:
#    set(FAST_FRAME_POINTER -fomit-frame-pointer)
# Current optimized settings (proven 66% improvement from baseline):
# set(FAST_DEBUG_OPT -O1)  # DISABLED for full debugging
# set(FAST_DEBUG_INFO -gline-tables-only)  # something wrong with this
# set(FAST_FRAME_POINTER -fno-omit-frame-pointer)

# 🚀 FAST BUILD TIP: Use ./fast_build.sh [jobs] for even faster builds!
# Example: ./fast_build.sh 16 (uses 16 parallel jobs)
# Default: ./fast_build.sh (uses 2x CPU cores)

# Full debugging settings (enables step-through debugging)
set(FAST_DEBUG_OPT -O0)  # No optimization for accurate debugging

# Generate both debug formats: DWARF for GDB and CodeView for stack traces
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # On Windows with Clang: generate both DWARF (for GDB) and CodeView (for stack traces)
    set(FAST_DEBUG_INFO -g3 -gdwarf-4)  # DWARF for GDB compatibility + full debug info
    message(STATUS "Using dual debug format: DWARF for GDB + CodeView for stack traces")
else()
    # Non-Windows or non-Clang: standard DWARF debug info
    set(FAST_DEBUG_INFO -g3)  # Full debug info including macros
    message(STATUS "Using DWARF debug format")
endif()

# Windows symbol files for stack traces
if(WIN32)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Clang supports CodeView debug format for Windows
        set(FAST_DEBUG_INFO ${FAST_DEBUG_INFO} -gcodeview)
        message(STATUS "Adding CodeView debug info for Windows stack traces (Clang)")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC doesn't support CodeView, but DWARF format works well for stack traces
        # LLD linker doesn't support --print-map, so we skip MAP file generation
        message(STATUS "Using DWARF debug format for Windows stack traces (GCC)")
    endif()
endif()

set(FAST_FRAME_POINTER -fno-omit-frame-pointer)

# Define common compiler flags and definitions
set(COMMON_COMPILE_FLAGS
    -Wall
    -Wextra 
    #-Wpedantic
    -funwind-tables
    ${FAST_DEBUG_INFO}
    ${FAST_FRAME_POINTER}
    ${FAST_DEBUG_OPT}
    -Werror=return-type
    -Werror=missing-declarations
    -Werror=redundant-decls
    -Werror=init-self
    -Werror=missing-field-initializers  
    -Werror=pointer-arith
    -Werror=write-strings
    -Werror=format=2
    -Werror=implicit-fallthrough
    -Werror=missing-include-dirs
    -Werror=date-time
    -Werror=unused-parameter
    -Werror=unused-variable
    -Werror=unused-value
    -Werror=cast-align
    -Werror=misleading-indentation
    -Werror=uninitialized
    -Werror=array-bounds
    -Werror=strict-overflow=2
    -Werror=null-dereference
    -Werror=format-security
    -Werror=unused-function
    -Werror=deprecated-declarations
    # Additional warnings for gradual code quality improvement
    #-Wshadow
    -DFASTLED_FIVE_BIT_HD_GAMMA_FUNCTION_2_8
    -Wno-comment
    # ignore Arduino/PlatformIO-specific PROGMEM macro
    -DPROGMEM=
)

# Add GCC-specific warning flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND COMMON_COMPILE_FLAGS 
        -Werror=maybe-uninitialized
        -Werror=logical-op
        -Werror=class-memaccess
    )
endif()

# C++-specific compiler flags
set(CXX_SPECIFIC_FLAGS
    -Werror=suggest-override
    -Werror=non-virtual-dtor
    -Werror=reorder
    -Werror=sign-compare
    -Werror=float-equal
    -Werror=mismatched-tags
    -Werror=switch-enum
    #-Werror=zero-as-null-pointer-constant
    -Werror=delete-non-virtual-dtor
    # Additional warnings for gradual code quality improvement
    #-Wold-style-cast
    #-Wsign-conversion
)

# Add GCC-specific C++ warning flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND CXX_SPECIFIC_FLAGS 
        #-Werror=useless-cast
        -Werror=redundant-move
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND CXX_SPECIFIC_FLAGS -Werror=self-assign -Werror=infinite-recursion -Werror=extra-tokens -Werror=unused-private-field -Wglobal-constructors -Werror=global-constructors)
endif()

# Set FASTLED_ALL_SRC=1 for clang builds or when explicitly requested via environment variable
# This allows testing both all source on (clang) and off (gcc) builds
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR DEFINED ENV{FASTLED_ALL_SRC})
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Clang compiler detected: enabling FASTLED_ALL_SRC for unified compilation testing")
    else()
        message(STATUS "FASTLED_ALL_SRC environment variable set: enabling unified compilation testing")
    endif()
    set(FASTLED_ALL_SRC 1)
    add_compile_definitions(FASTLED_ALL_SRC=1)
endif()


set(UNIT_TEST_COMPILE_FLAGS
    -Wall
    #-Wextra 
    #-Wpedantic
    -funwind-tables
    ${FAST_DEBUG_INFO}
    ${FAST_FRAME_POINTER}
    ${FAST_DEBUG_OPT}
    -Werror=return-type
    -Werror=missing-declarations
    #-Werror=redundant-decls
    -Werror=init-self
    #-Werror=missing-field-initializers  
    #-Werror=pointer-arith
    #-Werror=write-strings
    #-Werror=format=2
    #-Werror=implicit-fallthrough
    #-Werror=missing-include-dirs
    -Werror=date-time
    #-Werror=unused-parameter
    #-Werror=unused-variable
    #-Werror=unused-value
    -Werror=uninitialized
    -Werror=array-bounds
    -Werror=null-dereference
    -Werror=deprecated-declarations

    # Not supported in gcc.
    #-Werror=infinite-recursion
    #-v
    -Wno-comment
)

# Add GCC-specific unit test warning flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND UNIT_TEST_COMPILE_FLAGS 
        -Werror=maybe-uninitialized
    )
endif()

set(UNIT_TEST_CXX_FLAGS
    -Werror=suggest-override
    -Werror=non-virtual-dtor
    -Werror=switch-enum
    #-Werror=reorder
    #-Werror=sign-compare
    #-Werror=float-equal
    #-Werror=conversion
    -Werror=delete-non-virtual-dtor
    #-Werror=zero-as-null-pointer-constant
)

set(COMMON_COMPILE_DEFINITIONS
    DEBUG
    FASTLED_FORCE_NAMESPACE=1
    FASTLED_USE_JSON_UI=1
    FASTLED_NO_AUTO_NAMESPACE
    FASTLED_TESTING
    ENABLE_CRASH_HANDLER
    FASTLED_STUB_IMPL
    FASTLED_NO_PINMAP
    HAS_HARDWARE_PIN_SUPPORT
    _GLIBCXX_DEBUG
    _GLIBCXX_DEBUG_PEDANTIC
)

# Fix for Microsoft STL version check with older Clang versions on Windows
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND COMMON_COMPILE_DEFINITIONS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)
endif()


# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.build/bin)

# Set binary directory
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.build/bin)

# Set path to FastLED source directory
add_compile_definitions(${COMMON_COMPILE_DEFINITIONS})
set(FASTLED_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Include FastLED source directory
include_directories(${FASTLED_SOURCE_DIR}/src)

# Delegate source file computation to src/CMakeLists.txt
add_subdirectory(${FASTLED_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/fastled)

if(NOT APPLE)
    target_link_options(fastled PRIVATE -static-libgcc -static-libstdc++)
endif()

# Try to find libunwind, but make it optional
# libunwind doesn't always provide CMake config files, so we'll search manually

# First try to find using pkg-config
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBUNWIND QUIET libunwind)
endif()

# If pkg-config didn't work, try manual search
if(NOT LIBUNWIND_FOUND)
    find_path(LIBUNWIND_INCLUDE_DIR 
        NAMES libunwind.h
        PATHS /usr/include /usr/local/include /opt/homebrew/include
    )

    find_library(LIBUNWIND_LIBRARY 
        NAMES unwind
        PATHS /usr/lib /usr/local/lib /opt/homebrew/lib /usr/lib/x86_64-linux-gnu
    )

    find_library(LIBUNWIND_X86_64_LIBRARY 
        NAMES unwind-x86_64
        PATHS /usr/lib /usr/local/lib /opt/homebrew/lib /usr/lib/x86_64-linux-gnu
    )

    # Check if both header and library are found
    if(LIBUNWIND_INCLUDE_DIR AND LIBUNWIND_LIBRARY)
        set(LIBUNWIND_FOUND TRUE)
        if(LIBUNWIND_X86_64_LIBRARY)
            set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARY} ${LIBUNWIND_X86_64_LIBRARY})
        else()
            set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARY})
        endif()
        set(LIBUNWIND_INCLUDE_DIRS ${LIBUNWIND_INCLUDE_DIR})
    endif()
endif()

# Set the final flag based on what we found
if(LIBUNWIND_FOUND)
    set(USE_LIBUNWIND TRUE)
    # For x86_64 systems, we need both libunwind and libunwind-x86_64
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(LIBUNWIND_LIBRARIES unwind unwind-x86_64 lzma)
        message(STATUS "LibUnwind found for x86_64: ${LIBUNWIND_LIBRARIES}")
    else()
        # Use pkg-config results if available, otherwise use manual search
        if(PKG_CONFIG_FOUND AND LIBUNWIND_STATIC_LIBRARIES)
            set(LIBUNWIND_LIBRARIES ${LIBUNWIND_STATIC_LIBRARIES})
            set(LIBUNWIND_INCLUDE_DIRS ${LIBUNWIND_STATIC_INCLUDE_DIRS})
            message(STATUS "LibUnwind found via pkg-config: ${LIBUNWIND_LIBRARIES}")
        elseif(LIBUNWIND_LIBRARIES AND LIBUNWIND_INCLUDE_DIRS)
            # Add x86_64 library if available
            if(LIBUNWIND_X86_64_LIBRARY)
                list(APPEND LIBUNWIND_LIBRARIES ${LIBUNWIND_X86_64_LIBRARY})
            endif()
            message(STATUS "LibUnwind found manually: ${LIBUNWIND_LIBRARIES}")
        endif()
    endif()
    message(STATUS "LibUnwind headers: ${LIBUNWIND_INCLUDE_DIRS}")
else()
    set(USE_LIBUNWIND FALSE)
    message(STATUS "LibUnwind not found. Falling back to basic stack traces with execinfo.")
endif()

# Enable testing
enable_testing()

# Create a static library for test infrastructure to avoid Windows symbol conflicts
# This avoids the duplicate symbol issues that occur when mixing shared and static libraries
add_library(test_shared_static STATIC
    doctest_main.cpp
)

target_include_directories(test_shared_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(test_shared_static PRIVATE ${UNIT_TEST_COMPILE_FLAGS})
target_compile_options(test_shared_static PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${UNIT_TEST_CXX_FLAGS}>)
target_compile_definitions(test_shared_static PRIVATE ${COMMON_COMPILE_DEFINITIONS})

# Enable debug symbols for the static library on Windows
if(WIN32)
    if(MSVC)
        set_target_properties(test_shared_static PROPERTIES
            COMPILE_FLAGS "/Zi /Od /RTC1")
    else()
        target_compile_options(test_shared_static PRIVATE -g -O0 -fno-omit-frame-pointer)
    endif()
    target_compile_definitions(test_shared_static PRIVATE _DEBUG)
endif()

# Find all test source files
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

# Find test executables (only actual test executables, not libraries)
file(GLOB TEST_BINARIES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_*${CMAKE_EXECUTABLE_SUFFIX}")

# Process source files - create executables that link to static library
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Create executable that links to static library
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} fastled test_shared_static)
    
    # Set the correct subsystem for Windows
    if(WIN32)
        # Add Windows debugging libraries for crash handler
        target_link_libraries(${TEST_NAME} dbghelp psapi)
        
        # Enable debug symbols for better stack traces
        if(MSVC)
            set_target_properties(${TEST_NAME} PROPERTIES
                WIN32_EXECUTABLE FALSE
                LINK_FLAGS "/SUBSYSTEM:CONSOLE /DEBUG:FULL /OPT:NOREF /OPT:NOICF"
                COMPILE_FLAGS "/Zi /Od /RTC1")
        else()
            # For MinGW/Clang on Windows
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                set_target_properties(${TEST_NAME} PROPERTIES
                    WIN32_EXECUTABLE FALSE
                    LINK_FLAGS "-Xlinker /subsystem:console -g -O0")
                target_compile_options(${TEST_NAME} PRIVATE -g -O0 -fno-omit-frame-pointer)
            else()
                set_target_properties(${TEST_NAME} PROPERTIES
                    WIN32_EXECUTABLE FALSE)
                target_compile_options(${TEST_NAME} PRIVATE -g -O0 -fno-omit-frame-pointer)
            endif()
        endif()
        
        # Ensure debug information is preserved
        target_compile_definitions(${TEST_NAME} PRIVATE _DEBUG)
    endif()
    
    # Add C++-specific flags only for C++ files
    target_compile_options(fastled PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CXX_SPECIFIC_FLAGS}>)
    if(USE_LIBUNWIND)
        target_link_libraries(${TEST_NAME} ${LIBUNWIND_LIBRARIES})
        target_include_directories(${TEST_NAME} PRIVATE ${LIBUNWIND_INCLUDE_DIRS})
    endif()
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${TEST_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    # Add static linking flags and debug flags for test executables
    if(NOT APPLE AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(${TEST_NAME} PRIVATE -static-libgcc -static-libstdc++)
    endif()
    target_compile_options(${TEST_NAME} PRIVATE ${UNIT_TEST_COMPILE_FLAGS})
    # Add C++-specific flags only for C++ files
    target_compile_options(${TEST_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${UNIT_TEST_CXX_FLAGS}>)
    target_compile_definitions(${TEST_NAME} PRIVATE 
        ${COMMON_COMPILE_DEFINITIONS}
        $<$<BOOL:${USE_LIBUNWIND}>:USE_LIBUNWIND>
    )
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Process remaining binaries (those without corresponding source files)
option(CLEAN_ORPHANED_BINARIES "Remove orphaned test binaries" ON)
if(CLEAN_ORPHANED_BINARIES)
    foreach(ORPHANED_BINARY ${TEST_BINARIES})
        get_filename_component(BINARY_NAME ${ORPHANED_BINARY} NAME_WE)
        get_filename_component(BINARY_DIR ${ORPHANED_BINARY} DIRECTORY)
        get_filename_component(PARENT_DIR ${BINARY_DIR} DIRECTORY)
        get_filename_component(GRANDPARENT_DIR ${PARENT_DIR} DIRECTORY)
        set(CORRESPONDING_SOURCE "${GRANDPARENT_DIR}/${BINARY_NAME}.cpp")
        if(NOT EXISTS "${CORRESPONDING_SOURCE}")
            message(STATUS "Found orphaned binary without source: ${ORPHANED_BINARY}")
            file(REMOVE "${ORPHANED_BINARY}")
            message(STATUS "Deleted orphaned binary: ${ORPHANED_BINARY}")
        endif()
    endforeach()
endif()

# Add verbose output for tests
set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")
