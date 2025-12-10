# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(FlipMan VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow an optional hint where SFML lives. Example:
# cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DSFML_ROOT="${CMAKE_SOURCE_DIR}"
if (DEFINED SFML_ROOT)
    set(SFML_HINT_PATH "${SFML_ROOT}")
else()
    set(SFML_HINT_PATH "${CMAKE_SOURCE_DIR}")
endif()

# First try the normal find_package (works if SFML provides a CMake config for your build)
find_package(SFML 2.6 COMPONENTS graphics window system QUIET)

add_executable(flip-man src/main.cpp)

target_include_directories(flip-man PRIVATE ${CMAKE_SOURCE_DIR}/include)

if (SFML_FOUND)
    message(STATUS "Found SFML via find_package: ${SFML_LIBRARIES}")
    target_link_libraries(flip-man PRIVATE sfml-graphics sfml-window sfml-system)
else()
    message(STATUS "SFML not found via find_package — falling back to manual library lookup under ${SFML_HINT_PATH}/lib")

    # Try to find MinGW-style import libraries (libsfml-*.a) in the local lib folder
    find_library(SFML_GRAPHICS_LIB NAMES sfml-graphics sfml-graphics-s PATHS "${SFML_HINT_PATH}/lib" NO_DEFAULT_PATH)
    find_library(SFML_WINDOW_LIB   NAMES sfml-window   sfml-window-s   PATHS "${SFML_HINT_PATH}/lib" NO_DEFAULT_PATH)
    find_library(SFML_SYSTEM_LIB   NAMES sfml-system   sfml-system-s   PATHS "${SFML_HINT_PATH}/lib" NO_DEFAULT_PATH)

    if (NOT SFML_GRAPHICS_LIB)
        message(FATAL_ERROR "Could not find libsfml-graphics (.a) in ${SFML_HINT_PATH}/lib. Make sure you installed the MinGW build of SFML into the project 'lib' folder.")
    endif()
    if (NOT SFML_WINDOW_LIB)
        message(FATAL_ERROR "Could not find libsfml-window (.a) in ${SFML_HINT_PATH}/lib. Make sure you installed the MinGW build of SFML into the project 'lib' folder.")
    endif()
    if (NOT SFML_SYSTEM_LIB)
        message(FATAL_ERROR "Could not find libsfml-system (.a) in ${SFML_HINT_PATH}/lib. Make sure you installed the MinGW build of SFML into the project 'lib' folder.")
    endif()

    message(STATUS "Linking with: ${SFML_GRAPHICS_LIB}; ${SFML_WINDOW_LIB}; ${SFML_SYSTEM_LIB}")

    target_link_libraries(flip-man PRIVATE
        ${SFML_GRAPHICS_LIB}
        ${SFML_WINDOW_LIB}
        ${SFML_SYSTEM_LIB}
        opengl32 gdi32 winmm ws2_32 ole32 uuid
    )
endif()

# Optionally copy DLLs next to the executable on build (works with MinGW runtime DLLs)
if (WIN32)
    add_custom_command(TARGET flip-man POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/bin"
        $<TARGET_FILE_DIR:flip-man>
    )
endif()
