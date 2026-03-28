cmake_minimum_required(VERSION 3.16)
project(audio-client LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../portaudio ${CMAKE_CURRENT_BINARY_DIR}/portaudio)

add_executable(audio-client main.cpp)
target_link_libraries(audio-client PRIVATE portaudio)
