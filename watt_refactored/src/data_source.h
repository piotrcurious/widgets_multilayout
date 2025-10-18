#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <Arduino.h>

// Centralized data management for all dynamic values
class DataSource {
public:
    // Constructor
    DataSource() : watts(0), volts(0), amperes(0), wattHours(0), lastWhCalculationTime(0) {}

    // Update all sensor data
    void update() {
        // Simulate sensor readings
        volts = 220.0 + (rand() % 20 - 10);  // 210-230V
        amperes = 5.0 + (rand() % 10 / 10.0 - 0.5); // 4.5-5.5A
        watts = volts * amperes;

        // Update watt-hours
        updateWattHours();
    }

    // Getters for individual values
    float getWatts() const { return watts; }
    float getVolts() const { return volts; }
    float getAmperes() const { return amperes; }
    float getWattHours() const { return wattHours; }

private:
    float watts;
    float volts;
    float amperes;
    float wattHours;
    uint32_t lastWhCalculationTime;

    // Update watt-hours based on time elapsed
    void updateWattHours() {
        uint32_t currentTime = millis();
        if (lastWhCalculationTime == 0) {
            lastWhCalculationTime = currentTime;
            return;
        }

        float deltaTime = (currentTime - lastWhCalculationTime) / 3600000.0; // ms to hours
        wattHours += watts * deltaTime;
        lastWhCalculationTime = currentTime;
    }
};

#endif // DATA_SOURCE_H