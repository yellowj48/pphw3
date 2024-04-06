#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>

#define THREAD_COUNT 8
#define MINUTES 60
#define HOURS 1 // For demonstration purposes, reduce hours to 1

std::mutex mutex;
std::vector<int> sensorReadings(THREAD_COUNT * MINUTES);
std::vector<bool> sensorsReady(THREAD_COUNT, false);

std::random_device rd;
std::mt19937 gen(rd());

# define M_PI 3.14159265358979323846

bool allSensorsReady(int caller) {
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (!sensorsReady[i] && i != caller) {
            return false;
        }
    }
    return true;
}

int generateRandomTemperature(int hour) {
    // Simulate daily variability in mean temperature
    double dailyMean = -20 + (std::sin(hour / 24.0 * 2 * M_PI) * 20); // Mean temperature fluctuates between -40 and 0 degrees
    std::normal_distribution<> distr(dailyMean, 15); // Standard deviation of 15 to simulate fluctuation
    
    int temp = std::round(distr(gen));
    temp = std::max(-100, temp); // Ensuring temperature is not below -100
    temp = std::min(70, temp); // Ensuring temperature is not above 70
    
    return temp;
}

void sensorThreadFunction(int sensorId) {
    for (int hour = 0; hour < HOURS; hour++) {
        for (int minute = 0; minute < MINUTES; minute++) {
            sensorsReady[sensorId] = false;
            int temp = generateRandomTemperature(hour);
            sensorReadings[minute + (sensorId * MINUTES)] = temp;
            sensorsReady[sensorId] = true;

            // Ensure all sensors are ready before proceeding
            while (!allSensorsReady(sensorId)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}

void generateReport(int hour) {
    std::cout << "[Hour " << hour + 1 << " Report]" << std::endl;
    // Sorting is required to efficiently find the top and bottom temperatures
    std::vector<int> sortedReadings = sensorReadings;
    std::sort(sortedReadings.begin(), sortedReadings.end());
    
    std::cout << "Highest temperatures: ";
    for (int i = sortedReadings.size() - 1; i >= 0 && i >= sortedReadings.size() - 5; i--) {
        std::cout << sortedReadings[i] << "F ";
    }
    std::cout << std::endl << "Lowest temperatures: ";
    for (int i = 0; i < sortedReadings.size() && i < 5; i++) {
        std::cout << sortedReadings[i] << "F ";
    }
    std::cout << std::endl;
}

void reportThreadFunction() {
    for (int hour = 0; hour < HOURS; hour++) {
        // Wait for all sensors to finish the hour's readings
        while (!allSensorsReady(-1)) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::lock_guard<std::mutex> lock(mutex);
        generateReport(hour);
    }
}

int main() {
    std::thread sensorThreads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        sensorThreads[i] = std::thread(sensorThreadFunction, i);
    }
    std::thread reportThread(reportThreadFunction);
    
    for (auto& t : sensorThreads) {
        t.join();
    }
    reportThread.join();
    
    return 0;
}
