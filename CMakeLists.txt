cmake_minimum_required(VERSION 3.16)
project(jui VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 source encoding for MSVC
if(MSVC)
    add_compile_options(/utf-8 /DNOMINMAX)
endif()

# ============================================================
# jui core library
# ============================================================
set(JUI_SOURCES
    src/core/value.cpp
    src/core/widget.cpp
    src/core/data_model.cpp
    src/core/surface.cpp
    src/core/engine.cpp
    src/render/render_widget.cpp
    src/render/d2d_renderer.cpp
    src/layout/layout.cpp
    src/test/driver.cpp
    src/test/validator.cpp
    src/test/recorder.cpp
    src/test/debug_logger.cpp
    src/inspector/inspector.cpp
)

set(JUI_HEADERS
    include/jui/jui.h
    include/jui/core/value.h
    include/jui/core/widget.h
    include/jui/core/data_model.h
    include/jui/core/surface.h
    include/jui/core/engine.h
    include/jui/render/render_widget.h
    include/jui/render/widget_state.h
    include/jui/render/d2d_renderer.h
    include/jui/layout/layout.h
    include/jui/test/driver.h
    include/jui/test/validator.h
    include/jui/test/recorder.h
    include/jui/test/debug_logger.h
    include/jui/test/test.h
    include/jui/inspector/inspector.h
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

# ============================================================
# Build examples (10 progressive examples)
# ============================================================
option(JUI_BUILD_EXAMPLES "Build example applications" ON)
if(JUI_BUILD_EXAMPLES)
    # Legacy hello example (kept for backward compatibility)
    add_executable(jui_hello examples/hello/main.cpp)
    target_link_libraries(jui_hello PRIVATE jui imm32)
    target_include_directories(jui_hello PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty)
    set_target_properties(jui_hello PROPERTIES WIN32_EXECUTABLE TRUE)

    # New progressive examples 01-10
    set(EXAMPLE_NAMES
        01_hello_window 02_hello_button 03_layout_demo
        04_form_demo 05_controls_demo 06_tabs_demo
        07_list_demo 08_grid_demo 09_binding_demo 10_full_app
    )
    foreach(NAME ${EXAMPLE_NAMES})
        add_executable(jui_example_${NAME} examples/${NAME}/main.cpp)
        target_link_libraries(jui_example_${NAME} PRIVATE jui imm32)
        target_include_directories(jui_example_${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty)
        set_target_properties(jui_example_${NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endforeach()
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
        tests/test_list_grid.cpp
        tests/test_ai_integration.cpp
        tests/test_all_examples.cpp
        tests/test_visual_coverage.cpp
        tests/test_debug_logger.cpp
        tests/test_blackbox_03.cpp
        tests/test_text_visibility.cpp
        tests/test_inspector.cpp
        tests/test_all_examples_inspect.cpp
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
