#include <iostream>
#include <sqlite3.h>
#include <sstream>
#include <deque>
#include <vector>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <mutex>

using Clock = std::chrono::system_clock;

struct Measurement {
    Clock::time_point time;
    double value;
};

std::mutex db_mutex;

Clock::time_point parseTime(const std::string& s) {
    std::tm tm{};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return Clock::from_time_t(std::mktime(&tm));
}

std::string timeToIso(const Clock::time_point& tp) {
    std::time_t t = Clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buf);
}

std::tm toTM(const Clock::time_point& tp) {
    std::time_t t = Clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

void initDatabase(sqlite3* db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS measurements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT UNIQUE,
            temperature REAL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        CREATE TABLE IF NOT EXISTS hourly_avg (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date_hour TEXT UNIQUE,
            average REAL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        CREATE TABLE IF NOT EXISTS daily_avg (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT UNIQUE,
            average REAL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

bool addMeasurement(sqlite3* db, const std::string& timestamp, double temperature) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::string sql = "INSERT OR REPLACE INTO measurements (timestamp, temperature) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, temperature);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool addHourlyAverage(sqlite3* db, const std::string& dateHour, double average) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::string sql = "INSERT OR REPLACE INTO hourly_avg (date_hour, average) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, dateHour.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, average);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool addDailyAverage(sqlite3* db, const std::string& date, double average) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::string sql = "INSERT OR REPLACE INTO daily_avg (date, average) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, average);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

void cleanupOldMeasurements(sqlite3* db, const Clock::time_point& cutoffTime) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::string iso = timeToIso(cutoffTime);
    std::string sql = "DELETE FROM measurements WHERE timestamp < ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, iso.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

int main() {
    // Инициализация БД
    sqlite3* db;
    int rc = sqlite3_open("measurements.db", &db);
    
    if (rc) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    initDatabase(db);
    std::cerr << "Logger started, database initialized" << std::endl;
    
    std::deque<Measurement> measurements;
    std::vector<Measurement> hourBuffer;
    std::vector<Measurement> dayBuffer;

    int currentHour = -1;
    int currentDay  = -1;
    int currentYear = -1;

    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string ts;
        double temp;

        ss >> ts >> temp;
        auto tp = parseTime(ts);
        auto tm = toTM(tp);

        if (currentHour == -1) {
            currentHour = tm.tm_hour;
            currentDay  = tm.tm_mday;
            currentYear = tm.tm_year;
        }

        // Добавляем в БД
        addMeasurement(db, ts, temp);
        measurements.push_back({tp, temp});

        // Очистка измерений старше 24 часов
        auto now = Clock::now();
        while (!measurements.empty() &&
               now - measurements.front().time > std::chrono::hours(24)) {
            measurements.pop_front();
        }
        
        // Очистка БД от данных старше месяца
        auto cutoff = now - std::chrono::hours(24 * 30);
        cleanupOldMeasurements(db, cutoff);

        // При смене часа сохраняем среднее за прошлый час
        if (tm.tm_hour != currentHour) {
            if (!hourBuffer.empty()) {
                double sum = 0;
                for (auto& m : hourBuffer) sum += m.value;
                double avg = sum / hourBuffer.size();

                std::tm oldTm = toTM(hourBuffer.front().time);
                char dateHourBuf[32];
                std::strftime(dateHourBuf, sizeof(dateHourBuf), "%Y-%m-%d %H", &oldTm);
                
                addHourlyAverage(db, std::string(dateHourBuf), avg);
            }

            hourBuffer.clear();
            currentHour = tm.tm_hour;
        }
        hourBuffer.push_back({tp, temp});

        // При смене дня сохраняем среднее за прошлый день
        if (tm.tm_mday != currentDay) {
            if (!dayBuffer.empty()) {
                double sum = 0;
                for (auto& m : dayBuffer) sum += m.value;
                double avg = sum / dayBuffer.size();

                std::tm oldTm = toTM(dayBuffer.front().time);
                char dateBuf[32];
                std::strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", &oldTm);
                
                addDailyAverage(db, std::string(dateBuf), avg);
            }

            dayBuffer.clear();
            currentDay = tm.tm_mday;
        }
        dayBuffer.push_back({tp, temp});
    }
    
    sqlite3_close(db);
    return 0;
}
