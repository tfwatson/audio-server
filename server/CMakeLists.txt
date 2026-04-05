cmake_minimum_required(VERSION 3.16)
project(audio-server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
find_package(Boost REQUIRED)

add_executable(audio-server main.cpp)
target_include_directories(audio-server PRIVATE include ${PORTAUDIO_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(audio-server PRIVATE ${PORTAUDIO_LIBRARIES})
