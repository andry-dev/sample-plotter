cmake_minimum_required(VERSION 3.9)

add_subdirectory(tewi)

add_executable(algo src/main.cpp)

set_target_properties(algo
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_compile_options(algo
    PRIVATE
    $<$<CONFIG:RELEASE>:-O3>)

if (MSVC)
target_compile_options(algo
    PRIVATE
    /std:c++latest)
endif()
    

target_include_directories(algo PRIVATE include)

target_link_libraries(algo tewi)
