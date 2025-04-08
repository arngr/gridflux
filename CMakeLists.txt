cmake_minimum_required(VERSION 3.10)
project(gridflux)

set(CMAKE_C_STANDARD 99)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SESSION_SOURCES "${SRC_DIR}/*.c")

include_directories(${SRC_DIR}/session)

add_executable(gridflux ${SRC_DIR}/gridflux.c ${SESSION_SOURCES})

option(DEBUG_MODE "Enable debug logging" ON)

if(DEBUG_MODE)
    message(STATUS "Debug mode is ON")
    target_compile_definitions(gridflux PRIVATE DEBUG_MODE=1)
else()
    message(STATUS "Debug mode is OFF")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Operating System: Windows")
    target_link_libraries(gridflux PRIVATE gdi32)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Operating System: macOS")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Operating System: Linux")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WNCK REQUIRED libwnck-3.0)
    pkg_check_modules(GOBJECT REQUIRED gobject-2.0)

    target_include_directories(gridflux PRIVATE ${WNCK_INCLUDE_DIRS} ${GOBJECT_INCLUDE_DIRS})
    target_link_libraries(gridflux PRIVATE ${WNCK_LIBRARIES} ${GOBJECT_LIBRARIES} X11)

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
else()
    message(FATAL_ERROR "Unsupported Operating System: ${CMAKE_SYSTEM_NAME}")
endif()

add_custom_target(run
    COMMAND gridflux 
    DEPENDS gridflux 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(gridflux PROPERTIES CLEAN_COMMAND "rm -f gridflux")
