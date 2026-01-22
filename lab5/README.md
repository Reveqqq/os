# Система мониторинга температуры

Полнофункциональная система для мониторинга температуры с HTTP сервером, веб-интерфейсом и хранением данных в SQLite БД.

**Кроссплатформенная:** работает на Linux, macOS и Windows

## Компоненты

### 1. **Simulator** (simulator.cpp)
- Генерирует значения температуры по нормальному распределению
- Выдает данные в формате: `YYYY-MM-DDTHH:MM:SS temperature`
- Отправляет новые измерения каждые 5 секунд

### 2. **Logger** (logger.cpp)
- Получает данные от симулятора через stdin
- Сохраняет измерения в SQLite БД (`measurements.db`)
- Вычисляет и сохраняет среднечасовые и среднедневные значения
- Автоматически очищает старые данные

### 3. **Server** (server.cpp)
- HTTP сервер на порту **8080**
- REST API endpoints:
  - `GET /api/current` - текущая температура
  - `GET /api/stats?start=YYYY-MM-DDTHH:MM:SS&end=YYYY-MM-DDTHH:MM:SS` - статистика за период
- Обслуживает статические файлы:
  - `/index.html` - главная страница
  - `/style.css` - стили
  - `/script.js` - клиентский код

### 4. **Web Application** (index.html, style.css, script.js)
- Интерактивный веб-интерфейс
- Отображает текущую температуру
- График температуры за выбранный период (Chart.js)
- Таблица с детальными данными
- Статистика (средняя, min, max, количество измерений)
- Фильтры по периодам времени

## Построение

### Компиляция

#### Linux/macOS
```bash
bash build.sh
```

#### Windows
```batch
build.bat
```

## Запуск

### Автоматический запуск

#### Linux/macOS
```bash
./run.sh
```

#### Windows
```batch
run.bat
```


### Ручной запуск

#### Linux/macOS
```bash
# В одном терминале
cd src
./simulator | ./logger

# В другом терминале
cd src
./server
```

#### Windows
```batch
REM В одном окне
cd src
simulator.exe | logger.exe

REM В другом окне
cd src
server.exe
```

## Структура БД

### Таблица `measurements`
- `id` - уникальный идентификатор
- `timestamp` - время измерения (YYYY-MM-DDTHH:MM:SS)
- `temperature` - значение температуры
- `created_at` - время добавления в БД

### Таблица `hourly_avg`
- `id` - уникальный идентификатор
- `date_hour` - дата и час (YYYY-MM-DD HH)
- `average` - среднее значение за час
- `created_at` - время добавления в БД

### Таблица `daily_avg`
- `id` - уникальный идентификатор
- `date` - дата (YYYY-MM-DD)
- `average` - среднее значение за день
- `created_at` - время добавления в БД

## API

### GET /api/current

Возвращает текущую температуру.

**Ответ:**
```json
{
  "timestamp": "2024-01-20T14:30:45",
  "temperature": 22.45
}
```

### GET /api/stats

Возвращает статистику за выбранный период.

**Параметры:**
- `start` - начало периода (YYYY-MM-DDTHH:MM:SS)
- `end` - конец периода (YYYY-MM-DDTHH:MM:SS)

**Ответ:**
```json
{
  "data": [
    {
      "timestamp": "2024-01-20T14:00:00",
      "temperature": 21.50
    },
    {
      "timestamp": "2024-01-20T14:05:00",
      "temperature": 22.30
    }
  ],
  "summary": {
    "count": 288,
    "average": 22.15,
    "min": 18.50,
    "max": 25.80
  }
}
```

## Требования

- C++17 или выше
- SQLite3 dev библиотека

**Платформо-специфичные требования:**
- Linux: `libsqlite3-dev`
- macOS: `sqlite3` 
- Windows: SQLite3 binaries + Winsock2 (встроена в Windows SDK)

## Внешние зависимости

- [Chart.js 3.9.1](https://www.chartjs.org/) - для построения графиков
- SQLite3 - для хранения данных
