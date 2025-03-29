#include <AUnit.h>
#include "TestAnalyzer.h"

TestAnalyzer analyzer;

void print_stats(size_t num_tests = 0)
{
    Serial.println("\n--- Estadísticas Actualizadas ---");
    Serial.println("TiempoRel | AvgDist | AvgForce (Min-Max)");
    for (SensorItem *item : analyzer.accumulated_data)
    {
        Serial.printf("%9d | %7.2f | %7.2f (%7.2f-%7.2f)\n",
                      item->time,
                      item->distance,
                      item->force, item->min, item->max);
    }
    Serial.printf("Número de pruebas realizadas: %d\n", num_tests+1);
}

test(InterpTest)
{

    // Agregar puntos de prueba
    // Distancia    Fuerza  Tiempo
    assertTrue(analyzer.addPoint(10.0, 27.0, 20));
    assertTrue(analyzer.addPoint(20.0, 33.0, 40));
    assertTrue(analyzer.addPoint(30.0, 10.0, 60));
    // assertTrue(analyzer.addPoint(40.0, 0.0, 80));

    // Ejecutar acumulación
    analyzer.addTest(0);
    print_stats();

    // Verificar resultados acumulados
    assertFalse(analyzer.isEmpty()); // Asegurarse de que los datos acumulados no estén vacíos

    // Buscar el punto de tiempo relativo 20
    SensorItem *item = analyzer.getPoint(-40);
    // Verificar que el punto exista
    assertTrue(item);
    assertEqual(item->force, 21.0);
    assertEqual(item->distance, 0.0);
}

test(avgTest)
{

    analyzer.clearData();
    // Agregar puntos de prueba
    assertTrue(analyzer.addPoint(5.0, 20.0, 20));
    assertTrue(analyzer.addPoint(10.0, 25.0, 40));
    assertTrue(analyzer.addPoint(15.0, 30.0, 60));
    assertTrue(analyzer.addPoint(20.0, 35.0, 80));
    assertTrue(analyzer.addPoint(25.0, 10.0, 100));
    assertTrue(analyzer.addPoint(30.0, 5.0, 120));
    assertTrue(analyzer.addPoint(35.0, 4.0, 140));

    // Ejecutar acumulación
    analyzer.addTest(1);
    print_stats(1);

    // Buscar el punto de tiempo relativo 0
    SensorItem *item = analyzer.getPoint(0);
    // Verificar que el punto exista
    assertTrue(item);
    assertEqual(item->force, ((item->max+item->min)/2.0));
}

void setup()
{
    delay(1000);
    Serial.begin(115200);
}

void loop()
{
    aunit::TestRunner::run();
    // Dejar vacío
}
