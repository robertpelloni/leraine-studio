#meta
cmake_minimum_required(VERSION 3.0.0)
set(PROJECT_NAME leraine-studio)
project(${PROJECT_NAME} VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

set(WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/binaries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKING_DIRECTORY}")

set(GCC_COVERAGE_COMPILE_FLAGS "lm")

#build routine
include_directories($ENV{VCPKG_ROOT}/installed/x64-linux/include include)
link_directories($ENV{VCPKG_ROOT}/installed/x64-linux/lib libraries)

file(GLOB_RECURSE ALL_SOURCE_FILES "source/*.cpp" "source/*.cxx" "source/*.c")
file(GLOB_RECURSE HEADER_FILES "source/*.h" "source/*.hpp")

# Filter out main.cpp for the library
list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX "main.cpp$")
set(LIB_SOURCES ${ALL_SOURCE_FILES})

find_package(SFML COMPONENTS system window graphics audio CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(ImGui CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(yaml-cpp REQUIRED)

# Create library
add_library(leraine_lib STATIC ${LIB_SOURCES} ${HEADER_FILES})
target_link_libraries(leraine_lib PRIVATE
    ImGui-SFML::ImGui-SFML
    FLAC
    OpenAL
    OpenGL
    Vorbis
    sfml-graphics
    sfml-window
    sfml-audio
    sfml-system
    bass
    bass_fx
    ZLIB::ZLIB
    yaml-cpp
)

# Main Executable
add_executable(${PROJECT_NAME} source/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE leraine_lib)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# Copy asset files to binaries/
file(GLOB_RECURSE ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
foreach(ONE_ASSET ${ASSETS})
  file(RELATIVE_PATH RELATIVE_ASSET "${CMAKE_CURRENT_SOURCE_DIR}/assets/" "${ONE_ASSET}")
  set(ASSET_DESTINATION "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${RELATIVE_ASSET}")
  get_filename_component(ASSET_DESTINATION_DIR "${ASSET_DESTINATION}" DIRECTORY)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${ASSET_DESTINATION_DIR})
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${ONE_ASSET}
    ${ASSET_DESTINATION_DIR})
endforeach()

# Tests
enable_testing()
add_executable(test_runner tests/test_main.cpp)
target_link_libraries(test_runner PRIVATE leraine_lib)
add_test(NAME CoreTests COMMAND test_runner)
