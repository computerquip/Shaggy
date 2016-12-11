cmake_minimum_required(VERSION 3.6)
project(Shaggy)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLM REQUIRED)

set(SOURCE_FILES src/main.c)
add_executable(Shaggy ${SOURCE_FILES})

set(POSIX_LIBRARIES m)

target_link_libraries(Shaggy
        ${SDL2_LIBRARY}
        ${GLEW_LIBRARIES}
        ${POSIX_LIBRARIES}
        )

target_include_directories(Shaggy PRIVATE
        ${SDL2_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}
        )