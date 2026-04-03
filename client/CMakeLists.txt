cmake_minimum_required(VERSION 3.16)
project(audio-client LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_executable(audio-client main.cpp src/Listener.cpp src/Player.cpp)
target_include_directories(audio-client PRIVATE include ${PORTAUDIO_INCLUDE_DIRS})
target_link_libraries(audio-client PRIVATE ${PORTAUDIO_LIBRARIES})
