#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "Arduino.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

/*
 * NetworkManager class handles WiFi connectivity for ESP devices
 * It provides functionality for both Access Point (AP) and Station (STA) modes
 * Includes DNS server capabilities for captive portal functionality
 */
class NetworkManager
{

private:
    DNSServer dnsServer; // DNS server instance for captive portal

public:

    /*
     * Initializes the device in AP mode with DNS capabilities
     * @param hostName - Name for the access point and mDNS service
     */
    void begin(const char *hostName)
    {
        // The default android DNS
        static const IPAddress apIP(8, 8, 4, 4);

        Serial.printf("begin wifi\n");

        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(hostName);

        Serial.printf("softIP Address: %s\n", WiFi.softAPIP().toString());

        // if DNSServer is started with "*" for domain name, it will reply with
        // provided IP to all DNS request
        dnsServer.start(53, "*", WiFi.softAPIP());

        if (MDNS.begin(hostName))
        { // Verifica si la inicialización fue exitosa
            if (!MDNS.addService("http", "tcp", 80))
            {
                Serial.println("Error al añadir el servicio http");
            }
        }
        else
        {
            Serial.println("Error inicializando mDNS"); // Imprime un mensaje de error
        }
    }

    /*
     * Connects to a WiFi network in station mode
     * @param ssid - Network name
     * @param password - Network password
     * Attempts connection for 20 seconds before timing out
     */
    void connect(const char *ssid, const char *password)
    {
        Serial.printf("start wifi\n");

        WiFi.begin(ssid, password);

        uint32_t current_time = millis();
        // probamos conectarnos durante 20s
        while (millis() - current_time < 20000)
        {
            if (isConnected())
            {
                Serial.println("WiFi connected");
                Serial.printf("IP Address: %s time: %dms\n",
                              WiFi.localIP().toString(),
                              millis() - current_time);
                return;
            }
            delay(500);
        }
        WiFi.disconnect(false);
        Serial.printf("wifi: timeout!\n");
    }

    /*
     * Checks if device is connected to WiFi
     * @return true if connected, false otherwise
     */
    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED; // Verifica el estado de la conexión
    }

    /*
     * Disconnects from current WiFi network
     */
    void disconnect()
    {
        WiFi.disconnect(); // Desconecta de la red WiFi
    }

    /*
     * Processes DNS requests for captive portal functionality
     * Should be called regularly in the main loop
     */
    void update()
    {
        dnsServer.processNextRequest(); // Procesa peticiones DNS (necesario para captive portal)
    }
};

#endif