@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0src"

set COMPILER=clang++
if not defined COMPILER (
    echo Ошибка: clang++ не найден. Убедитесь что установлен LLVM/Clang для Windows.
    exit /b 1
)

set ERRORS=0

REM Компилируем Simulator
echo.
echo [1/3] Компиляция Simulator
%COMPILER% -std=c++17 -o simulator.exe simulator.cpp
if errorlevel 1 (
    echo Ошибка компиляции Simulator
    set /a ERRORS=ERRORS+1
) else (
    echo Simulator скомпилирован успешно
)

REM Компилируем Logger
echo.
echo [2/3] Компиляция Logger (с SQLite)
%COMPILER% -std=c++17 -o logger.exe logger.cpp -lsqlite3
if errorlevel 1 (
    echo Ошибка компиляции Logger
    set /a ERRORS=ERRORS+1
) else (
    echo Logger скомпилирован успешно
)

REM Компилируем Server
echo.
echo [3/3] Компиляция Server (с SQLite и Winsock2)
%COMPILER% -std=c++17 -o server.exe server.cpp -lsqlite3 -lws2_32
if errorlevel 1 (
    echo Ошибка компиляции Server
    set /a ERRORS=ERRORS+1
) else (
    echo Server скомпилирован успешно
)

echo.
echo =======================================

if %ERRORS% equ 0 (
    echo Все компоненты успешно скомпилированы!
    echo.
    echo Для запуска системы используйте:
    echo   run.bat
    echo.
    exit /b 0
) else (
    echo Компиляция завершена с ошибками (%ERRORS%)
    exit /b 1
)
