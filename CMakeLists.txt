cmake_minimum_required(VERSION 3.23)
project(SR2)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(SDL2_PATH "C:/SDL2-2.28.5/x86_64-w64-mingw32")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

add_executable(SR2 main.cpp)


target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY})
target_link_libraries(${PROJECT_NAME} -lopengl32 -lfreeglut)