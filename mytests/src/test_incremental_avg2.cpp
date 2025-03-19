#include "data.h"
#include <algorithm>

const uint TEST_MAX_TIME = 20000;

const uint TEST_STEP_TIME = 100;

const int TEST_START_TIME = -300;
const int TEST_END_TIME = 300;

const uint8_t MAX_RESULT = ((TEST_END_TIME - TEST_START_TIME) / TEST_STEP_TIME) + 1;
DataArray<MAX_RESULT, SensorItem> accumulated_data;

const uint MAX_RAW_DATA = TEST_MAX_TIME / TEST_STEP_TIME;
DataArray<MAX_RAW_DATA, SensorItem> test_data;

// Función para detectar la ruptura
size_t detect_rupture()
{
    float max_force = 0;
    size_t rupture_index = 0;
    for (size_t i = 0; i < test_data.size(); ++i)
    {
        if (test_data[i]->force > max_force)
        {
            max_force = test_data[i]->force;
            rupture_index = i;
        }
    }
    return rupture_index;
}

// Función para calcular distancia interpolada/extrapolada
float calculate_distance(int target_time)
{
    if (test_data.size() < 2)
        return 0.0f;

    SensorItem *prev = nullptr;
    SensorItem *next = nullptr;

    for (SensorItem *item : test_data)
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
        float slope = (test_data[1]->distance - test_data[0]->distance) / (test_data[1]->time - test_data[0]->time);
        return test_data[0]->distance - slope * (test_data[0]->time - target_time);
    }
    else if (!next)
    {
        // Extrapolar hacia adelante
        float slope = (test_data[test_data.size() - 1]->distance - test_data[test_data.size() - 2]->distance) /
                      (test_data[test_data.size() - 1]->time - test_data[test_data.size() - 2]->time);
        return test_data[test_data.size() - 1]->distance + slope * (target_time - test_data[test_data.size() - 1]->time);
    }
    else
    {
        // Interpolar
        float slope = (next->distance - prev->distance) / (next->time - prev->time);
        return prev->distance + slope * (target_time - prev->time);
    }
}
// Función para calcular fuerza interpolada/extrapolada
float calculate_force(int target_time)
{
    if (test_data.size() < 2)
        return 0.0f;

    SensorItem *prev = nullptr;
    SensorItem *next = nullptr;

    for (SensorItem *item : test_data)
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
        float slope = (test_data[1]->force - test_data[0]->force) / (test_data[1]->time - test_data[0]->time);
        return std::max(0.0f, test_data[0]->force + slope * (target_time - test_data[0]->time));
    }
    else if (!next)
    {
        // Extrapolar hacia adelante
        float slope = (test_data[test_data.size() - 1]->force - test_data[test_data.size() - 2]->force) /
                      (test_data[test_data.size() - 1]->time - test_data[test_data.size() - 2]->time);
        return std::max(0.0f, test_data[test_data.size() - 1]->force + slope * (target_time - test_data[test_data.size() - 1]->time));
    }
    else
    {
        // Interpolar
        float slope = (next->force - prev->force) / (next->time - prev->time);
        return std::max(0.0f, prev->force + slope * (target_time - prev->time));
    }
}

void align_and_accumulate(size_t num_tests = 0)
{
    size_t rupture_index = detect_rupture();
    int rupture_time = test_data[rupture_index]->time;

    for (int rel_time = TEST_START_TIME; rel_time <= TEST_END_TIME; rel_time += TEST_STEP_TIME) 
    {
        int target_time = rupture_time + rel_time;

        float distance = calculate_distance(target_time);
        float force = calculate_force(target_time);

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
}

void print_stats(size_t num_tests = 0)
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

    // Primera prueba
    /*const char* json1 = R"([
        {"d":12.0,"f":27.0,"t":100},
        {"d":18.0,"f":33.0,"t":200},
        {"d":24.0,"f":10.0,"t":300}
    ])";*/
    const char *json1 = R"([
 {"d":0.20,"f":0.51,"t":100},
 {"d":0.37,"f":1.47,"t":200},
 {"d":0.53,"f":2.27,"t":300},
 {"d":0.67,"f":3.04,"t":400},
 {"d":0.78,"f":3.50,"t":500},
 {"d":0.88,"f":3.80,"t":600},
 {"d":0.99,"f":4.13,"t":700},
 {"d":1.09,"f":4.33,"t":800},
 {"d":1.19,"f":4.64,"t":900},
 {"d":1.29,"f":4.63,"t":1000},
 {"d":1.40,"f":4.65,"t":1100},
 {"d":1.50,"f":4.66,"t":1200},
 {"d":1.60,"f":4.62,"t":1300},
 {"d":1.70,"f":4.58,"t":1400},
 {"d":1.80,"f":4.58,"t":1500},
 {"d":1.90,"f":4.57,"t":1600},
 {"d":2.01,"f":4.38,"t":1700},
 {"d":2.11,"f":4.17,"t":1800},
 {"d":2.21,"f":4.04,"t":1900},
 {"d":2.31,"f":3.98,"t":2000},
 {"d":2.42,"f":3.78,"t":2100},
 {"d":2.52,"f":3.59,"t":2200},
 {"d":2.62,"f":3.60,"t":2300},
 {"d":2.72,"f":3.70,"t":2400},
 {"d":2.82,"f":3.84,"t":2500},
 {"d":2.92,"f":4.08,"t":2600},
 {"d":3.03,"f":4.29,"t":2700},
 {"d":3.13,"f":4.39,"t":2800},
 {"d":3.23,"f":4.35,"t":2900},
 {"d":3.33,"f":4.23,"t":3000},
 {"d":3.44,"f":4.02,"t":3100},
 {"d":3.54,"f":3.71,"t":3200},
 {"d":3.64,"f":3.51,"t":3300},
 {"d":3.74,"f":3.28,"t":3400},
 {"d":3.84,"f":2.88,"t":3500},
 {"d":3.94,"f":1.73,"t":3600},
 {"d":4.05,"f":0.48,"t":3700}
    ])";
    test_data.deserializeData(json1);
    align_and_accumulate(0);
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
    align_and_accumulate(1);
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
    align_and_accumulate(2);

    print_stats();
}

void loop() {}
