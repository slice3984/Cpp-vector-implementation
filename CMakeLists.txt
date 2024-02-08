cmake_minimum_required(VERSION 3.26)
project(vector)

set(CMAKE_CXX_STANDARD 17)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(vector main.cpp
        Vector.h)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

#target_compile_options(vector PRIVATE -fsanitize=address)
#target_link_libraries(vector PRIVATE clang_rt.asan-x86_64)
#target_link_options(vector PRIVATE -fsanitize=address)