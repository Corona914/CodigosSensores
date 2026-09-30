#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_BME280.h>
#include <Adafruit_PM25AQI.h>
#include <WiFi.h>
#include <WebServer.h>

// --- Configuración de Red WiFi ---
const char* ssid = "Totalplay-60B9";
const char* password = "60B9EA34a8k9pE3X";

// Servidor Web en el puerto 80
WebServer server(80);

// Instancias I2C (Luz y Clima)
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_BME280 bme;

// Instancia para el PMS5003
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();

// Definir pines para Serial2 (UART) en la ESP32
#define RXD2 16
#define TXD2 17

// --- Variables para lectura no bloqueante ---
unsigned long previousMillis = 0;
const long interval = 2000;  // Leer sensores cada 2 segundos

// Variables globales para almacenar la última lectura
float temp = 0.0, hum = 0.0, pres = 0.0, lux = 0.0;
uint16_t pm10 = 0, pm25 = 0, pm100 = 0;
bool pms_read_success = false;

// --- Funciones del Servidor Web ---

void handleRoot() {
  // Interfaz HTML simple para ver desde el navegador
  String html = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>";

  // --- LÍNEA NUEVA: Refrescar la página automáticamente cada 10 segundos ---
  html += "<meta http-equiv='refresh' content='10'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Estación Ambiental ESP32</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f3f4f6; color: #1f2937; margin: 0; padding: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; }";
  html += ".card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); margin-bottom: 20px; }";
  html += "h1 { text-align: center; color: #111827; }";
  html += "h2 { color: #3b82f6; border-bottom: 2px solid #e5e7eb; padding-bottom: 5px; }";
  html += ".metric { font-size: 1.1em; margin: 10px 0; }";
  html += ".value { font-weight: bold; color: #059669; }";
  html += "</style></head><body>";

  html += "<div class='container'><h1>Monitoreo Ambiental</h1>";

  // Tarjeta de Clima
  html += "<div class='card'><h2>Clima y Luz</h2>";
  html += "<div class='metric'>Temperatura: <span class='value'>" + String(temp, 2) + " &deg;C</span></div>";
  html += "<div class='metric'>Humedad: <span class='value'>" + String(hum, 2) + " %</span></div>";
  html += "<div class='metric'>Presión: <span class='value'>" + String(pres, 2) + " hPa</span></div>";
  html += "<div class='metric'>Luminosidad: <span class='value'>" + String(lux, 2) + " lux</span></div>";
  html += "</div>";

  // Tarjeta de Calidad del Aire
  html += "<div class='card'><h2>Calidad del Aire (PM2.5)</h2>";
  if (pms_read_success) {
    html += "<div class='metric'>PM 1.0: <span class='value'>" + String(pm10) + " &micro;g/m&sup3;</span></div>";
    html += "<div class='metric'>PM 2.5: <span class='value'>" + String(pm25) + " &micro;g/m&sup3;</span></div>";
    html += "<div class='metric'>PM 10: <span class='value'>" + String(pm100) + " &micro;g/m&sup3;</span></div>";
  } else {
    html += "<div class='metric' style='color:red;'>Esperando datos del sensor PMS5003...</div>";
  }
  html += "</div></div></body></html>";

  server.send(200, "text/html", html);
}

void handleApi() {
  // Endpoint JSON (Útil si conectas esto a un frontend en React o similar)
  String json = "{";
  json += "\"temperatura\":" + String(temp, 2) + ",";
  json += "\"humedad\":" + String(hum, 2) + ",";
  json += "\"presion\":" + String(pres, 2) + ",";
  json += "\"luminosidad\":" + String(lux, 2) + ",";
  json += "\"pm1_0\":" + String(pm10) + ",";
  json += "\"pm2_5\":" + String(pm25) + ",";
  json += "\"pm10\":" + String(pm100);
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nIniciando estación de monitoreo...");

  // 1. Inicializar Sensores
  if (!tsl.begin()) {
    Serial.println("Error: TSL2561");
    while (1)
      ;
  }
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);

  if (!bme.begin(0x76)) {
    Serial.println("Error: BME280");
    while (1)
      ;
  }

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  if (!aqi.begin_UART(&Serial2)) {
    Serial.println("Error: PMS5003");
    while (1)
      ;
  }

  // 2. Conectar a WiFi
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado.");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // 3. Configurar rutas del Servidor Web
  server.on("/", handleRoot);
  server.on("/api/data", handleApi);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  // 1. Atender peticiones web entrantes (Debe ejecutarse continuamente)
  server.handleClient();

  // 2. Leer sensores cada 2 segundos sin bloquear el código
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // --- Leer Clima y Luz ---
    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light) lux = event.light;

    temp = bme.readTemperature();
    hum = bme.readHumidity();
    pres = bme.readPressure() / 100.0F;

    // --- Leer Calidad del Aire ---
    PM25_AQI_Data data;
    if (aqi.read(&data)) {
      pm10 = data.pm10_env;
      pm25 = data.pm25_env;
      pm100 = data.pm100_env;
      pms_read_success = true;
    } else {
      pms_read_success = false;
    }

    // Opcional: Imprimir en consola para depuración
    Serial.println("Sensores actualizados.");
  }
}