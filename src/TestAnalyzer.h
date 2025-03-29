#ifndef TESTANALYZER_H
#define TESTANALYZER_H

#include "data.h"

class TestAnalyzer 
{
private:
   
// test const
static const uint TEST_MAX_TIME = 20000;
static const int TEST_START_TIME = -200;
static const int TEST_END_TIME = 100;

public:

static const uint TEST_STEP_TIME = 20; // 50hz

static const uint8_t MAX_RESULT = ((TEST_END_TIME - TEST_START_TIME) / TEST_STEP_TIME) + 1;
DataArray<MAX_RESULT, SensorItem> accumulated_data;

void clear(){
	accumulated_data.clear();
}
void clearData(){
    test_data.clear();
}

bool isEmpty(){
    return accumulated_data.size() < 1;
}

bool addPoint(float distance, float force, int time){
    SensorItem *item = test_data.getEmpty();
    if (item)
    {
        item->set(distance, force, time);
        test_data.push(item);
        return true;
    } 
    return false;
}

void addTest(size_t num_tests = 0)
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

			// Actualizar min/max
			acc_item->max = std::max(acc_item->max, force);
			acc_item->min = std::min(acc_item->min, force);
		}
	}
}

SensorItem* getPoint(int time){
    for (SensorItem *acc_item : accumulated_data)
    {
        if (acc_item->time == time)
        {
            return acc_item;
        }
    }
    return nullptr;
}

private:

static const uint MAX_RAW_DATA = TEST_MAX_TIME / TEST_STEP_TIME;
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
		prev = test_data[0];
		next = test_data[1];
	}
	else if (!next)
	{
		// Extrapolar hacia adelante
		prev = test_data[test_data.size() - 2];
		next = test_data[test_data.size() - 1];
	}
	// Interpolar
	float slope = (next->distance - prev->distance) / (next->time - prev->time);
	return prev->distance + slope * (target_time - prev->time);
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
		prev = test_data[0];
		next = test_data[1];
	}
	else if (!next)
	{
		// Extrapolar hacia adelante
		prev = test_data[test_data.size() - 2];
		next = test_data[test_data.size() - 1];
	}

	// Interpolar
	float slope = (next->force - prev->force) / (next->time - prev->time);
	return std::max(0.0f, prev->force + slope * (target_time - prev->time));
}
};

#endif // TESTANALYZER_H