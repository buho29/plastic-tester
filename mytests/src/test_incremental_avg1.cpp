#include "data.h"
#include <algorithm>

struct SensorData
{
    int time = 0;
    float distance = 0.0f;
    float force = 0.0f;
    void set(float distance, float force, int time)
    {
        this->distance = distance;
        this->force = force;
        this->time = time;
    };
};

const uint TEST_MAX_TIME = 20000;
const uint TEST_STEP_TIME = 100;

const uint MAX_DATA = TEST_MAX_TIME / TEST_STEP_TIME;
SensorData raw_data[MAX_DATA];
uint count_raw_data = 0;

void clear_raw_data()
{
    for (SensorData &item : raw_data)
    {
        item.time = 0;
        item.distance = 0.0f;
        item.force = 0.0f;
    }
    count_raw_data = 0;
}
const int REL_TIMES[] = {-300, -200, -100, 0, 100, 200, 300};
// const int REL_TIMES[] = {-300, -250, -200, -150, -100, -50, 0, 50, 100, 150, 200};
const uint NUM_REL_TIMES = sizeof(REL_TIMES) / sizeof(int);

const uint8_t MAX_RESULT = 200; // NUM_REL_TIMES;
DataArray<MAX_RESULT, SensorItem> lastResult;

size_t num_tests = 0;

// Función para detectar la ruptura
size_t detect_rupture()
{
    float max_force = 0;
    size_t rupture_index = 0;
    for (size_t i = 0; i < count_raw_data; ++i)
    {
        if (raw_data[i].force > max_force)
        {
            max_force = raw_data[i].force;
            rupture_index = i;
        }
    }
    return rupture_index;
}

// Función para calcular distancia interpolada/extrapolada
float calculate_distance(int target_time)
{
    if (count_raw_data < 2)
        return 0.0f;

    SensorData *prev = nullptr;
    SensorData *next = nullptr;

    for (SensorData &item : raw_data)
    {
        if (item.time <= target_time)
            prev = &item;
        if (item.time > target_time && !next)
        {
            next = &item;
            break;
        }
    }

    if (!prev)
    {
        // Extrapolar hacia atrás
        float slope = (raw_data[1].distance - raw_data[0].distance) / (raw_data[1].time - raw_data[0].time);
        return raw_data[0].distance - slope * (raw_data[0].time - target_time);
    }
    else if (!next)
    {
        // Extrapolar hacia adelante
        float slope = (raw_data[count_raw_data - 1].distance - raw_data[count_raw_data - 2].distance) /
                      (raw_data[count_raw_data - 1].time - raw_data[count_raw_data - 2].time);
        return raw_data[count_raw_data - 1].distance + slope * (target_time - raw_data[count_raw_data - 1].time);
    }
    else
    {
        // Interpolar
        float slope = (next->distance - prev->distance) / (next->time - prev->time);
        return prev->distance + slope * (target_time - prev->time);
    }
}

float calculate_force(int target_time)
{
    if (count_raw_data < 2)
        return 0.0f;

    SensorData *prev = nullptr;
    SensorData *next = nullptr;

    for (SensorData &item : raw_data)
    {
        if (item.time <= target_time)
            prev = &item;
        if (item.time > target_time && !next)
        {
            next = &item;
            break;
        }
    }

    if (!prev)
    {
        // Extrapolar hacia atrás
        float slope = (raw_data[1].force - raw_data[0].force) / (raw_data[1].time - raw_data[0].time);
        return std::max(0.0f, raw_data[0].force + slope * (target_time - raw_data[0].time));
    }
    else if (!next)
    {
        // Extrapolar hacia adelante
        float slope = (raw_data[count_raw_data - 1].force - raw_data[count_raw_data - 2].force) /
                      (raw_data[count_raw_data - 1].time - raw_data[count_raw_data - 2].time);
        return std::max(0.0f, raw_data[count_raw_data - 1].force + slope * (target_time - raw_data[count_raw_data - 1].time));
    }
    else
    {
        // Interpolar
        float slope = (next->force - prev->force) / (next->time - prev->time);
        return std::max(0.0f, prev->force + slope * (target_time - prev->time));
    }
}

void align_and_accumulate()
{
    size_t rupture_index = detect_rupture();
    int rupture_time = raw_data[rupture_index].time;

    for (int i = 0; i < NUM_REL_TIMES; ++i)
    {
        int rel_time = REL_TIMES[i];
        int target_time = rupture_time + rel_time;

        float distance = calculate_distance(target_time);
        float force = calculate_force(target_time);

        SensorItem *acc_item = nullptr;
        for (SensorItem *item : lastResult)
        {
            if (item->time == rel_time)
            {
                acc_item = item;
                break;
            }
        }

        if (!acc_item)
        {
            acc_item = lastResult.getEmpty();
            acc_item->time = rel_time;
            acc_item->distance = distance;
            acc_item->force = force;
            acc_item->min = force;
            acc_item->max = force;
            lastResult.push(acc_item);
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
                for (SensorItem *item : lastResult)
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
    for (SensorItem *item : lastResult)
    {
        Serial.printf("%9d | %7.2f | %7.2f (%7.2f-%7.2f)\n",
                      item->time,
                      item->distance,
                      item->force, item->min, item->max);
    }
    Serial.printf("Número de pruebas realizadas: %d\n", num_tests);
}


void print_raw_data()
{
	for (size_t i = 0; i < count_raw_data; i++)
	{
        //{"d":5.0,"f":20.0,"t":100},
		Serial.printf(" {\"d\":%.2f,\"f\":%.2f,\"t\":%d},\n",
			 raw_data[i].distance ,raw_data[i].force, raw_data[i].time);
	}
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Primera prueba
    /*raw_data[count_raw_data++].set(0.2, 1.47, 100);
    raw_data[count_raw_data++].set(18.0, 33.0, 200);
    raw_data[count_raw_data++].set(24.0, 10.0, 300);*/

    raw_data[count_raw_data++].set(0.20, 0.51, 100);
    raw_data[count_raw_data++].set(0.37, 1.47, 200);
    raw_data[count_raw_data++].set(0.53, 2.27, 300);
    raw_data[count_raw_data++].set(0.67, 3.04, 400);
    raw_data[count_raw_data++].set(0.78, 3.50, 500);
    raw_data[count_raw_data++].set(0.88, 3.80, 600);
    raw_data[count_raw_data++].set(0.99, 4.13, 700);
    raw_data[count_raw_data++].set(1.09, 4.33, 800);
    raw_data[count_raw_data++].set(1.19, 4.64, 900);
    raw_data[count_raw_data++].set(1.29, 4.63, 1000);
    raw_data[count_raw_data++].set(1.40, 4.65, 1100);
    raw_data[count_raw_data++].set(1.50, 4.66, 1200);
    raw_data[count_raw_data++].set(1.60, 4.62, 1300);
    raw_data[count_raw_data++].set(1.70, 4.58, 1400);
    raw_data[count_raw_data++].set(1.80, 4.58, 1500);
    raw_data[count_raw_data++].set(1.90, 4.57, 1600);
    raw_data[count_raw_data++].set(2.01, 4.38, 1700);
    raw_data[count_raw_data++].set(2.11, 4.17, 1800);
    raw_data[count_raw_data++].set(2.21, 4.04, 1900);
    raw_data[count_raw_data++].set(2.31, 3.98, 2000);
    raw_data[count_raw_data++].set(2.42, 3.78, 2100);
    raw_data[count_raw_data++].set(2.52, 3.59, 2200);
    raw_data[count_raw_data++].set(2.62, 3.60, 2300);
    raw_data[count_raw_data++].set(2.72, 3.70, 2400);
    raw_data[count_raw_data++].set(2.82, 3.84, 2500);
    raw_data[count_raw_data++].set(2.92, 4.08, 2600);
    raw_data[count_raw_data++].set(3.03, 4.29, 2700);
    raw_data[count_raw_data++].set(3.13, 4.39, 2800);
    raw_data[count_raw_data++].set(3.23, 4.35, 2900);
    raw_data[count_raw_data++].set(3.33, 4.23, 3000);
    raw_data[count_raw_data++].set(3.44, 4.02, 3100);
    raw_data[count_raw_data++].set(3.54, 3.71, 3200);
    raw_data[count_raw_data++].set(3.64, 3.51, 3300);
    raw_data[count_raw_data++].set(3.74, 3.28, 3400);
    raw_data[count_raw_data++].set(3.84, 2.88, 3500);
    raw_data[count_raw_data++].set(3.94, 1.73, 3600);
    raw_data[count_raw_data++].set(4.05, 0.48, 3700);

    print_raw_data();
    align_and_accumulate();
    print_stats();

    // Segunda prueba
    clear_raw_data();
    raw_data[count_raw_data++].set(5.0, 20.0, 100);
    raw_data[count_raw_data++].set(10.0, 25.0, 200);
    raw_data[count_raw_data++].set(15.0, 30.0, 300);
    raw_data[count_raw_data++].set(20.0, 35.0, 400);
    raw_data[count_raw_data++].set(25.0, 10.0, 500);
    raw_data[count_raw_data++].set(30.0, 2.0, 600);

    align_and_accumulate();
    print_stats();

    // Tercera prueba
    clear_raw_data();
    raw_data[count_raw_data++].set(15.0, 22.0, 100);
    raw_data[count_raw_data++].set(20.0, 25.0, 200);
    raw_data[count_raw_data++].set(25.0, 28.0, 300);
    raw_data[count_raw_data++].set(30.0, 31.0, 400);
    raw_data[count_raw_data++].set(35.0, 15.0, 500);
    raw_data[count_raw_data++].set(40.0, 7.0, 600);
    raw_data[count_raw_data++].set(45.0, 2.0, 700);
    align_and_accumulate();
    print_stats();
}

void loop() {}
