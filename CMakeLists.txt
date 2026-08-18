cmake_minimum_required(VERSION 3.16)
project(solaris LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Binary output folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)

# Enable Qt standard autogen tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 or fallback to Qt5
find_package(Qt6 COMPONENTS Widgets QUIET)
if (Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_VERSION}")
    set(QT_LIBRARIES Qt6::Widgets)
else()
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    message(STATUS "Found Qt5: ${Qt5_VERSION}")
    set(QT_LIBRARIES Qt5::Widgets)
endif()

include_directories(src)

add_executable(solaris
    src/main.cpp
    src/CPU.cpp
    src/CPU_Thumb.cpp
    src/CPU_ARM.cpp
    src/CPU.h
)

target_link_libraries(solaris PRIVATE ${QT_LIBRARIES})

add_executable(test_cpu
    tests/test_cpu.cpp
    src/CPU.cpp
    src/CPU_Thumb.cpp
    src/CPU_ARM.cpp
    src/CPU.h
)

if (WIN32)
    set_target_properties(solaris PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )
endif()
