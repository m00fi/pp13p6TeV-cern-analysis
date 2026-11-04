cmake_minimum_required(VERSION 3.20)
project(pp13p6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ROOT 6 REQUIRED COMPONENTS Core RIO Tree Hist)
find_path(PYTHIA8_INCLUDE_DIR Pythia8/Pythia.h REQUIRED)
find_library(PYTHIA8_LIBRARY pythia8 REQUIRED)

add_executable(generate src/generate.cc)
target_include_directories(generate PRIVATE ${PYTHIA8_INCLUDE_DIR})
target_link_libraries(generate PRIVATE ${PYTHIA8_LIBRARY} ${CMAKE_DL_LIBS} ROOT::Core ROOT::RIO ROOT::Tree)