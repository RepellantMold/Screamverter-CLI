cmake_minimum_required(VERSION 3.0)

project(Screamverter)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_EXTENSIONS OFF)

if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (MSVC)

add_subdirectory(libtrkcvt)

include_directories(include libtrkcvt/include)

add_executable( Screamverter
                src/main.c
                src/tostm.c
                src/tostx.c)

target_link_libraries(Screamverter PUBLIC libtrkcvt)