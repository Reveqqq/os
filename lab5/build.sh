#!/bin/bash

cd "$(dirname "$0")/src"

# Определяем компилятор и флаги в зависимости от ОС
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    # Windows
    COMPILER=${CXX:-clang++}
    SIMULATOR_FLAGS="-std=c++17"
    LOGGER_FLAGS="-std=c++17 -lsqlite3"
    SERVER_FLAGS="-std=c++17 -lsqlite3 -lws2_32"
    EXE_EXT=".exe"
    USE_COLOR=0
else
    # POSIX (Linux, macOS)
    COMPILER=${CXX:-clang++}
    SIMULATOR_FLAGS="-std=c++17"
    LOGGER_FLAGS="-std=c++17 -lsqlite3"
    SERVER_FLAGS="-std=c++17 -lsqlite3"
    EXE_EXT=""
    USE_COLOR=1
fi

if [ $USE_COLOR -eq 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    NC='\033[0m'
else
    RED=""
    GREEN=""
    YELLOW=""
    NC=""
fi

ERRORS=0

# Компилируем Simulator
echo ""
echo -e "${YELLOW}[1/3]${NC} Компиляция Simulator"
if $COMPILER $SIMULATOR_FLAGS -o simulator$EXE_EXT simulator.cpp; then
    echo -e "${GREEN}Simulator скомпилирован успешно${NC}"
else
    echo -e "${RED}Ошибка компиляции Simulator${NC}"
    ERRORS=$((ERRORS + 1))
fi

# Компилируем Logger
echo ""
echo -e "${YELLOW}[2/3]${NC} Компиляция Logger (с SQLite)"
if $COMPILER $LOGGER_FLAGS -o logger$EXE_EXT logger.cpp; then
    echo -e "${GREEN}Logger скомпилирован успешно${NC}"
else
    echo -e "${RED}Ошибка компиляции Logger${NC}"
    ERRORS=$((ERRORS + 1))
fi

# Компилируем Server
echo ""
echo -e "${YELLOW}[3/3]${NC} Компиляция Server (с SQLite)"
if $COMPILER $SERVER_FLAGS -o server$EXE_EXT server.cpp; then
    echo -e "${GREEN}Server скомпилирован успешно${NC}"
else
    echo -e "${RED}Ошибка компиляции Server${NC}"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "======================================="

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}Все компоненты успешно скомпилированы!${NC}"
    echo ""
    echo "Для запуска системы используйте:"
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
        echo "  run.bat"
    else
        echo "  ./run.sh"
    fi
    echo ""
    exit 0
else
    echo -e "${RED}Компиляция завершена с ошибками ($ERRORS)${NC}"
    exit 1
fi
