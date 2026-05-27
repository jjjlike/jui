add_executable(jui_hello hello/main.cpp)
target_link_libraries(jui_hello PRIVATE jui imm32)
target_include_directories(jui_hello PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty)

# Set Windows subsystem
set_target_properties(jui_hello PROPERTIES WIN32_EXECUTABLE TRUE)
