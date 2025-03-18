#include "data.h"
#include <list>

const uint8_t MAX_RESULT = 200;
DataArray<MAX_RESULT, SensorItem> accumulated_average; // Accumulated average (empty list)

// Function to update the accumulated average
void update_average( size_t &n, DataArray<MAX_RESULT, SensorItem> &lastResult)
{
    size_t pos = 0;
    auto it_average = accumulated_average.begin();

    for (SensorItem *item : lastResult)
    {
        if (pos >= accumulated_average.size())
        {
            // If the position does not exist in the accumulated_average, add it
            SensorItem *new_item = accumulated_average.getEmpty();
            new_item->set(0.0f, 0.0f, item->time);
            accumulated_average.push(new_item);
            it_average = std::prev(accumulated_average.end()); // Point to the last element
        }

        // Update the accumulated average
        SensorItem *average = *it_average;
        average->distance = (average->distance * n + item->distance) / (n + 1);
        average->force = (average->force * n + item->force) / (n + 1);
        average->time = item->time;

        // Move to the next position
        ++it_average;
        ++pos;
    }

    // If the new series is shorter, fill with zeros
    while (pos < accumulated_average.size())
    {
        SensorItem *average = *it_average;
        average->distance = (average->distance * n + 0.0f) / (n + 1);
        average->force = (average->force * n + 0.0f) / (n + 1);
        ++it_average;
        ++pos;
    }

    // Increment the counter of processed series
    n++;
}

void update_max( size_t &n, DataArray<MAX_RESULT, SensorItem> &lastResult)
{
    size_t pos = 0;
    auto it_average = accumulated_average.begin();

    for (SensorItem *item : lastResult)
    {
        if (pos >= accumulated_average.size())
        {
            // If the position does not exist in the accumulated_average, add it
            SensorItem *new_item = accumulated_average.getEmpty();
            new_item->set(0.0f, 0.0f, 0);
            accumulated_average.push(new_item);
            it_average = std::prev(accumulated_average.end()); // Point to the last element
        }

        // Update the max value
        SensorItem *average = *it_average;
        average->distance = max(average->distance,item->distance);
        average->force = max(average->force ,item->force);
        average->time = item->time;

        // Move to the next position
        ++it_average;
        ++pos;
    }

    // Increment the counter of processed series
    n++;
}

void print_average()
{
    // Display the result
    Serial.println("Accumulated average:");
    for (SensorItem *item : accumulated_average)
    {
        Serial.print("Distance: ");
        Serial.print(item->distance);
        Serial.print(", Force: ");
        Serial.print(item->force);
        Serial.print(", Time: ");
        Serial.println(item->time);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    // Initialization

    size_t n = 1; // Counter of processed series

    // Data series (lists of SensorItem)
    static DataArray<MAX_RESULT, SensorItem> series_1;
    const char *json1 = R"([
        {"d": 10.0, "f": 25.0,"t": 100},
        {"d": 15.0, "f": 7.0,"t": 200}
    ])";
    accumulated_average.deserializeData(json1);
    //update_average(n, series_1);
    //updateAvg(n, series_1);

    print_average();

    const char *json2 = R"([
        {"d": 20.0, "f": 10.0,"t": 100}
    ])";
    series_1.deserializeData(json2);
    //update_average(n, series_1);
    update_max(n, series_1);

    print_average();

    const char *json3 = R"([
        {"d": 15.0, "f": 15.0,"t": 100},
        {"d": 35.0, "f": 20.0,"t": 200},
        {"d": 40.0, "f": 25.0,"t": 300}
    ])";
    series_1.deserializeData(json3);
    //update_average(n, series_1);
    update_max(n, series_1);

    print_average();

}

void loop()
{
    // Nothing to do here
}

