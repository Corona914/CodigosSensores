#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSps30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_TSL2561_U.h>
#include <TinyGPS++.h>

// --- Definiciones y Objetos GPS ---
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600
TinyGPSPlus gps;

// --- Definiciones y Objetos Sensores I2C ---
#ifndef NO_ERROR
#define NO_ERROR 0
#endif
SensirionI2cSps30 sps30;
Adafruit_BME280 bme; 
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
static int16_t error;

// --- Temporizador No Bloqueante ---
unsigned long tiempoAnteriorSensores = 0;
const unsigned long intervaloSensores = 2000;
bool alertaGpsMostrada = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(100); }
  
  Serial.println("\n--- Iniciando Sistema Modular ---");
  
  inicializarGPS();
  inicializarSensoresI2C();
}

void loop() {
  // 1. El GPS se procesa en cada ciclo de forma ininterrumpida
  procesarGPS();

  // 2. Los sensores I2C se leen cada 2 segundos sin bloquear el procesador
  if (millis() - tiempoAnteriorSensores >= intervaloSensores) {
    leerSensoresI2C();
    tiempoAnteriorSensores = millis();
  }

  // 3. Alerta de cableado del GPS (modificada para no colgar el sistema)
  if (millis() > 5000 && gps.charsProcessed() < 10 && !alertaGpsMostrada) {
    Serial.println(F("ADVERTENCIA: No se detectan datos del GPS. Revisa el cableado."));
    alertaGpsMostrada = true; // Se muestra solo una vez para no saturar el monitor
  }
}

// ==========================================
//          MÓDULOS DE INICIALIZACIÓN
// ==========================================

void inicializarGPS() {
  Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("GPS: Iniciando parseo en pines RX(16) TX(17)...");
}

void inicializarSensoresI2C() {
  Wire.begin();
  
  // Inicializar BME280 (0x76)
  if (!bme.begin(0x76)) {
      Serial.println("I2C Error: No se encontró el sensor BME280.");
  } else {
      Serial.println("I2C: BME280 iniciado.");
  }

  // Inicializar TSL2561
  if (!tsl.begin()) {
      Serial.println("I2C Error: No se encontró el sensor TSL2561.");
  } else {
      Serial.println("I2C: TSL2561 iniciado.");
      tsl.enableAutoRange(true); 
      tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); 
  }

  // Inicializar SPS30
  sps30.begin(Wire, SPS30_I2C_ADDR_69);
  sps30.stopMeasurement();
  delay(100); // Pequeño delay permitido solo en el setup
  sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);
  Serial.println("I2C: SPS30 iniciado.");
  Serial.println("---------------------------------");
}

// ==========================================
//          MÓDULOS DE PROCESAMIENTO
// ==========================================

void procesarGPS() {
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    
    if (gps.encode(c)) {
      if (gps.location.isUpdated()) {
        Serial.println("===== ACTUALIZACIÓN GPS =====");
        Serial.print("Latitud: ");  Serial.println(gps.location.lat(), 6); 
        Serial.print("Longitud: "); Serial.println(gps.location.lng(), 6);
        Serial.print("Altitud (m): "); Serial.println(gps.altitude.meters());
        Serial.print("Satélites: "); Serial.println(gps.satellites.value());
        Serial.println("=============================\n");
      }
    }
  }
}

void leerSensoresI2C() {
  // BME280
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  // TSL2561
  sensors_event_t event;
  tsl.getEvent(&event);
  float lux = event.light;

  // SPS30
  uint16_t dataReadyFlag = 0;
  uint16_t mc1p0 = 0, mc2p5 = 0, mc4p0 = 0, mc10p0 = 0;
  uint16_t nc0p5 = 0, nc1p0 = 0, nc2p5 = 0, nc4p0 = 0, nc10p0 = 0;
  uint16_t typicalParticleSize = 0;

  error = sps30.readDataReadyFlag(dataReadyFlag);
  if (error == NO_ERROR && dataReadyFlag) {
      sps30.readMeasurementValuesUint16(mc1p0, mc2p5, mc4p0, mc10p0,
                                        nc0p5, nc1p0, nc2p5, nc4p0,
                                        nc10p0, typicalParticleSize);
  }

  // Impresión
  Serial.println("===== LECTURA SENSORES I2C =====");
  Serial.print("Temp: "); Serial.print(temp); Serial.println(" °C");
  Serial.print("Hum:  "); Serial.print(hum); Serial.println(" %");
  Serial.print("Pres: "); Serial.print(pres); Serial.println(" hPa");
  
  if (event.light) {
      Serial.print("Luz:  "); Serial.print(lux); Serial.println(" lux");
  } else {
      Serial.println("Luz:  Sobrecarga / Error");
  }

  Serial.print("PM 1.0: "); Serial.print(mc1p0); Serial.println(" ug/m3");
  Serial.print("PM 2.5: "); Serial.print(mc2p5); Serial.println(" ug/m3");
  Serial.print("PM 10:  "); Serial.print(mc10p0); Serial.println(" ug/m3");
  Serial.println("================================\n");
}