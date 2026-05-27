cmake_minimum_required(VERSION 3.16)
project(jui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 source encoding for MSVC
if(MSVC)
    add_compile_options(/utf-8 /DNOMINMAX)
endif()

# jui core library
set(JUI_SOURCES
    src/core/value.cpp
    src/core/widget.cpp
    src/core/data_model.cpp
    src/core/surface.cpp
    src/core/engine.cpp
    src/render/render_widget.cpp
    src/render/d2d_renderer.cpp
    src/layout/layout.cpp
)

set(JUI_HEADERS
    include/jui/jui.h
    include/jui/core/value.h
    include/jui/core/widget.h
    include/jui/core/data_model.h
    include/jui/core/surface.h
    include/jui/core/engine.h
    include/jui/render/render_widget.h
    include/jui/render/d2d_renderer.h
    include/jui/layout/layout.h
)

add_library(jui ${JUI_SOURCES} ${JUI_HEADERS})
target_include_directories(jui PUBLIC include)
target_include_directories(jui PRIVATE thirdparty)

# Windows: link D2D, DWrite, windowing
if(WIN32)
    target_link_libraries(jui PRIVATE
        d2d1 dwrite windowscodecs
    )
endif()

# Build examples
option(JUI_BUILD_EXAMPLES "Build example applications" ON)
if(JUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================
# Unit Tests (Google Test)
# ============================================================
option(JUI_BUILD_TESTS "Build unit tests" ON)
if(JUI_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    # Prevent gtest from overriding our settings
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    set(TEST_SOURCES
        tests/test_value.cpp
        tests/test_widget.cpp
        tests/test_data_model.cpp
        tests/test_surface.cpp
        tests/test_engine.cpp
        tests/test_render_widget.cpp
        tests/test_d2d_renderer.cpp
        tests/test_layout.cpp
        tests/test_widget_state.cpp
    )

    add_executable(jui_tests ${TEST_SOURCES})
    target_link_libraries(jui_tests PRIVATE jui gtest_main)
    target_include_directories(jui_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/thirdparty
    )
    include(GoogleTest)
    gtest_discover_tests(jui_tests)
endif()
