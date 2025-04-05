# Plastic Tester

## Descripción General

Sistema DIY de bajo coste para realizar ensayos de tracción (*tensile testing machine*) en probetas impresas en 3D. Diseñado específicamente para:

- Validar la calidad de filamentos para impresión 3D
- Evaluar la adhesión entre capas de piezas impresas
- Obtener datos comparativos entre diferentes parámetros de impresión

El sistema utiliza:
- **Sensor**: Celda de carga económico con HX711
- **Actuador**: Motor paso a paso con driver
- **Control**: Interfaz web accesible vía WiFi
- **Visualización**: Gráficas de fuerza/desplazamiento

**Características principales**:
- Coste aproximado: < 100€ en componentes
- Fuerza máxima: 50kg
- Precisión: ±0.1kg
- Velocidad ajustable: 0.1 - 5 mm/min

## Notas de Desarrollo
- Plataforma: ESP32
- IDE: PlatformIO

## Montaje


<video width="640" height="360" controls>
  <source src="./docs/mount/test.mp4" type="video/mp4">
  Tu navegador no soporta la etiqueta de video.
</video>


<video width="640" height="360" controls>
  <source src="./docs/mount/test.mp4" type="video/mp4">
  Tu navegador no soporta la etiqueta de video.
</video>


<video width="640" height="360" controls>
  <source src="./docs/mount/frame.avi" type="video/avi">
  Tu navegador no soporta la etiqueta de video.
</video>


## Conexiones de Hardware

### Celda de Carga (HX711)
```cpp
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
const float CALIBRATING_FACTOR = -2316138 / 44.34; // lectura/kg real
```

### Motor Paso a Paso
```cpp
const uint8_t MOTOR_STEP_PIN = 32;
const uint8_t MOTOR_DIRECTION_PIN = 33;
const uint8_t ENDSTOP_PIN = 27;
```

## Instalación

1. **Editar `config.json`: (opcional)**
    ```json
    {
      "wifi_pass": "tu pass",
      "wifi_ssid": "tu wifi"
    }
    ```
2. **Subir la carpeta `data` al ESP32:**
    1. Si deseas actualizar/copiar la carpeta `vue` a `data/www` ejecutar:
        ```bash
        python update_data_web.py 
        ```
    2. Pasos en PlatformIO:
        - Build filesystem image
        - Upload filesystem image
3. **Compilar el proyecto:**
    1. Upload

## Uso

Por defecto el usuario es:
- **Usuario**: admin
- **Password**: admin

### En PC
- Abre el navegador y ve a [http://plastester.local](http://plastester.local)
- Cambiar las credenciales del WiFi si es necesario (Pagina Options)

### En Móvil (Android)
El mDNS no funciona en móviles, necesitas usar la IP:
- Si está conectado al WiFi, abre el navegador y ve a la IP de tu dispositivo (ej.: [http://192.168.1.57](http://192.168.1.57))
- Si no está conectado o no sabes la IP:
    - Conéctate al WiFi del dispositivo y abre el navegador en [http://plastester.local](http://plastester.local) o [http://8.8.4.4](http://8.8.4.4)
    - Copia la IP y cambia las credenciales del WiFi si es necesario (Pagina System/Options)


### Abrir desde `index.html` de la carpeta `vue`
- Editar el `index.html` y cambiar el host poniendo la IP del ESP32:
    ```js
    // Para ejecutar el HTML fuera del ESP32 
    var host = document.location.host;
    if (host === "" || // archivo local
        host === "127.0.0.1:3000") // vista previa en preview (addon visual code)
        host = '192.168.1.57'; // IP del ESP32
    ```

## Vue/Quasar

- Uso **Vue v2** y **Quasar v1** (decidí no actualizar a Vue 3 por ahora)
- No uso CLI, edito los `.js` directamente de la carpeta `vue`

## Ficheros del Proyecto
- `index.html`: Inicio y configuración de host y colores de la web
- `main.js`: Creación y configuración de router y Vue
- `vuex.js`: Gestión de datos y eventos WebSocket
- `pages`: Carpeta donde estan las páginas de la web
- `components.js`: Definición componentes usadas en la web (charts)

## Librerías usadas

### C++

- [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)
- [AsyncTCP](https://github.com/ESP32Async/AsyncTCP)
- [ArduinoJson](https://arduinojson.org/)
- [FastAccelStepper](https://github.com/gin66/FastAccelStepper)

### JavaScript

- [Vue v2](https://v2.vuejs.org/)
- [Vuex v3](https://v3.vuex.vuejs.org/)
- [Vue-router v3](https://v3.router.vuejs.org/)
- [Quasar Framework v1](https://v1.quasar.dev/)
- [Chart.js v2](https://www.chartjs.org)/[Vue-chartjs v3](https://vue-chartjs.org)

## Pantallas
![alt text](docs/img/screen.webp)
![Home](docs/img/home.png)
![Run](docs/img/run.png)
![Move](docs/img/move.png)
![History](docs/img/history.png)
![Result](docs/img/result.png)
![Icons](docs/img/icons.png)
![System](docs/img/system.png)
