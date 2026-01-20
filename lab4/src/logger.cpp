#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <vector>
#include <chrono>
#include <iomanip>
#include <ctime>

using Clock = std::chrono::system_clock;

struct Measurement {
    Clock::time_point time;
    double value;
};

Clock::time_point parseTime(const std::string& s) {
    std::tm tm{};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return Clock::from_time_t(std::mktime(&tm));
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

int main() {
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
            std::ofstream("hourly_avg.log", std::ios::app);
            std::ofstream("daily_avg.log", std::ios::app);
            currentHour = tm.tm_hour;
            currentDay  = tm.tm_mday;
            currentYear = tm.tm_year;
        }

        measurements.push_back({tp, temp});

        // Очистка измерений старше 24 часов
        auto now = Clock::now();
        while (!measurements.empty() &&
               now - measurements.front().time > std::chrono::hours(24)) {
            measurements.pop_front();
        }

        // Очистка daily_avg.log старше года
        {
            std::ifstream in("daily_avg.log");
            std::ofstream out("daily_tmp.log");

            std::string line;
            while (std::getline(in, line)) {
                std::tm t{};
                std::istringstream ss(line);
                ss >> std::get_time(&t, "%Y-%m-%d");

                if (!ss.fail()) {
                    // текущий или предыдущий календарный год
                    if (t.tm_year == tm.tm_year ||
                        t.tm_year == tm.tm_year - 1) {
                        out << line << '\n';
                    }
                }
            }

            in.close();
            out.close();
            std::rename("daily_tmp.log", "daily_avg.log");
        }

        // Очистка hourly_avg.log старше месяца
        {
            std::ifstream in("hourly_avg.log");
            std::ofstream out("hourly_tmp.log");
            std::string l;
            while (std::getline(in, l)) {
                std::tm t{};
                std::istringstream s(l);
                s >> std::get_time(&t, "%Y-%m-%d");
                auto tp2 = Clock::from_time_t(std::mktime(&t));
                if (now - tp2 < std::chrono::hours(24 * 30)) {
                    out << l << "\n";
                }
            }
            in.close(); out.close();
            std::rename("hourly_tmp.log", "hourly_avg.log");
        }

        // При смене часа сохраняем среднее за прошлый час
        if (tm.tm_hour != currentHour) {
            if (!hourBuffer.empty()) {
                double sum = 0;
                for (auto& m : hourBuffer) sum += m.value;
                double avg = sum / hourBuffer.size();

                std::ofstream f("hourly_avg.log", std::ios::app);
                std::tm oldTm = toTM(hourBuffer.front().time);

                f << std::put_time(&oldTm, "%Y-%m-%d ")
                << currentHour << " " << avg << "\n";
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

                std::ofstream f("daily_avg.log", std::ios::app);
                std::tm oldTm = toTM(dayBuffer.front().time);
                f << std::put_time(&oldTm, "%Y-%m-%d ")
                << avg << "\n";
            }

            dayBuffer.clear();
            currentDay = tm.tm_mday;
        }
        dayBuffer.push_back({tp, temp});

        // Перезапись measurements.log
        {
            std::ofstream f("measurements.log", std::ios::trunc);
            for (auto& m : measurements) {
                std::tm tm = toTM(m.time);
                f << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
                    << " " << m.value << "\n";

            }
        }
    }
}
