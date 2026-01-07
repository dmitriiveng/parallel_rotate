cmake_minimum_required(VERSION 3.25)

project(voxel_engine LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)

set(NAME rotate)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${NAME} ${SOURCES})

target_include_directories(${NAME}
    PUBLIC
    src/_include
)
target_include_directories(${NAME} PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(${NAME} PRIVATE pthread ${Boost_LIBRARIES})