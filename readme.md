# Plastic Tester


[Documentacion en español](<readme es.md>)

**Demo on YouTube**

[![Demo on YouTube](https://img.youtube.com/vi/grObh11XFaI/hqdefault.jpg)](https://www.youtube.com/watch?v=grObh11XFaI)

![alt text](docs/img/tester1.jpg)

## General Description

Low-cost DIY system designed to perform tensile tests (*tensile testing machine*) on 3D-printed specimens.

Designed for:

- Validating the quality of 3D printing filaments
- Evaluating layer adhesion in printed parts
- Obtaining comparative data between different filaments
- Obtaining comparative data between different printing parameters
- Being compact and portable

The system uses:
- **Sensor**: Affordable load cell with HX711
- **Actuator**: Stepper motor with driver
- **Control**: Web interface accessible via WiFi
- **Visualization**: Real-time force/displacement graphs

**Key Features**:
- Approximate cost: < €100 in components
- Maximum force: 50kg
- Accuracy: ±0.1kg
- Adjustable speed: 0.1 - 5 mm/min

## Development Notes
- Platform: ESP32
- IDE: PlatformIO

## 3D Printing
- The printable parts and original files (Fusion 360) are located in the folder
[docs/3d](./docs/3d)

![alt text](docs/img/print.jpg)
![alt text](docs/img/slicer.jpg)

## Assembly
### Frame
![Frame](./docs/mount/frame.gif)

[Download video](./docs/mount/frame.avi)

### Mount the T8 nut on the holder with inserts

- Cut off the excess screws

![alt text](docs/mount/t8.jpg)

### Slide
![slide](./docs/mount/slide.gif)

[Download video](./docs/mount/slide.avi)

### Complete
![slide](./docs/mount/mount.gif)

[Download video](./docs/mount/mount.avi)

### Aligning the T8 nut holder with the lead screw

![alt text](docs/mount/slide.jpg)

The screw is 5.8mm in diameter, and the bearings have a 6mm inner diameter.

1. Best method:
    - Center the screw with electrical tape.
![alt text](docs/mount/m6.jpg)
    - Place 2 pieces of 0.2mm paper; since the support area has a base, I left 0.25mm of clearance.
![alt text](docs/mount/paper.jpg)
2. Alternative method:
    - Push towards the base and tighten the screw.
![alt text](docs/mount/not_paper.jpg)
    - Place 4 or more pieces of 0.2mm paper.

## Hardware

### Load Cell (HX711)
```cpp
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
const float CALIBRATING_FACTOR = -2316138 / 44.34; // real kg reading
```
1. **Set the HX711 to 80hz** 
    - Cut the blue square
    - Solder the red square 

![hx711](docs/img/hx711.png)

2. **Attach the screw to the sensor**
    - Drill with a 3mm bit 
    - Attach the eye screw with 2 nuts

![alt text](docs/img/sensor.jpg)
![alt text](docs/img/sensor1.jpg)

**Note: The screw must be soldered**
![alt text](<docs/img/tornillo ojo.jpg>)

### PCB
 - 2 resistors of 1K Ohm are needed
![alt text](docs/img/pcb2.jpg)
**Flipped view (if looking from above)**
![alt text](docs/img/pcb3.jpg)
![alt text](docs/img/pcb1.jpg)
**Normal view (if looking from below)**
![alt text](docs/img/pcb.jpg)

### Complete Circuit
![alt text](docs/img/circuit.jpg)

### Stepper Motor
```cpp
const uint8_t MOTOR_STEP_PIN = 32;
const uint8_t MOTOR_DIRECTION_PIN = 33;
const uint8_t ENDSTOP_PIN = 27;
```
## Installation


1. **Edit [config.json](./data/data/config.json): (optional)**
    ```json
    {
      "wifi_pass": "your pass",
      "wifi_ssid": "your wifi"
    }
    ```
2. **Upload the `data` folder to the ESP32:**
    1. If you want to update/copy the `vue` folder to `data/www`, run:
        ```bash
        python update_data_web.py 
        ```
    2. Steps in PlatformIO:
        - Build filesystem image
        - Upload filesystem image
3. **Compile the project:**
    1. Upload

## Usage

By default, the user is:
- **Username**: admin
- **Password**: admin

### On PC
- Open the browser and go to [http://plastester.local](http://plastester.local)
- Change the WiFi credentials if necessary (Options Page)

### On Mobile (Android)
mDNS does not work on mobile devices, you need to use the IP:
- If connected to WiFi, open the browser and go to your device's IP (e.g., [http://192.168.1.57](http://192.168.1.57))
- If not connected or you don't know the IP:
    - Connect to the device's WiFi and open the browser at [http://plastester.local](http://plastester.local) or [http://8.8.4.4](http://8.8.4.4)
    - Copy the IP and change the WiFi credentials if necessary (System/Options Page)


### Open from `index.html` in the `vue` folder
- Edit the `index.html` and change the host by setting the ESP32's IP:
    ```js
    // To run the HTML outside the ESP32 
    var host = document.location.host;
    if (host === "" || // local file
        host === "127.0.0.1:3000") // preview in preview (visual code addon)
        host = '192.168.1.57'; // ESP32 IP
    ```

## Vue/Quasar

- Using **Vue v2** and **Quasar v1** (decided not to upgrade to Vue 3 for now)
- Not using CLI, editing `.js` files directly in the `vue` folder

## Web Page Files
- `index.html`: Home and host configuration and web colors
- `main.js`: Router and Vue creation and configuration
- `vuex.js`: WebSocket data and event management
- `pages/`: Folder containing the web pages
- `components.js`: Definition of components used on the web (charts)

## Shopping List

**The links are for reference only, make sure it's the best purchase** 

| Item                              | Used | (€)  | Link |
|----------------------------------|------|------|------|
| **Main Components**              |      |      |      |
| Screw Trapezoidal T8 300mm P2L2  | 1    | 9,89 | [Link](https://es.aliexpress.com/item/32507277503.html) |
| Nema17 48mm 2A                   | 1    | 13,19| [Link](https://es.aliexpress.com/item/1005004731197516.html) |
| Stepper DRV8825                  | 1    | 1,59 | [Link](https://www.aliexpress.com/item/4000083334758.html) |
| Expansion Board Stepper          | 1    | 1,79 | [Link](https://es.aliexpress.com/item/10000278156894.html) |
| Weight Sensor 50KG               | 1    | 1,30 | [Link](https://es.aliexpress.com/item/1005006668822214.html) |
| HX711 Green Small                | 1    | 1,18 | [Link](https://www.aliexpress.com/item/1005007091731394.html) |
| ESP32 DEVKIT V1                  | 1    | 3,79 | [Link](https://es.aliexpress.com/item/1005008503831020.html) |
| F8-22M Bearing Axial 8x22x7 (x2) | 2    | 1,89 | [Link](https://es.aliexpress.com/item/1005007129279401.html) |
| 606ZZ 6x17x6mm (x10)             | 8    | 3,09 | [Link](https://es.aliexpress.com/item/1005003067718206.html) |
| 608zz 8x22x7mm (x10)             | 2    | 3,89 | [Link](https://es.aliexpress.com/item/1005003067718206.html) |
| Square Alu 25x25x2 - 250         | 1    | 3,59 | [Link](https://es.aliexpress.com/item/1005007456875848.html) |
| Endstop Switch                   | 1    | 2,79 | [Link](https://es.aliexpress.com/item/1005003139255707.html) |
| Lock Collar T8 (x5)              | 3    | 1,69 | [Link](https://es.aliexpress.com/item/1005001667273798.html) |
| Power Supply Jack Socket         | 1    | 0,54 | [Link](https://www.aliexpress.com/item/32838214586.html) |
| Switch (x5)                      | 1    | 1,34 | [Link](https://www.aliexpress.com/item/32873386670.html) |
| XH2.54 Connector                 | 1    | 2,99 | [Link](https://www.aliexpress.com/item/1005007425641197.html) |
| M3xL3xOD4.2 (x50)                | 10   | 1,59 | [Link](https://www.aliexpress.com/item/1005006472702418.html) |
| PCB Board Protoboard 5x7         | 1    | 0,90 | [Link](https://www.aliexpress.com/item/1005006100148769.html) |
| 1K Ohm resistor                  | 2    |      |      |
| **Total Components**             |      | 57,03|      |
|                                  |      |      |      |
| **Screws and Accessories (best in local seller)**       |      |      |      |
| D Ring Shackle Closed M3 (x2)    | 2    | 2,82 | [Link](https://www.aliexpress.com/item/1005007576884635.html) |
| Nuts M6 (x50)                    | 28   | 4,28 | [Link](https://www.aliexpress.com/item/1005007593861199.html) |
| Nuts M4 (x25)                    | 2    | 1,59 | [Link](https://www.aliexpress.com/item/1005007593861199.html) |
| Nuts M3 (x50)                    | 8    | 1,69 | [Link](https://es.aliexpress.com/item/1005004531602992.html) |
| Screw M6 X 25 (x20)              | 12   | 5,84 | [Link](https://www.aliexpress.com/item/1005003463456440.html) |
| Screw M6 X 60 (x10)              | 6    | 4,48 | [Link](https://www.aliexpress.com/item/1005004527586307.html) |
| Screw M6 X 35 (x5)               | 4    | 1,89 | [Link](https://www.aliexpress.com/item/1005004527586307.html) |
| Screw M3 X 22 (x40)              | 8    | 2,05 | [Link](https://www.aliexpress.com/item/1005004527586307.html) |
| Screw M4 X 20 (x20)              | 2    | 1,95 | [Link](https://www.aliexpress.com/item/1005004527586307.html) |
| Washer M6 12x1.5 (x50)           | 8    | 2,06  | [Link](https://www.aliexpress.com/item/1005004527586307.html) |
| Sheep Eye Screw M3 (x10)         | 2    | 2,59 | Local Seller |
| Threaded Rod M6 -1000            | 1    | 0,80 | Local Seller |
| Plywood Board 340x160x20         | 1    | 6,00 | Local Seller |
| **Total Screws**                 |      | 38,04|      |
| **TOTAL GENERAL**                |      | 95,07|      |
|                                  |      |      |      |
| **Tools**                        |      |      |      |
| Crimping Plier XH2.54            | 1    | 9,39 | [Link](https://www.aliexpress.com/item/1005006224244342.html) |



## Libraries Used

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

## Screens
![Home](docs/img/home.jpg)
![Run](docs/img/run.jpg)
![Run](docs/img/run1.jpg)
![Move](docs/img/move.jpg)
![History](docs/img/history.jpg)
![Result](docs/img/result.jpg)
![Icons](docs/img/options.jpg)
![Icons](docs/img/options1.jpg)
![Icons](docs/img/options2.jpg)
![System](docs/img/sys.jpg)
![System](docs/img/sys1.jpg)
![System](docs/img/sys2.jpg)

## Gallery
![alt text](docs/img/tester.jpg)
![alt text](docs/img/tester1.jpg)
![alt text](docs/img/button.jpg)
**I used a 2x13mm steel shaft** (a cut drill bit in my case)

- Insert it through the "usb" window

![alt text](docs/img/button1.jpg)