cmake_minimum_required(VERSION 3.13)
project(itt_jit_trigger C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Localiza apenas o modulo opencv_core a partir do diretorio build
if(NOT OpenCV_FOUND)
    find_package(OpenCV REQUIRED core PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../../build" NO_DEFAULT_PATH)
endif()

# Fallback se o OpenCV foi compilado apenas em Release (evita erro procurando opencv_core510d.lib)
if(TARGET opencv_core)
    get_target_property(_dbg_lib opencv_core IMPORTED_IMPLIB_DEBUG)
    if(_dbg_lib AND NOT EXISTS "${_dbg_lib}")
        get_target_property(_rel_lib opencv_core IMPORTED_IMPLIB_RELEASE)
        get_target_property(_rel_dll opencv_core IMPORTED_LOCATION_RELEASE)
        if(_rel_lib)
            set_target_properties(opencv_core PROPERTIES
                IMPORTED_IMPLIB_DEBUG "${_rel_lib}"
                IMPORTED_LOCATION_DEBUG "${_rel_dll}"
            )
        endif()
    endif()
endif()

# Inclui headers necessarios
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/ittnotify/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build
)

# 1. Alvo da DLL/SO de prova (poc_dll)
add_library(poc_dll SHARED poc/poc_dll.c)

# 2. Alvo do executavel de teste
add_executable(itt_jit_trigger src/itt_jit_trigger.cpp)

# Link com opencv_core e ittnotify
find_library(ITTNOTIFY_LIB ittnotify PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../../build/3rdparty/lib/Release" "${CMAKE_CURRENT_SOURCE_DIR}/../../build/3rdparty/lib")

if(ITTNOTIFY_LIB)
    target_link_libraries(itt_jit_trigger PRIVATE opencv_core ${ITTNOTIFY_LIB})
else()
    target_link_libraries(itt_jit_trigger PRIVATE opencv_core)
endif()

# Define diretorio de saida dos binarios em bin/
set_target_properties(poc_dll PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

set_target_properties(itt_jit_trigger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    VS_DEBUGGER_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:itt_jit_trigger>"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    VS_DEBUGGER_ENVIRONMENT "INTEL_JIT_PROFILER64=${CMAKE_CURRENT_SOURCE_DIR}/bin/poc_dll.dll"
)

# Copia automatica pos-build da opencv_core DLL para bin/
add_custom_command(TARGET itt_jit_trigger POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:opencv_core>"
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:opencv_core>"
    COMMENT "Copiando opencv_core DLL para bin/..."
)
