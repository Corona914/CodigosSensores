#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_BME280.h>
#include <Adafruit_PM25AQI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

#include <ArduinoJson.h> // Necesaria para empaquetar los datos

// --- Configuración de Red WiFi ---
const char* ssid = "Totalplay-60B9";
const char* password = "60B9EA34a8k9pE3X";

// --- Configuración de Supabase ---
// Cambia esto por la URL de tu proyecto (agrega /rest/v1/mediciones al final)
const String supabase_url = "https://avzywbgfmcnognpcymmw.supabase.co/rest/v1/mediciones";
// Cambia esto por tu llave 'anon' / 'public'
const String supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImF2enl3YmdmbWNub2ducGN5bW13Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODkxNjkxODcsImV4cCI6MjEwNDc0NTE4N30.D8UBxUF1o6rL1WcHZquJBHae7QZ2vAdefAjUOl6gJGI";
const String api_key = "super_secreta_apikey_001";
const String node_id = "RS-001"; // Identificador de este dispositivo

// --- Configuración del Servidor Python Local ---
// Reemplaza '192.168.X.X' con la IP local de tu Mac. ¡No quites el puerto :8000!
const String server_url = "http://192.168.100.14:8000/api/mediciones";

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
unsigned long previousSensorMillis = 0;
const long sensorInterval = 2000;  // Leer sensores cada 2 segundos

unsigned long previousDbMillis = 0;
const long dbInterval = 10000;     // Enviar a Supabase cada 10 segundos

// Variables globales para almacenar la última lectura
float temp = 0.0, hum = 0.0, pres = 0.0, lux = 0.0;
uint16_t pm10 = 0, pm25 = 0, pm100 = 0;
bool pms_read_success = false;

// --- Funciones de Lógica de Negocio ---

// Clasificación de PM2.5 basada en la NOM-025-SSA1-2021

void enviarDatosBackend() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(server_url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + api_key);

    // Crear el documento JSON (Ya no enviamos la clasificación)
    StaticJsonDocument<256> doc;
    doc["node_id"] = node_id;
    doc["valor_pm25"] = pm25;
    doc["temperatura"] = temp;
    doc["humedad"] = hum;
    doc["presion"] = pres;
    doc["iluminancia"] = lux;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode == 200) {
      Serial.println("Éxito: Medición cruda enviada al backend.");
    } else {
      Serial.print("Error del backend. Código HTTP: ");
      Serial.println(httpResponseCode);
      Serial.println(http.getString());
    }
    http.end();
  } else {
    Serial.println("No hay conexión WiFi, imposible enviar al servidor.");
  }
}

// --- Funciones del Servidor Web (Sin cambios) ---
void handleRoot() {
  String html = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>";
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
  html += "<div class='card'><h2>Clima y Luz</h2>";
  html += "<div class='metric'>Temperatura: <span class='value'>" + String(temp, 2) + " &deg;C</span></div>";
  html += "<div class='metric'>Humedad: <span class='value'>" + String(hum, 2) + " %</span></div>";
  html += "<div class='metric'>Presión: <span class='value'>" + String(pres, 2) + " hPa</span></div>";
  html += "<div class='metric'>Luminosidad: <span class='value'>" + String(lux, 2) + " lux</span></div>";
  html += "</div>";
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

  if (!tsl.begin()) {
    Serial.println("Error: TSL2561");
    while (1);
  }
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);

  if (!bme.begin(0x76)) {
    Serial.println("Error: BME280");
    while (1);
  }

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  if (!aqi.begin_UART(&Serial2)) {
    Serial.println("Error: PMS5003");
    while (1);
  }

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

  server.on("/", handleRoot);
  server.on("/api/data", handleApi);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();

  // 1. Leer sensores cada 2 segundos
  if (currentMillis - previousSensorMillis >= sensorInterval) {
    previousSensorMillis = currentMillis;

    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light) lux = event.light;

    temp = bme.readTemperature();
    hum = bme.readHumidity();
    pres = bme.readPressure() / 100.0F;

    PM25_AQI_Data data;
    if (aqi.read(&data)) {
      pm10 = data.pm10_env;
      pm25 = data.pm25_env;
      pm100 = data.pm100_env;
      pms_read_success = true;
    } else {
      pms_read_success = false;
    }
  }

  // 2. Enviar a base de datos cada 10 segundos
  if (currentMillis - previousDbMillis >= dbInterval) {
    previousDbMillis = currentMillis;
    
    // Solo enviamos si logramos leer el PM2.5, de lo contrario la DB rechazará el valor vacío
    if (pms_read_success) {
      enviarDatosBackend();
    }
  }
}