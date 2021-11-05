project(worker)

cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_COMPILER /usr/bin/c++)
set(CMAKE_CXX_FLAGS "-m64 -std=c++14")
add_compile_options(-W -Wall -g -fpermissive)

include(FetchContent)
# spdlog
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG        v1.9.2
)

FetchContent_MakeAvailable(spdlog)

include_directories(include)

###############################
# for hpp
###############################
add_library(worker SHARED)
target_sources(worker
PRIVATE
    src/worker.cpp
    src/job.cpp
    src/worker_manager.cpp
PUBLIC
    include/worker.h
)

target_include_directories (worker
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${spdlog_SOURCE_DIR}/include
)
