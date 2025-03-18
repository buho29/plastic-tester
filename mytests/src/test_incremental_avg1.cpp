#include "data.h"
#include <vector>
#include <algorithm>

struct SensorData
{
    int time = 0;
    float distance = 0.0f;
    float force = 0.0f;
};

const uint TEST_MAX_TIME = 20000;
const uint TEST_STEP_TIME = 50;

const uint MAX_DATA = TEST_MAX_TIME / TEST_STEP_TIME;
SensorData raw_data[MAX_DATA];

void clear_data()
{
    for (SensorData &item : raw_data)
    {
        item.time = 0;
        item.distance = 0.0f;
        item.force = 0.0f;
    }
}

const uint8_t MAX_RESULT = 200;
DataArray<MAX_RESULT, SensorItem> accumulated_data;

const int REL_TIMES[] = {-300, -250, -200, -150, -100, -50, 0, 50, 100, 150, 200};
const int NUM_REL_TIMES = sizeof(REL_TIMES) / sizeof(int);

size_t num_tests = 0;

const uint8_t MAX_RESULT = 200;
DataArray<MAX_RESULT, SensorItem> accumulated_data;

// Función para detectar la ruptura
size_t detect_rupture(DataArray<MAX_RESULT, SensorItem> &data)
{
    float max_force = 0;
    size_t rupture_index = 0;
    for (size_t i = 0; i < data.size(); ++i)
    {
        if (data[i]->force > max_force)
        {
            max_force = data[i]->force;
            rupture_index = i;
        }
    }
    return rupture_index;
}

// Función para calcular distancia interpolada/extrapolada
float calculate_distance(DataArray<MAX_RESULT, SensorItem> &data, int target_time)
{
    if (data.size() < 2)
        return 0.0f;

    SensorItem *prev = nullptr;
    SensorItem *next = nullptr;

    for (SensorItem *item : data)
    {
        if (item->time <= target_time)
            prev = item;
        if (item->time > target_time && !next)
        {
            next = item;
            break;
        }
    }

    if (!prev)
    {
        // Extrapolar hacia atrás
        float slope = (data[1]->distance - data[0]->distance) / (data[1]->time - data[0]->time);
        return data[0]->distance - slope * (data[0]->time - target_time);
    }
    else if (!next)
    {
        // Extrapolar hacia adelante
        float slope = (data[data.size() - 1]->distance - data[data.size() - 2]->distance) /
                      (data[data.size() - 1]->time - data[data.size() - 2]->time);
        return data[data.size() - 1]->distance + slope * (target_time - data[data.size() - 1]->time);
    }
    else
    {
        // Interpolar
        float slope = (next->distance - prev->distance) / (next->time - prev->time);
        return prev->distance + slope * (target_time - prev->time);
    }
}

float calculate_force(DataArray<MAX_RESULT, SensorItem> &data, int target_time)
{
    if (data.size() < 2)
        return 0.0f;

    SensorItem *prev = nullptr;
    SensorItem *next = nullptr;

    for (SensorItem *item : data)
    {
        if (item->time <= target_time)
            prev = item;
        if (item->time > target_time && !next)
        {
            next = item;
            break;
        }
    }

    if (!prev)
    {
        // Extrapolar hacia atrás
        float slope = (data[1]->force - data[0]->force) / (data[1]->time - data[0]->time);
        return std::max(0.0f, data[0]->force + slope * (target_time - data[0]->time));
    }
    else if (!next)
    {
        // Extrapolar hacia adelante
        float slope = (data[data.size() - 1]->force - data[data.size() - 2]->force) /
                      (data[data.size() - 1]->time - data[data.size() - 2]->time);
        return std::max(0.0f, data[data.size() - 1]->force + slope * (target_time - data[data.size() - 1]->time));
    }
    else
    {
        // Interpolar
        float slope = (next->force - prev->force) / (next->time - prev->time);
        return std::max(0.0f, prev->force + slope * (target_time - prev->time));
    }
}

void align_and_accumulate(DataArray<MAX_RESULT, SensorItem> &new_data)
{
    size_t rupture_index = detect_rupture(new_data);
    int rupture_time = new_data[rupture_index]->time;

    for (int i = 0; i < NUM_REL_TIMES; ++i)
    {
        int rel_time = REL_TIMES[i];
        int target_time = rupture_time + rel_time;

        float distance = calculate_distance(new_data, target_time);
        float force = calculate_force(new_data, target_time);

        SensorItem *acc_item = nullptr;
        for (SensorItem *item : accumulated_data)
        {
            if (item->time == rel_time)
            {
                acc_item = item;
                break;
            }
        }

        if (!acc_item)
        {
            acc_item = accumulated_data.getEmpty();
            acc_item->time = rel_time;
            acc_item->distance = distance;
            acc_item->force = force;
            acc_item->min = force;
            acc_item->max = force;
            accumulated_data.push(acc_item);
        }
        else
        {
            // Actualizar media acumulada
            acc_item->distance = (acc_item->distance * num_tests + distance) / (num_tests + 1);
            acc_item->force = (acc_item->force * num_tests + force) / (num_tests + 1);

            // Actualizar max
            acc_item->max = std::max(acc_item->max, force);

            // Actualizar min, buscando el siguiente valor no nulo si es 0
            if (acc_item->min == 0 || (force > 0 && force < acc_item->min))
            {
                acc_item->min = force;
            }
            // Si después de actualizar sigue siendo 0, buscar el siguiente mínimo no nulo
            if (acc_item->min == 0)
            {
                float next_min = std::numeric_limits<float>::max();
                for (SensorItem *item : accumulated_data)
                {
                    if (item->time == rel_time && item->force > 0 && item->force < next_min)
                    {
                        next_min = item->force;
                    }
                }
                if (next_min != std::numeric_limits<float>::max())
                {
                    acc_item->min = next_min;
                }
            }
        }
    }

    num_tests++;
}

void print_stats()
{
    Serial.println("\n--- Estadísticas Actualizadas ---");
    Serial.println("TiempoRel | AvgDist | AvgForce (Min-Max)");
    for (SensorItem *item : accumulated_data)
    {
        Serial.printf("%9d | %7.2f | %7.2f (%7.2f-%7.2f)\n",
                      item->time,
                      item->distance,
                      item->force, item->min, item->max);
    }
    Serial.printf("Número de pruebas realizadas: %d\n", num_tests);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    DataArray<MAX_RESULT, SensorItem> test_data;

    // Primera prueba
    const char *json1 = R"([
        {"d":12.0,"f":27.0,"t":100},
        {"d":18.0,"f":33.0,"t":200},
        {"d":24.0,"f":10.0,"t":300}
    ])";
    test_data.deserializeData(json1);
    align_and_accumulate(test_data);
    print_stats();

    // Segunda prueba
    const char *json2 = R"([
        {"d":5.0,"f":20.0,"t":100},
        {"d":10.0,"f":25.0,"t":200},
        {"d":15.0,"f":30.0,"t":300},
        {"d":20.0,"f":35.0,"t":400},
        {"d":25.0,"f":10.0,"t":500},
        {"d":30.0,"f":2.0,"t":600}
    ])";
    test_data.deserializeData(json2);
    align_and_accumulate(test_data);
    print_stats();

    // Tercera prueba
    const char *json3 = R"([
        {"d":15.0,"f":22.0,"t":100},
        {"d":20.0,"f":25.0,"t":200},
        {"d":25.0,"f":28.0,"t":300},
        {"d":30.0,"f":31.0,"t":400},
        {"d":35.0,"f":15.0,"t":500},
        {"d":40.0,"f":7.0,"t":600},
        {"d":45.0,"f":2.0,"t":700}
    ])";
    test_data.deserializeData(json3);
    align_and_accumulate(test_data);

    print_stats();
}

void loop() {}
