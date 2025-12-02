#include <Arduino.h>

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>
#include "fauxmoESP.h"

// ⚠️ Crea un file "credentials.h" con le tue credenziali WiFi:
#include "credentials.h"

// Pin LED
#define LED1 0  // D3
#define LED2 2  // D4 (BUILTIN_LED)

#define SERIAL_BAUDRATE 115200

fauxmoESP fauxmo;
AsyncWebServer server(80);

// Nomi dispositivi
const char* presa1Name = "presa1";
const char* presa2Name = "presa2";

// -----------------------------------------------------------------------------
// WiFi Setup
// -----------------------------------------------------------------------------

void wifiSetup() {
    WiFi.mode(WIFI_STA);
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println();
    Serial.printf("[WIFI] Connected! IP address: %s\n", WiFi.localIP().toString().c_str());
}

// -----------------------------------------------------------------------------
// Web Server Setup
// -----------------------------------------------------------------------------

void serverSetup() {
    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data))) return;
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
        if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body)) return;
    });

    server.begin();
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();

    // Configura i pin LED
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);

    // Spegni i LED all'avvio (logica inversa)
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);

    wifiSetup();
    serverSetup();

    fauxmo.createServer(false);
    fauxmo.setPort(80);
    fauxmo.enable(true);

    // Aggiungi dispositivi usando variabili
    fauxmo.addDevice(presa1Name); // LED1
    fauxmo.addDevice(presa2Name); // LED2

    // Callback Alexa → azione
    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        if (strcmp(device_name, presa1Name) == 0) {
            digitalWrite(LED1, !state);
        } else if (strcmp(device_name, presa2Name) == 0) {
            digitalWrite(LED2, !state);
        }
    });
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------

void loop() {
    fauxmo.handle();

    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }
}
