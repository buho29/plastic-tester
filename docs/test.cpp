#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cmath>

struct TestData {
    std::vector<double> tiempos;
    std::vector<double> fuerzas;
    size_t indice_ruptura;
};

class TestAnalyzer {
    std::vector<TestData> pruebas;
    const int ventana_pre = 300; // 300ms antes de ruptura
    const int ventana_post = 100; // 100ms después de ruptura
    
public:
    void agregarPrueba(const std::vector<double>& t, const std::vector<double>& f) {
        TestData td;
        td.tiempos = t;
        td.fuerzas = f;
        td.indice_ruptura = std::distance(f.begin(), 
            std::max_element(f.begin(), f.end()));
        pruebas.push_back(td);
    }

    void procesarDatos(const char* filename) {
        std::vector<std::vector<double>> series_alineadas;
        
        // 1. Alinear series temporalmente
        for(const auto& prueba : pruebas) {
            double t_ruptura = prueba.tiempos[prueba.indice_ruptura];
            
            std::vector<double> serie_alineada;
            for(int dt = -ventana_pre; dt <= ventana_post; dt += 100) {
                double t_target = t_ruptura + dt;
                
                // Interpolación lineal
                auto it = std::lower_bound(prueba.tiempos.begin(), 
                                         prueba.tiempos.end(), t_target);
                
                if(it == prueba.tiempos.begin() || it == prueba.tiempos.end()) continue;
                
                size_t pos = std::distance(prueba.tiempos.begin(), it);
                double f = interpolacionLineal(
                    prueba.tiempos[pos-1], prueba.fuerzas[pos-1],
                    prueba.tiempos[pos], prueba.fuerzas[pos],
                    t_target
                );
                
                serie_alineada.push_back(f);
            }
            series_alineadas.push_back(serie_alineada);
        }

        // 2. Calcular estadísticos
        std::ofstream file(filename);
        file << "TiempoRelativo,Min,Max,Promedio,Percentil95,Ruptura\n";
        
        for(size_t i = 0; i < series_alineadas[0].size(); ++i) {
            std::vector<double> valores;
            for(const auto& serie : series_alineadas) {
                if(i < serie.size()) valores.push_back(serie[i]);
            }
            
            auto [min, max] = std::minmax_element(valores.begin(), valores.end());
            double promedio = calcularPromedio(valores);
            double p95 = calcularPercentil(valores, 95);
            
            int t_rel = -ventana_pre + i*100;
            bool es_ruptura = (t_rel == 0);
            
            file << t_rel << "," 
                 << *min << "," 
                 << *max << ","
                 << promedio << ","
                 << p95 << ","
                 << (es_ruptura ? "1" : "0") << "\n";
        }
    }

private:
    double interpolacionLineal(double t1, double f1, double t2, double f2, double t) {
        double m = (f2 - f1)/(t2 - t1);
        return f1 + m*(t - t1);
    }

    double calcularPromedio(const std::vector<double>& v) {
        double sum = 0.0;
        for(double val : v) sum += val;
        return sum / v.size();
    }

    double calcularPercentil(std::vector<double> v, double p) {
        std::sort(v.begin(), v.end());
        size_t n = (v.size()-1) * p/100.0;
        return v[n];
    }
};

// Ejemplo de uso
int main() {
    TestAnalyzer analyzer;
    
    // Generar datos de prueba 1
    std::vector<double> t1, f1;
    for(int i=0; i<500; i+=100) {
        t1.push_back(i);
        f1.push_back(i * 0.8 + (i==300 ? 50 : 0)); // Ruptura en 300ms
    }
    analyzer.agregarPrueba(t1, f1);

    // Generar datos de prueba 2
    std::vector<double> t2, f2;
    for(int i=0; i<600; i+=100) {
        t2.push_back(i);
        f2.push_back(i * 0.75 + (i==400 ? 60 : 0)); // Ruptura en 400ms
    }
    analyzer.agregarPrueba(t2, f2);

    // Procesar y generar archivo CSV
    analyzer.procesarDatos("analisis_material.csv");
    
    std::cout << "Procesamiento completado. Use el archivo CSV con:\n";
    std::cout << "import pandas as pd\n";
    std::cout << "import matplotlib.pyplot as plt\n";
    std::cout << "df = pd.read_csv('analisis_material.csv')\n";
    std::cout << "plt.plot(df['TiempoRelativo'], df['Promedio'], label='Promedio')\n";
    std::cout << "plt.fill_between(df['TiempoRelativo'], df['Min'], df['Max'], alpha=0.2)\n";
    std::cout << "plt.scatter(df[df['Ruptura']==1]['TiempoRelativo'], \n";
    std::cout << "            df[df['Ruptura']==1]['Percentil95'], color='red', marker='X')\n";

    return 0;
}
