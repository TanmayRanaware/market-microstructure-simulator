cmake_minimum_required(VERSION 3.20)
project(MarketMicrostructureSimulator 
        VERSION 1.0.0
        DESCRIPTION "Market Microstructure Simulator with C++ Engine and Python Bindings"
        LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_PYTHON "Build Python bindings" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(ENABLE_LOGGING "Enable detailed logging" OFF)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -DDEBUG)
    endif()
endif()

# Find required packages
find_package(Threads REQUIRED)

# Find Python and pybind11 if building Python bindings
if(BUILD_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    find_package(pybind11 REQUIRED)
endif()

# Find Google Test if building tests
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)

# Source files
set(MMS_SOURCES
    cpp/src/order_book.cpp
    cpp/src/matching_engine.cpp
    cpp/src/agents.cpp
    cpp/src/simulator.cpp
)

set(MMS_HEADERS
    cpp/include/mms/types.hpp
    cpp/include/mms/rng.hpp
    cpp/include/mms/order_book.hpp
    cpp/include/mms/matching_engine.hpp
    cpp/include/mms/agents.hpp
    cpp/include/mms/simulator.hpp
)

# Create core library
add_library(mms_core STATIC ${MMS_SOURCES} ${MMS_HEADERS})
target_link_libraries(mms_core PRIVATE Threads::Threads)

# Set compile definitions
target_compile_definitions(mms_core PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<BOOL:${ENABLE_LOGGING}>:ENABLE_LOGGING>
)

# Build tests
if(BUILD_TESTS)
    enable_testing()
    
    # Test source files
    set(TEST_SOURCES
        cpp/tests/test_types.cpp
        cpp/tests/test_rng.cpp
        cpp/tests/test_order_book.cpp
        cpp/tests/test_matching_engine.cpp
        cpp/tests/test_agents.cpp
        cpp/tests/test_simulator.cpp
    )
    
    add_executable(mms_tests ${TEST_SOURCES})
    target_link_libraries(mms_tests PRIVATE 
        mms_core 
        GTest::gtest 
        GTest::gtest_main
        Threads::Threads
    )
    
    # Add tests to CTest
    add_test(NAME MMSUnitTests COMMAND mms_tests)
endif()

# Build Python bindings
if(BUILD_PYTHON)
    pybind11_add_module(mms_core_py python/mms_bindings.cpp)
    target_link_libraries(mms_core_py PRIVATE mms_core)
    
    # Set Python module properties
    set_target_properties(mms_core_py PROPERTIES
        OUTPUT_NAME "mms_core"
        SUFFIX ".so"
    )
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_executable(simple_sim cpp/examples/simple_sim.cpp)
    target_link_libraries(simple_sim PRIVATE mms_core Threads::Threads)
    
    add_executable(benchmark cpp/examples/benchmark.cpp)
    target_link_libraries(benchmark PRIVATE mms_core Threads::Threads)
endif()

# Installation
install(TARGETS mms_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_TESTS)
    install(TARGETS mms_tests
        RUNTIME DESTINATION bin
    )
endif()

if(BUILD_EXAMPLES)
    install(TARGETS simple_sim benchmark
        RUNTIME DESTINATION bin
    )
endif()

if(BUILD_PYTHON)
    install(TARGETS mms_core_py
        LIBRARY DESTINATION ${Python3_SITELIB}/mms
    )
endif()

# Install headers
install(DIRECTORY cpp/include/ DESTINATION include)

# Python package installation
if(BUILD_PYTHON)
    # Install Python package files
    install(DIRECTORY python/mms/ DESTINATION ${Python3_SITELIB}/mms)
    install(FILES python/setup.py DESTINATION ${Python3_SITELIB}/mms)
    install(FILES requirements.txt DESTINATION ${Python3_SITELIB}/mms)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "MarketMicrostructureSimulator")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Market Microstructure Team")
set(CPACK_PACKAGE_CONTACT "team@mms.example.com")

include(CPack)

# Print configuration summary
message(STATUS "=== Market Microstructure Simulator Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build Python: ${BUILD_PYTHON}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Enable logging: ${ENABLE_LOGGING}")
message(STATUS "======================================================")
