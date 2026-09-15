@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo Compilando Sample PoC ITT JIT Trigger (MSVC x64)
echo =======================================================

REM 1. Localizar vcvars64.bat usando vswhere
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find **\vcvars64.bat`) do (
    set VCVARS_BAT=%%i
)

if not defined VCVARS_BAT (
    echo [ERRO] vcvars64.bat nao encontrado! Verifique a instalacao do Visual Studio.
    exit /b 1
)

echo [INFO] Configurando ambiente MSVC: "!VCVARS_BAT!"
call "!VCVARS_BAT!" >nul

set SCRIPT_DIR=%~dp0
set BIN_DIR=!SCRIPT_DIR!bin

if not exist "!BIN_DIR!" (
    mkdir "!BIN_DIR!"
)

REM 2. Caminhos do OpenCV no repositorio
set OPENCV_ROOT=!SCRIPT_DIR!..\..
set OPENCV_BUILD=!OPENCV_ROOT!\build

set INC_DIRS=/I"!OPENCV_ROOT!\include" /I"!OPENCV_ROOT!\modules\core\include" /I"!OPENCV_BUILD!" /I"!OPENCV_ROOT!\3rdparty\ittnotify\include"
set LIB_DIRS=/LIBPATH:"!OPENCV_BUILD!\lib\Release" /LIBPATH:"!OPENCV_BUILD!\3rdparty\lib\Release"
set LIBS=opencv_core510.lib ittnotify.lib

echo [1/3] Compilando DLL de prova (poc\poc_dll.c) para bin\poc_dll.dll...
cl /LD /MD /O2 /nologo "!SCRIPT_DIR!poc\poc_dll.c" /Fo"!BIN_DIR!\poc_dll.obj" /Fe"!BIN_DIR!\poc_dll.dll"
if errorlevel 1 (
    echo [ERRO] Falha ao compilar poc_dll.dll
    exit /b 1
)

echo [2/3] Compilando executavel de teste (src\itt_jit_trigger.cpp) para bin\itt_jit_trigger.exe...
cl /EHsc /MD /O2 /nologo "!SCRIPT_DIR!src\itt_jit_trigger.cpp" !INC_DIRS! /Fo"!BIN_DIR!\itt_jit_trigger.obj" /link !LIB_DIRS! !LIBS! /OUT:"!BIN_DIR!\itt_jit_trigger.exe"
if errorlevel 1 (
    echo [ERRO] Falha ao compilar itt_jit_trigger.exe
    exit /b 1
)

echo [3/3] Copiando runtime DLL do OpenCV (opencv_core510.dll) para bin\...
if exist "!OPENCV_BUILD!\bin\Release\opencv_core510.dll" (
    copy /y "!OPENCV_BUILD!\bin\Release\opencv_core510.dll" "!BIN_DIR!" >nul
    echo [OK] opencv_core510.dll copiada para bin\.
) else (
    echo [AVISO] opencv_core510.dll nao encontrada em !OPENCV_BUILD!\bin\Release\.
)

REM Limpeza de arquivos intermediarios da pasta bin
del /f /q "!BIN_DIR!\*.obj" "!BIN_DIR!\*.exp" "!BIN_DIR!\*.lib" 2>nul

echo =======================================================
echo [SUCESSO] Binarios gerados em !BIN_DIR!
echo  - bin\itt_jit_trigger.exe
echo  - bin\poc_dll.dll
echo  - bin\opencv_core510.dll
echo =======================================================
