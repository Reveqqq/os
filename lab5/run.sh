#!/bin/bash

cd "$(dirname "$0")/src"

# Проверяем наличие исполняемых файлов
if [ ! -f "simulator" ] || [ ! -f "logger" ] || [ ! -f "server" ]; then
    echo "Ошибка: не все исполняемые файлы скомпилированы!"
    echo ""
    echo "Скомпилируйте программы:"
    echo "  clang++ -std=c++17 -o simulator simulator.cpp"
    echo "  clang++ -std=c++17 -o logger logger.cpp -lsqlite3"
    echo "  clang++ -std=c++17 -o server server.cpp -lsqlite3"
    exit 1
fi

# Функция для очистки при выходе
cleanup() {
    echo ""
    echo "  Завершение работы программ"
    kill $(jobs -p) 2>/dev/null
    wait
    echo "Программы остановлены"
}

# Обработчик сигнала прерывания
trap cleanup INT TERM

# Удаляем старую БД (опционально)
# rm -f measurements.db

# Запускаем симулятор и логгер в фоне
echo "Запуск симулятора и логгера"
./simulator | ./logger &
LOGGER_PID=$!
sleep 1

# Запускаем сервер
echo "Запуск HTTP сервера"
echo ""
./server &
SERVER_PID=$!

echo "======================================="
echo "Система запущена!"
echo "======================================="
echo ""
echo "Web Interface: http://localhost:8080"
echo ""
echo "API endpoints:"
echo "   - GET http://localhost:8080/api/current"
echo "   - GET http://localhost:8080/api/stats"
echo ""
echo "Database: ./measurements.db"
echo ""
echo "Нажмите Ctrl+C для остановки"
echo ""

# Ждем завершения процессов
wait
