#include "data.h"
#include <list>

const uint8_t MAX_RESULT = 200;
DataList<MAX_RESULT, SensorItem> accumulated_average; // Accumulated average (empty list)

// Function to update the accumulated average
void update_average( size_t &n, DataList<MAX_RESULT, SensorItem> &new_series)
{
    size_t pos = 0;
    auto it_average = accumulated_average.begin();

    for (SensorItem *item : new_series)
    {
        if (pos >= accumulated_average.size())
        {
            // If the position does not exist in the accumulated_average, add it
            SensorItem *new_item = accumulated_average.getEmpty();
            new_item->set(0.0f, 0.0f, 0);
            accumulated_average.push(new_item);
            it_average = std::prev(accumulated_average.end()); // Point to the last element
        }

        // Update the accumulated average
        SensorItem *average = *it_average;
        average->distance = (average->distance * n + item->distance) / (n + 1);
        average->force = (average->force * n + item->force) / (n + 1);
        average->time = (average->time * n + item->time) / (n + 1);

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
        average->time = (average->time * n + 0) / (n + 1);
        ++it_average;
        ++pos;
    }

    // Increment the counter of processed series
    n++;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    // Initialization

    size_t n = 0; // Counter of processed series

    // Data series (lists of SensorItem)
    static DataList<MAX_RESULT, SensorItem> series_1;
    const char *json1 = R"([
        {"d": 10.0, "f": 5.0,"t": 100},
        {"d": 15.0, "f": 7.0,"t": 150}
    ])";
    series_1.deserializeData(json1);
    // Update the average with each series
    update_average(n, series_1);

    const char *json2 = R"([
        {"d": 20.0, "f": 10.0,"t": 200}
    ])";
    series_1.deserializeData(json2);
    update_average(n, series_1);

    const char *json3 = R"([
        {"d": 30.0, "f": 15.0,"t": 300},
        {"d": 35.0, "f": 20.0,"t": 350},
        {"d": 40.0, "f": 25.0,"t": 400}
    ])";
    series_1.deserializeData(json3);
    update_average(n, series_1);

/*output:
Accumulated average:
Distance: 20.00, Force: 10.00, Time: 200
Distance: 16.67, Force: 9.00, Time: 166
Distance: 13.33, Force: 8.33, Time: 133
*/
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

void loop()
{
    // Nothing to do here
}

