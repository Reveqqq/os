// API URL
const API_URL = 'http://localhost:8080';
let temperatureChart = null;

// Функция для плавной навигации
function scrollToSection(sectionId) {
    const section = document.getElementById(sectionId);
    if (section) {
        section.scrollIntoView({ behavior: 'smooth' });
        
        // Обновляем активный элемент в боковой панели
        document.querySelectorAll('.nav-item').forEach(item => {
            item.classList.remove('active');
        });
        event.target.closest('.nav-item').classList.add('active');
    }
}

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    // Устанавливаем начальные значения дат
    setDefaultDates();
    
    // Загружаем данные
    loadCurrentTemperature();
    loadStatistics();
    
    // Обновляем текущую температуру каждые 5 секунд
    setInterval(loadCurrentTemperature, 5000);
    
    // Обновляем статистику каждые 30 секунд
    setInterval(loadStatistics, 30000);
    
    // Слушатели для кнопок
    document.getElementById('applyFilters').addEventListener('click', loadStatistics);
    document.getElementById('resetFilters').addEventListener('click', resetFilters);
});

function setDefaultDates() {
    const endDate = new Date();
    const startDate = new Date(endDate.getTime() - 24 * 60 * 60 * 1000); // 24 часа назад
    
    document.getElementById('startDate').value = formatDateTimeLocal(startDate);
    document.getElementById('endDate').value = formatDateTimeLocal(endDate);
}

function formatDateTimeLocal(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    
    return `${year}-${month}-${day}T${hours}:${minutes}`;
}

function formatISO8601(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    const seconds = String(date.getSeconds()).padStart(2, '0');
    
    return `${year}-${month}-${day}T${hours}:${minutes}:${seconds}`;
}

function loadCurrentTemperature() {
    fetch(`${API_URL}/api/current`)
        .then(response => response.json())
        .then(data => {
            if (data.temperature !== undefined) {
                document.getElementById('tempValue').textContent = data.temperature.toFixed(2);
                const date = new Date(data.timestamp);
                document.getElementById('tempTime').textContent = 
                    `Обновлено: ${date.toLocaleString('ru-RU')}`;
                document.getElementById('apiStatus').className = 'status-dot online';
            }
        })
        .catch(error => {
            console.error('Error loading current temperature:', error);
            document.getElementById('apiStatus').className = 'status-dot offline';
        });
}

function loadStatistics() {
    const startDateInput = document.getElementById('startDate').value;
    const endDateInput = document.getElementById('endDate').value;
    
    if (!startDateInput || !endDateInput) {
        alert('Пожалуйста, выберите период времени');
        return;
    }
    
    // Конвертируем в ISO8601 формат
    const startDate = new Date(startDateInput + ':00');
    const endDate = new Date(endDateInput + ':00');
    
    const startISO = formatISO8601(startDate);
    const endISO = formatISO8601(endDate);
    
    const url = `${API_URL}/api/stats?start=${encodeURIComponent(startISO)}&end=${encodeURIComponent(endISO)}`;
    
    fetch(url)
        .then(response => response.json())
        .then(data => {
            if (data.data && data.summary) {
                updateStatistics(data.summary);
                updateChart(data.data);
                updateTable(data.data);
            }
        })
        .catch(error => {
            console.error('Error loading statistics:', error);
        });
}

function updateStatistics(summary) {
    document.getElementById('avgTemp').textContent = summary.average?.toFixed(2) || '--';
    document.getElementById('maxTemp').textContent = summary.max?.toFixed(2) || '--';
    document.getElementById('minTemp').textContent = summary.min?.toFixed(2) || '--';
    document.getElementById('countTemp').textContent = summary.count || 0;
}

function updateChart(data) {
    const ctx = document.getElementById('temperatureChart').getContext('2d');
    
    // Подготавливаем данные для графика
    const labels = data.map(item => {
        const date = new Date(item.timestamp);
        return date.toLocaleTimeString('ru-RU', { 
            hour: '2-digit', 
            minute: '2-digit',
            second: '2-digit'
        });
    });
    
    const temperatures = data.map(item => item.temperature);
    
    // Удаляем старый график если существует
    if (temperatureChart) {
        temperatureChart.destroy();
    }
    
    // Создаем новый график
    temperatureChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [{
                label: 'Температура (°C)',
                data: temperatures,
                borderColor: '#667eea',
                backgroundColor: 'rgba(102, 126, 234, 0.1)',
                borderWidth: 2,
                fill: true,
                tension: 0.4,
                pointRadius: data.length > 100 ? 0 : 4,
                pointBackgroundColor: '#667eea',
                pointBorderColor: '#fff',
                pointBorderWidth: 2,
                pointHoverRadius: 6
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            plugins: {
                legend: {
                    display: true,
                    labels: {
                        color: '#ffffff',
                        font: {
                            size: 12,
                            weight: 'bold'
                        }
                    }
                },
                tooltip: {
                    mode: 'index',
                    intersect: false,
                    callbacks: {
                        label: function(context) {
                            return context.parsed.y.toFixed(2) + ' °C';
                        }
                    }
                }
            },
            scales: {
                y: {
                    beginAtZero: false,
                    title: {
                        display: true,
                        text: 'Температура (°C)'
                    },
                    ticks: {
                        color: '#ffffff'
                    },
                    grid: {
                        color: 'rgba(255, 255, 255, 0.05)'
                    }
                },
                x: {
                    ticks: {
                        color: '#ffffff',
                        maxRotation: 45,
                        minRotation: 0,
                        maxTicksLimit: 10
                    },
                    grid: {
                        color: 'rgba(255, 255, 255, 0.05)'
                    }
                }
            }
        }
    });
}

function updateTable(data) {
    const tbody = document.getElementById('tableBody');
    tbody.innerHTML = '';
    
    // Если много данных, показываем последние 50
    const displayData = data.length > 50 ? data.slice(-50) : data;
    
    displayData.forEach(item => {
        const row = tbody.insertRow();
        const dateCell = row.insertCell(0);
        const tempCell = row.insertCell(1);
        
        const date = new Date(item.timestamp);
        dateCell.textContent = date.toLocaleString('ru-RU');
        tempCell.textContent = item.temperature.toFixed(2) + ' °C';
    });
    
    if (displayData.length === 0) {
        const row = tbody.insertRow();
        row.innerHTML = '<td colspan="2" style="text-align: center; color: #999;">Нет данных</td>';
    }
}

function setLast24Hours() {
    const endDate = new Date();
    const startDate = new Date(endDate.getTime() - 24 * 60 * 60 * 1000);
    
    document.getElementById('startDate').value = formatDateTimeLocal(startDate);
    document.getElementById('endDate').value = formatDateTimeLocal(endDate);
    
    loadStatistics();
}

function setLast7Days() {
    const endDate = new Date();
    const startDate = new Date(endDate.getTime() - 7 * 24 * 60 * 60 * 1000);
    
    document.getElementById('startDate').value = formatDateTimeLocal(startDate);
    document.getElementById('endDate').value = formatDateTimeLocal(endDate);
    
    loadStatistics();
}

function setLast30Days() {
    const endDate = new Date();
    const startDate = new Date(endDate.getTime() - 30 * 24 * 60 * 60 * 1000);
    
    document.getElementById('startDate').value = formatDateTimeLocal(startDate);
    document.getElementById('endDate').value = formatDateTimeLocal(endDate);
    
    loadStatistics();
}

function resetFilters() {
    setDefaultDates();
    loadStatistics();
}
