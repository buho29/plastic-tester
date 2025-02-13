# Plastic Tester

De momento es un "starter kit". La aplicación no hace nada en sí, es solo una demostración de ESPAsyncWebServer con websocket y una interfaz web hecha en Quasar (Vue 2).

los datos son generados con random 

## Funcionalidades

- Sistema de login (usuario unico)
- Restricción de acceso a ciertas partes de la web, se crea un token por IP
- El dispositivo puede "manejar" el navegador (redirigir al usuario a una página)
- El dispositivo puede enviar notificaciones (ok/warn/error)
- El dispositivo actualiza los datos del usuario
- Sistema para guardar variables en JSON (admin/wifi, etc.)
- Sistema para guardar datos de ejemplo (limitado a 10 ficheros, es muy lento de leer, habría que cachearlo en memoria)
- Envío de datos de sensores cada 5 segundos

## Instalación

1. Editar `config.json` y cambiar:
    ```json
    "wifi_pass": "tu pass",
    "wifi_ssid": "tu wifi"
    ```
2. Subir la carpeta `data` al ESP32:
    1. En PlatformIO: build filesystem image
    2. Upload filesystem image
3. Compilar el proyecto:
    1. Upload

## Uso

- Si esta conectado al wifi abre el navegador 
    y pon la IP de tu dispositivo (ej:http://192.168.1.57) .
- Conectate al wifi del dispositivo y abre el navegador 
    y pon "http://8.8.4.4" (nota:aun no lo probé)
- Abrir el `index.html` de la carpeta `vue`:
    - Editar el `index.html`, cambiar el host y poner la IP del ESP32
    ```js
    // Para ejecutar el HTML fuera del ESP32 
    var host = document.location.host;
    if (host === "" || // local file
        host === "127.0.0.1:3000") // live preview
        host = '192.168.1.57'; // IP del ESP32
    ```

## Vue/Quasar

- Uso Vue v2 y Quasar v1. (me vine abajo cuando miré Vue 3 :/)
- No uso CLI, edito el `.js` y ya, no se compila

## Ficheros del proyecto
- index.html: inicio (ahi se puede configurar el host y los colores de la web)
- main.js: creacion y configuracion de router y vue
- vuex.js: creacion de vuex, es la troncal de los datos , el trata los eventos websocket y actualiza los datos
- pages.js: son las paginas de nuestra web 

## Librerías usadas

### C++

- [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)
- [AsyncTCP](https://github.com/ESP32Async/AsyncTCP)
- [ArduinoJson](https://arduinojson.org/)

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

## Licencia

Este proyecto está bajo la Licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.