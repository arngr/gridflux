cmake_minimum_required(VERSION 3.10)
project(gridflux)

set(CMAKE_C_STANDARD 99)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# Gather all C source files recursively from the src directory
file(GLOB_RECURSE SESSION_SOURCES "${SRC_DIR}/*.c")

# Include directories
include_directories(${SRC_DIR}/session)

# Create the executable
add_executable(gridflux ${SRC_DIR}/main.c ${SESSION_SOURCES})


# Check OS and adjust flags
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Operating System: Windows")
    # Windows-specific settings
    target_link_libraries(gridflux PRIVATE gdi32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Operating System: macOS")
    # macOS-specific settings (if any)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Operating System: Linux")
    
    # Check if Wayland is being used by checking environment variables
    if(DEFINED ENV{WAYLAND_DISPLAY})
        message(STATUS "Wayland is being used as the display server.")
        execute_process(
            COMMAND echo $XDG_SESSION_TYPE
            OUTPUT_VARIABLE SESSION_TYPE
            RESULT_VARIABLE SESSION_RESULT
            ERROR_VARIABLE SESSION_ERROR
            TIMEOUT 5
        )

        if(SESSION_RESULT EQUAL 0)
            string(STRIP "${SESSION_TYPE}" SESSION_TYPE)
            message(STATUS "Wayland Compositor Type: ${SESSION_TYPE}")
        else()
            message(STATUS "Unable to determine Wayland compositor type")
        endif()

    else()
        message(STATUS "X11 is being used as the display server.")
        target_link_libraries(gridflux PRIVATE X11)
        execute_process(
            COMMAND xprop -root _NET_WM_NAME
            RESULT_VARIABLE WM_RESULT
            OUTPUT_VARIABLE WM_NAME
            ERROR_VARIABLE WM_ERROR
            TIMEOUT 5
        )

        if(WM_RESULT EQUAL 0)
            string(STRIP "${WM_NAME}" WM_NAME)
            message(STATUS "Window Manager in use: ${WM_NAME}")
        else()
            message(STATUS "Unable to determine Window Manager")
        endif()
    endif()
else()
    message(FATAL_ERROR "Unsupported Operating System: ${CMAKE_SYSTEM_NAME}")
endif()

target_include_directories(gridflux PRIVATE session)

add_custom_target(run
    COMMAND gridflux 
    DEPENDS gridflux 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(gridflux PROPERTIES CLEAN_COMMAND "rm -f gridflux")
