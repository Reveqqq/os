#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <thread>
#include <mutex>
#include <fstream>
#include <signal.h>
#include <algorithm>

// Кроссплатформенная поддержка сокетов
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    typedef int socklen_t;
    #define close(sock) closesocket(sock)
    #define ssize_t int
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

// Глобальная переменная для завершения сервера
volatile bool running = true;

// Мьютекс для синхронизации доступа к БД
std::mutex db_mutex;

// Кроссплатформенная функция для преобразования времени
std::string getCurrentTime() {
    std::time_t t = std::time(nullptr);
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

// Структура для измерения
struct Measurement {
    std::string timestamp;
    double temperature;
};

// Инициализация базы данных
void initDatabase(sqlite3* db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS measurements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT UNIQUE,
            temperature REAL,
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

// Добавление измерения в БД
bool addMeasurement(sqlite3* db, const std::string& timestamp, double temperature) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::string sql = "INSERT OR REPLACE INTO measurements (timestamp, temperature) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement" << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, temperature);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

// Получение последней температуры
bool getLastTemperature(sqlite3* db, std::string& timestamp, double& temperature) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::string sql = "SELECT timestamp, temperature FROM measurements ORDER BY created_at DESC LIMIT 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        temperature = sqlite3_column_double(stmt, 1);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found;
}

// Получение статистики за период
std::vector<Measurement> getStatistics(sqlite3* db, const std::string& startTime, const std::string& endTime) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<Measurement> results;
    std::string sql = R"(
        SELECT timestamp, temperature FROM measurements 
        WHERE timestamp >= ? AND timestamp <= ? 
        ORDER BY timestamp ASC
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return results;
    }
    
    sqlite3_bind_text(stmt, 1, startTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, endTime.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Measurement m;
        m.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        m.temperature = sqlite3_column_double(stmt, 1);
        results.push_back(m);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

// Генерирование JSON ответа
std::string generateJsonResponse(const std::string& data) {
    return data;
}

// Чтение статического файла
std::string readStaticFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string getContentType(const std::string& filename) {
    if (filename.find(".html") != std::string::npos) return "text/html";
    if (filename.find(".css") != std::string::npos) return "text/css";
    if (filename.find(".js") != std::string::npos) return "application/javascript";
    if (filename.find(".json") != std::string::npos) return "application/json";
    if (filename.find(".png") != std::string::npos) return "image/png";
    if (filename.find(".jpg") != std::string::npos) return "image/jpeg";
    if (filename.find(".gif") != std::string::npos) return "image/gif";
    if (filename.find(".svg") != std::string::npos) return "image/svg+xml";
    return "text/plain";
}

// Парсинг HTTP запроса и получение пути с параметрами
std::pair<std::string, std::string> parseHttpRequest(const std::string& request) {
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    // Отделяем путь от query string
    size_t queryPos = path.find('?');
    std::string queryString = (queryPos != std::string::npos) ? path.substr(queryPos + 1) : "";
    std::string cleanPath = (queryPos != std::string::npos) ? path.substr(0, queryPos) : path;
    
    return {cleanPath, queryString};
}

// Обработчик HTTP запроса
std::string handleHttpRequest(sqlite3* db, const std::string& request) {
    auto [path, queryString] = parseHttpRequest(request);
    
    std::string response;
    
    if (path.find("/api/current") != std::string::npos) {
        // API: текущая температура
        std::string timestamp;
        double temperature;
        
        if (getLastTemperature(db, timestamp, temperature)) {
            std::ostringstream json;
            json << std::fixed << std::setprecision(2);
            json << "{"
                 << "\"timestamp\":\"" << timestamp << "\","
                 << "\"temperature\":" << temperature
                 << "}";
            response = json.str();
        } else {
            response = "{\"error\":\"No data\"}";
        }
    }
    else if (path.find("/api/stats") != std::string::npos) {
        // API: статистика за период
        std::string startTime = "2000-01-01T00:00:00";
        std::string endTime = "2100-01-01T00:00:00";
        
        // Парсим параметры (упрощенный парсер)
        size_t startPos = queryString.find("start=");
        size_t endPos = queryString.find("end=");
        
        if (startPos != std::string::npos) {
            startTime = queryString.substr(startPos + 6, 19);
        }
        if (endPos != std::string::npos) {
            endPos = queryString.find("&", endPos);
            if (endPos == std::string::npos) {
                endTime = queryString.substr(queryString.find("end=") + 4, 19);
            } else {
                endTime = queryString.substr(queryString.find("end=") + 4, endPos - (queryString.find("end=") + 4));
            }
        }
        
        auto stats = getStatistics(db, startTime, endTime);
        
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        json << "{\"data\":[";
        
        for (size_t i = 0; i < stats.size(); ++i) {
            if (i > 0) json << ",";
            json << "{"
                 << "\"timestamp\":\"" << stats[i].timestamp << "\","
                 << "\"temperature\":" << stats[i].temperature
                 << "}";
        }
        
        // Вычисляем статистику
        if (!stats.empty()) {
            double sum = 0, minT = stats[0].temperature, maxT = stats[0].temperature;
            for (const auto& m : stats) {
                sum += m.temperature;
                minT = std::min(minT, m.temperature);
                maxT = std::max(maxT, m.temperature);
            }
            double avg = sum / stats.size();
            
            json << "],\"summary\":{"
                 << "\"count\":" << stats.size() << ","
                 << "\"average\":" << avg << ","
                 << "\"min\":" << minT << ","
                 << "\"max\":" << maxT
                 << "}}";
        } else {
            json << "],\"summary\":{\"count\":0}}";
        }
        
        response = json.str();
    }
    else if (path == "/" || path == "/index.html") {
        // Главная страница
        std::string content = readStaticFile("index.html");
        if (!content.empty()) {
            return "HTML:" + content; // Используем префикс для обозначения HTML
        }
        response = "<!DOCTYPE html><html><body><h1>Not Found</h1></body></html>";
    }
    else if (path == "/style.css") {
        std::string content = readStaticFile("style.css");
        if (!content.empty()) {
            return "CSS:" + content;
        }
        response = "";
    }
    else if (path == "/script.js") {
        std::string content = readStaticFile("script.js");
        if (!content.empty()) {
            return "JS:" + content;
        }
        response = "";
    }
    else {
        response = "{\"error\":\"Not found\"}";
    }
    
    return response;
}

// Отправка HTTP ответа
void sendHttpResponse(int client_socket, const std::string& body, const std::string& contentType = "application/json") {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << contentType << "; charset=utf-8\r\n"
             << "Content-Length: " << body.length() << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
    
    std::string resp = response.str();
    int bytesSent = 0;
    int totalBytes = resp.length();
    while (bytesSent < totalBytes) {
        int ret = send(client_socket, resp.c_str() + bytesSent, totalBytes - bytesSent, 0);
        if (ret < 0) break;
        bytesSent += ret;
    }
}

// Обработчик клиента
void handleClient(int client_socket, sqlite3* db) {
    char buffer[8192] = {0};
    ssize_t bytesRead = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        std::string request(buffer);
        std::string response = handleHttpRequest(db, request);
        
        // Проверяем тип ответа по префиксу
        if (response.substr(0, 5) == "HTML:") {
            std::string content = response.substr(5);
            sendHttpResponse(client_socket, content, "text/html");
        } else if (response.substr(0, 4) == "CSS:") {
            std::string content = response.substr(4);
            sendHttpResponse(client_socket, content, "text/css");
        } else if (response.substr(0, 3) == "JS:") {
            std::string content = response.substr(3);
            sendHttpResponse(client_socket, content, "application/javascript");
        } else {
            sendHttpResponse(client_socket, response, "application/json");
        }
    }
    
    close(client_socket);
}

void signalHandler(int sig) {
    running = false;
}

int main() {
    // Инициализация Winsock для Windows
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    // Инициализация БД
    sqlite3* db;
    int rc = sqlite3_open("measurements.db", &db);
    
    if (rc) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    initDatabase(db);
    std::cout << "Database initialized" << std::endl;
    
    // Создание сокета
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Socket creation error" << std::endl;
        sqlite3_close(db);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    // Разрешение переиспользования адреса
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    // Привязка сокета
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(server_socket);
        sqlite3_close(db);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    // Прослушивание
    listen(server_socket, 5);
    std::cout << "Server listening on port 8080" << std::endl;
    
    // Обработчик сигнала для корректного завершения
    signal(SIGINT, signalHandler);
    
    // Основной цикл сервера
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket < 0) {
            if (!running) break;
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        // Обработка клиента в отдельном потоке
        std::thread(&handleClient, client_socket, db).detach();
    }
    
    close(server_socket);
    sqlite3_close(db);
    std::cout << "Server stopped" << std::endl;
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
