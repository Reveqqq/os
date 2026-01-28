#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QWidget>
#include <QGroupBox>
#include <QLCDNumber>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStringList>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <iostream>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_date_scale_engine.h>
#include <qwt_date_scale_draw.h>

struct TemperatureData {
    double timestamp;  // seconds since epoch
    double temperature;
};

class TemperatureGUI : public QMainWindow {
    Q_OBJECT

private:
    QwtPlot *plot;
    QLCDNumber *currentTempDisplay;
    QLabel *averageTempDisplay;
    QLabel *statusLabel;
    QDateTimeEdit *startDateEdit;
    QDateTimeEdit *endDateEdit;
    QComboBox *periodComboBox;
    QTimer *updateTimer;

    std::vector<TemperatureData> allData;

public:
    TemperatureGUI(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Temperature Monitor - Temperature Sensor Data Viewer");
        setGeometry(100, 100, 1400, 800);

        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // Top section with current temperature
        QGroupBox *currentTempGroup = new QGroupBox("Current Temperature", this);
        QHBoxLayout *currentLayout = new QHBoxLayout(currentTempGroup);

        QLabel *currentLabel = new QLabel("Current Temperature:", this);
        currentTempDisplay = new QLCDNumber(this);
        currentTempDisplay->setDigitCount(5);
        currentTempDisplay->setSegmentStyle(QLCDNumber::Flat);
        currentTempDisplay->setMinimumWidth(150);

        QLabel *unitLabel = new QLabel("°C", this);
        unitLabel->setStyleSheet("font-size: 18px; font-weight: bold;");

        currentLayout->addWidget(currentLabel);
        currentLayout->addWidget(currentTempDisplay);
        currentLayout->addWidget(unitLabel);
        currentLayout->addStretch();

        mainLayout->addWidget(currentTempGroup);

        // Plot section
        plot = new QwtPlot(this);
        plot->setTitle("Temperature Over Time");
        plot->setCanvasBackground(Qt::white);

        QwtPlotGrid *grid = new QwtPlotGrid();
        grid->attach(plot);

        plot->setAxisTitle(QwtPlot::xBottom, "Time");
        plot->setAxisTitle(QwtPlot::yLeft, "Temperature (°C)");

        // Set up time axis
        plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtDateScaleDraw());
        plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtDateScaleEngine());

        QwtLegend *legend = new QwtLegend();
        plot->insertLegend(legend, QwtPlot::BottomLegend);

        mainLayout->addWidget(plot, 1);

        // Control section
        QGroupBox *controlGroup = new QGroupBox("Date Range Selection", this);
        QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);

        // Period selection
        QHBoxLayout *periodLayout = new QHBoxLayout();
        QLabel *periodLabel = new QLabel("Quick Select:", this);
        periodComboBox = new QComboBox(this);
        periodComboBox->addItem("Last 24 Hours");
        periodComboBox->addItem("Last 7 Days");
        periodComboBox->addItem("Last 30 Days");
        periodComboBox->addItem("Custom Range");

        periodLayout->addWidget(periodLabel);
        periodLayout->addWidget(periodComboBox);
        periodLayout->addStretch();
        controlLayout->addLayout(periodLayout);

        // Custom date range
        QHBoxLayout *dateLayout = new QHBoxLayout();
        QLabel *fromLabel = new QLabel("From:", this);
        startDateEdit = new QDateTimeEdit(this);
        startDateEdit->setDateTime(QDateTime::currentDateTime().addDays(-1));
        startDateEdit->setCalendarPopup(true);

        QLabel *toLabel = new QLabel("To:", this);
        endDateEdit = new QDateTimeEdit(this);
        endDateEdit->setDateTime(QDateTime::currentDateTime());
        endDateEdit->setCalendarPopup(true);

        dateLayout->addWidget(fromLabel);
        dateLayout->addWidget(startDateEdit);
        dateLayout->addWidget(toLabel);
        dateLayout->addWidget(endDateEdit);
        dateLayout->addStretch();
        controlLayout->addLayout(dateLayout);

        mainLayout->addWidget(controlGroup);

        // Statistics section
        QGroupBox *statsGroup = new QGroupBox("Statistics", this);
        QHBoxLayout *statsLayout = new QHBoxLayout(statsGroup);

        QLabel *avgLabel = new QLabel("Average Temperature:", this);
        averageTempDisplay = new QLabel("-- °C", this);
        averageTempDisplay->setStyleSheet("font-size: 14px; font-weight: bold; color: #0066cc;");

        statsLayout->addWidget(avgLabel);
        statsLayout->addWidget(averageTempDisplay);
        statsLayout->addStretch();
        mainLayout->addWidget(statsGroup);

        // Status and buttons
        QHBoxLayout *bottomLayout = new QHBoxLayout();

        statusLabel = new QLabel("Ready", this);
        statusLabel->setStyleSheet("color: #008000;");

        QPushButton *refreshButton = new QPushButton("Refresh Data", this);
        QPushButton *exitButton = new QPushButton("Exit", this);

        connect(refreshButton, &QPushButton::clicked, this, &TemperatureGUI::updatePlot);
        connect(exitButton, &QPushButton::clicked, this, &QMainWindow::close);
        connect(periodComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &TemperatureGUI::onPeriodChanged);

        bottomLayout->addWidget(statusLabel);
        bottomLayout->addStretch();
        bottomLayout->addWidget(refreshButton);
        bottomLayout->addWidget(exitButton);

        mainLayout->addLayout(bottomLayout);

        // Timer for auto-update
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &TemperatureGUI::updatePlot);
        updateTimer->start(5000);  // Update every 5 seconds

        // Initial load
        loadAllData();
        updatePlot();
    }

    ~TemperatureGUI() {
        updateTimer->stop();
    }

private slots:
    void updatePlot() {
        QDateTime startDT = startDateEdit->dateTime();
        QDateTime endDT = endDateEdit->dateTime();

        loadAllData();
        plotData(startDT, endDT);
        updateStatistics(startDT, endDT);
        statusLabel->setText("Updated: " + QDateTime::currentDateTime().toString("hh:mm:ss"));
    }

    void onPeriodChanged(int index) {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime start;

        switch (index) {
            case 0:  // Last 24 Hours
                start = now.addDays(-1);
                break;
            case 1:  // Last 7 Days
                start = now.addDays(-7);
                break;
            case 2:  // Last 30 Days
                start = now.addDays(-30);
                break;
            case 3:  // Custom Range
                return;  // Don't change date edits for custom
            default:
                return;
        }

        startDateEdit->setDateTime(start);
        endDateEdit->setDateTime(now);
        updatePlot();
    }

private:
    std::string findLogFile(const std::string &filename) {
        // Try multiple search paths
        std::vector<std::string> searchPaths = {
            filename,                           // Current directory
            "src/" + filename,                  // src/ subdirectory
            "../src/" + filename,               // parent/src/
        };
        
        // Add home directory path if HOME is set
        const char* home = getenv("HOME");
        if (home) {
            searchPaths.push_back(std::string(home) + "/Documents/University/os/lab6/src/" + filename);
        }
        
        for (const auto &path : searchPaths) {
            std::ifstream test(path);
            if (test.good()) {
                std::cerr << "Found log file at: " << path << std::endl;
                return path;
            }
        }
        
        std::cerr << "WARNING: Could not find " << filename << " in any standard location" << std::endl;
        return filename;  // Return original if not found
    }

    void loadAllData() {
        allData.clear();

        // Find and load from measurements.log (current measurements)
        std::string measFile = findLogFile("measurements.log");
        loadFromFile(measFile);

        // Find and load from hourly_avg.log
        std::string hourlyFile = findLogFile("hourly_avg.log");
        loadHourlyData(hourlyFile);

        // Find and load from daily_avg.log
        std::string dailyFile = findLogFile("daily_avg.log");
        loadDailyData(dailyFile);

        std::cerr << "Total data points loaded: " << allData.size() << std::endl;

        if (allData.empty()) {
            statusLabel->setText("No data available. Is the logger running?");
            statusLabel->setStyleSheet("color: #ff0000;");
        } else {
            statusLabel->setStyleSheet("color: #008000;");
        }
    }

    void loadFromFile(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open: " << filename << std::endl;
            return;
        }

        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string timestamp;
            double temp;

            if (iss >> timestamp >> temp) {
                TemperatureData data;
                data.temperature = temp;

                // Parse ISO 8601 timestamp: YYYY-MM-DDTHH:MM:SS
                std::tm tm = {};
                std::istringstream ts_stream(timestamp);
                ts_stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

                if (ts_stream.fail()) continue;

                data.timestamp = std::mktime(&tm);
                allData.push_back(data);
                count++;
            }
        }
        file.close();
        std::cerr << "Loaded " << count << " entries from " << filename << std::endl;
    }

    void loadHourlyData(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string dateStr;
            int hour;
            double temp;

            if (iss >> dateStr >> hour >> temp) {
                TemperatureData data;
                data.temperature = temp;

                // Parse date: YYYY-MM-DD
                std::tm tm = {};
                std::istringstream date_stream(dateStr);
                date_stream >> std::get_time(&tm, "%Y-%m-%d");

                if (date_stream.fail()) continue;

                tm.tm_hour = hour;
                data.timestamp = std::mktime(&tm);
                allData.push_back(data);
            }
        }
        file.close();
    }

    void loadDailyData(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string dateStr;
            double temp;

            if (iss >> dateStr >> temp) {
                TemperatureData data;
                data.temperature = temp;

                // Parse date: YYYY-MM-DD
                std::tm tm = {};
                std::istringstream date_stream(dateStr);
                date_stream >> std::get_time(&tm, "%Y-%m-%d");

                if (date_stream.fail()) continue;

                tm.tm_hour = 12;  // Noon
                data.timestamp = std::mktime(&tm);
                allData.push_back(data);
            }
        }
        file.close();
    }

    void plotData(const QDateTime &startDT, const QDateTime &endDT) {
        double startTime = startDT.toSecsSinceEpoch();
        double endTime = endDT.toSecsSinceEpoch();

        QwtPlotCurve *curve = new QwtPlotCurve("Temperature");
        curve->setStyle(QwtPlotCurve::Lines);
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

        QVector<QPointF> points;
        for (const auto &data : allData) {
            if (data.timestamp >= startTime && data.timestamp <= endTime) {
                points.append(QPointF(data.timestamp * 1000.0, data.temperature));
            }
        }

        if (points.isEmpty()) {
            statusLabel->setText("No data in selected range");
            return;
        }

        curve->setSamples(points);

        // Style
        QPen pen;
        pen.setColor(Qt::blue);
        pen.setWidth(2);
        curve->setPen(pen);

        QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse, Qt::blue, QPen(Qt::blue), QSize(4, 4));
        curve->setSymbol(symbol);

        // Clear old curves
        plot->detachItems(QwtPlotItem::Rtti_PlotCurve);

        // Attach new curve
        curve->attach(plot);

        // Set scales
        plot->setAxisScale(QwtPlot::xBottom, startTime * 1000.0, endTime * 1000.0);

        if (!points.isEmpty()) {
            double minTemp = points[0].y();
            double maxTemp = points[0].y();
            for (const auto &p : points) {
                minTemp = std::min(minTemp, p.y());
                maxTemp = std::max(maxTemp, p.y());
            }
            double margin = (maxTemp - minTemp) * 0.1;
            plot->setAxisScale(QwtPlot::yLeft, minTemp - margin, maxTemp + margin);
        }

        plot->replot();

        // Update current temperature
        if (!allData.empty()) {
            const auto &latest = allData.back();
            currentTempDisplay->display(latest.temperature);
        }
    }

    void updateStatistics(const QDateTime &startDT, const QDateTime &endDT) {
        double startTime = startDT.toSecsSinceEpoch();
        double endTime = endDT.toSecsSinceEpoch();

        double sum = 0.0;
        int count = 0;

        for (const auto &data : allData) {
            if (data.timestamp >= startTime && data.timestamp <= endTime) {
                sum += data.temperature;
                count++;
            }
        }

        if (count > 0) {
            double average = sum / count;
            averageTempDisplay->setText(QString::number(average, 'f', 2) + " °C");
        } else {
            averageTempDisplay->setText("-- °C");
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TemperatureGUI gui;
    gui.show();
    return app.exec();
}

#include "temperature_gui.moc"
