@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0src"

REM Проверяем наличие исполняемых файлов
if not exist "simulator.exe" (
    echo Ошибка: не все исполняемые файлы скомпилированы!
    echo.
    echo Скомпилируйте программы используя build.bat
    exit /b 1
)

if not exist "logger.exe" (
    echo Ошибка: logger.exe не найден!
    echo.
    echo Скомпилируйте программы используя build.bat
    exit /b 1
)

if not exist "server.exe" (
    echo Ошибка: server.exe не найден!
    echo.
    echo Скомпилируйте программы используя build.bat
    exit /b 1
)

REM Удаляем старую БД (опционально)
REM if exist measurements.db del measurements.db

echo Запуск симулятора и логгера
start cmd /k "simulator.exe | logger.exe"

timeout /t 2

echo Запуск HTTP сервера
echo.
start cmd /k "server.exe"

echo =======================================
echo Система запущена!
echo =======================================
echo.
echo Web Interface: http://localhost:8080
echo.
echo API endpoints:
echo    - GET http://localhost:8080/api/current
echo    - GET http://localhost:8080/api/stats
echo.
echo Database: .\measurements.db
echo.
echo Закройте окна для остановки программ
echo.

pause
