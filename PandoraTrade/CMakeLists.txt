cmake_minimum_required (VERSION 3.8)

# Set the project name
project (PandoraTrade)

# Check for C++ standard support in the compiler
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Compiler flags for Debug and Release mode
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# Set output directories for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin/Debug/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin/Release/)

# Define macros for Linux
add_definitions(-std=c++11)
add_definitions(-D__LINUX__=1 -Dlinux)

# Include directories
include_directories("../PandoraStrategy/")
include_directories("../Interface/include/")
include_directories("../Interface/")

# Link directories based on the build type
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-g -DDEBUG)
    link_directories("${PROJECT_SOURCE_DIR}/../Interface/lib/Linux/Debug/")
    link_directories("${PROJECT_SOURCE_DIR}/../bin/Debug/")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG -O3)
    link_directories("${PROJECT_SOURCE_DIR}/../Interface/lib/Linux/Release/")
    link_directories("${PROJECT_SOURCE_DIR}/../bin/Release/")
endif()

# Source files in the current directory
aux_source_directory(${CMAKE_CURRENT_LIST_DIR} DIR_SRC)

# Add the executable
add_executable(PandoraTrader ${DIR_SRC})

# Link libraries
TARGET_LINK_LIBRARIES(PandoraTrader pthread)
target_link_libraries(PandoraTrader libPandoraStrategy.a libcwPandoraDLL_Linux.a libTinyXml_Linux.a)

# Link with imported libraries for trading APIs
add_library(CTPTradeLIB SHARED IMPORTED)
set_property(TARGET CTPTradeLIB PROPERTY IMPORTED_LOCATION "/root/projects/PandoraTrader/Interface/CTPTradeApiLinux/thosttraderapi_se.so")

add_library(CTPMdLIB SHARED IMPORTED)
set_property(TARGET CTPMdLIB PROPERTY IMPORTED_LOCATION "/root/projects/PandoraTrader/Interface/CTPTradeApiLinux/thostmduserapi_se.so")

target_link_libraries(PandoraTrader CTPTradeLIB)
target_link_libraries(PandoraTrader CTPMdLIB)
