cmake_minimum_required(VERSION 3.13)
project(itt_jit_trigger C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Localiza OpenCV ou utiliza os caminhos do build
if(NOT OpenCV_FOUND)
    find_package(OpenCV REQUIRED PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../../build" NO_DEFAULT_PATH)
endif()

# Inclui headers necessarios
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/ittnotify/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build
)

# 1. Alvo da DLL/SO de prova (poc_dll)
add_library(poc_dll SHARED poc_dll.c)

# 2. Alvo do executavel de teste
add_executable(itt_jit_trigger itt_jit_trigger.cpp)

# Link com opencv_core e ittnotify
find_library(ITTNOTIFY_LIB ittnotify PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../../build/3rdparty/lib/Release" "${CMAKE_CURRENT_SOURCE_DIR}/../../build/3rdparty/lib")

if(ITTNOTIFY_LIB)
    target_link_libraries(itt_jit_trigger PRIVATE ${OpenCV_LIBS} ${ITTNOTIFY_LIB})
else()
    target_link_libraries(itt_jit_trigger PRIVATE ${OpenCV_LIBS})
endif()
