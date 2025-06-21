cmake_minimum_required(VERSION 3.15)
project(SpotifySyncAudio LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Windows-specific configurations
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(WINSOCK ws2_32.lib)
    set(WASAPI ole32.lib)
endif()

# Include directories
include_directories(include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Test executable: audio capture test
add_executable(test_audio_capture
    src/test_audio_capture.cpp
    src/audio_capture.cpp
    src/logger.cpp
)
target_link_libraries(test_audio_capture ${WASAPI})

# Executable: sender
add_executable(sender
    src/sender.cpp
    src/audio_capture.cpp
    src/network_udp.cpp
    src/logger.cpp
    src/config.cpp
)
target_link_libraries(sender ${WINSOCK} ${WASAPI})

# Executable: receiver
add_executable(receiver
    src/receiver.cpp
    src/audio_playback.cpp
    src/network_udp.cpp
    src/jitter_buffer.cpp
    src/logger.cpp
    src/config.cpp
)
target_link_libraries(receiver ${WINSOCK} ${WASAPI})

# Set output directories
set_target_properties(sender receiver test_audio_capture
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Compiler-specific flags
if(MSVC)
    target_compile_options(sender PRIVATE /W4)
    target_compile_options(receiver PRIVATE /W4)
    target_compile_options(test_audio_capture PRIVATE /W4)
else()
    target_compile_options(sender PRIVATE -Wall -Wextra)
    target_compile_options(receiver PRIVATE -Wall -Wextra)
    target_compile_options(test_audio_capture PRIVATE -Wall -Wextra)
endif()

# Print configuration info
message(STATUS "Building SpotifySyncAudio")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
if(WIN32)
    message(STATUS "Windows Target: ${_WIN32_WINNT}")
endif()
