import numpy as np
import matplotlib.pyplot as plt
from scipy import interpolate
import pandas as pd

# Generador de datos de prueba simulados
def generar_prueba_simulada(ruptura_ms, duracion_total=1000):
    """Genera datos sintéticos de una prueba de material"""
    tiempos = np.arange(0, duracion_total + 100, 100)
    fuerzas = np.linspace(0, ruptura_ms * 0.8, len(tiempos))
    
    # Añadir ruido y pico de ruptura
    ruido = np.random.normal(0, 2, len(tiempos))
    idx_ruptura = np.abs(tiempos - ruptura_ms).argmin()
    if abs(tiempos[idx_ruptura] - ruptura_ms) > 0:
        print(f"ADVERTENCIA: Usando punto más cercano ({tiempos[idx_ruptura]}ms)")
    fuerzas[idx_ruptura:] = 0  # Fuerza cae después de ruptura
    fuerzas += ruido
    fuerzas[idx_ruptura] += 15  # Pico de ruptura
    
    return tiempos, fuerzas, idx_ruptura

class AnalizadorMateriales:
    def __init__(self, ventana_pre=300, ventana_post=100):
        self.ventana_pre = ventana_pre
        self.ventana_post = ventana_post
        self.pruebas = []
        
    def agregar_prueba(self, tiempos, fuerzas):
        """Registra una nueva prueba en el sistema"""
        idx_ruptura = np.argmax(fuerzas)
        self.pruebas.append({
            'tiempos': tiempos,
            'fuerzas': fuerzas,
            'idx_ruptura': idx_ruptura,
            't_ruptura': tiempos[idx_ruptura]
        })
    
    def procesar_datos(self):
        """Procesa todas las pruebas y genera DataFrame unificado"""
        series_alineadas = []
        tiempo_comun = np.arange(-self.ventana_pre, 
                                self.ventana_post + 100, 100)
        
        for prueba in self.pruebas:
            # Crear escala temporal relativa
            t_rel = prueba['tiempos'] - prueba['t_ruptura']
            f = prueba['fuerzas']
            
            # Crear función de interpolación
            interp_fn = interpolate.interp1d(
                t_rel, f, 
                kind='linear', 
                bounds_error=False, 
                fill_value=np.nan
            )
            
            # Interpolar en tiempos comunes
            fuerza_interp = interp_fn(tiempo_comun)
            series_alineadas.append(fuerza_interp)
        
        # Crear DataFrame con estadísticos
        df = pd.DataFrame(series_alineadas, columns=tiempo_comun)
        
        self.df_resultados = pd.DataFrame({
            'TiempoRelativo': tiempo_comun,
            'Min': df.min(),
            'Max': df.max(),
            'Promedio': df.mean(),
            'Percentil95': df.quantile(0.95),
            'Ruptura': np.where(tiempo_comun == 0, 1, 0)
        }).dropna()
        
        return self.df_resultados
    
    def graficar_resultados(self):
        """Genera la gráfica comparativa completa"""
        df = self.df_resultados
        
        plt.figure(figsize=(12, 7))
        
        # Área de rango
        plt.fill_between(df['TiempoRelativo'], df['Min'], df['Max'],
                        alpha=0.2, color='skyblue', label='Rango min-max')
        
        # Líneas principales
        plt.plot(df['TiempoRelativo'], df['Promedio'], 
                color='navy', lw=2, label='Promedio')
        plt.plot(df['TiempoRelativo'], df['Percentil95'], 
                '--', color='darkorange', lw=2, label='Percentil 95')
        
        # Puntos de ruptura
        rupturas = df[df['Ruptura'] == 1]
        plt.scatter(rupturas['TiempoRelativo'], rupturas['Percentil95'],
                   s=120, c='red', marker='X', label='Punto de ruptura')
        
        # Configuración del gráfico
        plt.title('Análisis Comparativo de Pruebas de Materiales', fontsize=14)
        plt.xlabel('Tiempo Relativo a la Ruptura (ms)', fontsize=12)
        plt.ylabel('Fuerza (kg)', fontsize=12)
        plt.grid(True, alpha=0.3)
        plt.axvline(0, color='gray', linestyle=':', label='Momento de ruptura')
        plt.legend()
        plt.tight_layout()
        plt.show()

# Ejemplo de uso ##################################################

# 1. Configurar analizador
analizador = AnalizadorMateriales(ventana_pre=300, ventana_post=100)

# 2. Generar y agregar pruebas simuladas
for _ in range(5):  # 5 pruebas con rupturas aleatorias
    ruptura = np.random.randint(300, 700)
    tiempos, fuerzas, _ = generar_prueba_simulada(ruptura)
    analizador.agregar_prueba(tiempos, fuerzas)

# 3. Procesar datos
df = analizador.procesar_datos()

# 4. Mostrar resultados
print("Datos procesados:\n", df.head())
analizador.graficar_resultados()
