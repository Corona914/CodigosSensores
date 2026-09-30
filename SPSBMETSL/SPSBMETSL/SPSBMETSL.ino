#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSps30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_TSL2561_U.h>

#ifndef NO_ERROR
#define NO_ERROR 0
#endif

// Creación de objetos para los tres sensores
SensirionI2cSps30 sps30;
Adafruit_BME280 bme; 
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

static char errorMessage[64];
static int16_t error;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(100); }
    
    Wire.begin();

    Serial.println("\n--- Inicializando Sensores ---");

    // 1. Inicializar BME280 (Dirección I2C comúnmente 0x76 o 0x77)
    if (!bme.begin(0x76)) {
        Serial.println("Error: No se encontró el sensor BME280. Revisa las conexiones.");
    } else {
        Serial.println("BME280 iniciado correctamente.");
    }

    // 2. Inicializar TSL2561
    if (!tsl.begin()) {
        Serial.println("Error: No se encontró el sensor TSL2561. Revisa las conexiones.");
    } else {
        Serial.println("TSL2561 iniciado correctamente.");
        tsl.enableAutoRange(true); // Ajuste automático de ganancia según la luz
        tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); // Medición rápida
    }

    // 3. Inicializar SPS30
    sps30.begin(Wire, SPS30_I2C_ADDR_69);
    sps30.stopMeasurement();
    delay(100);
    sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);
    Serial.println("SPS30 iniciado correctamente.");
    
    Serial.println("------------------------------\n");
    delay(2000);
}

void loop() {
    // --- LECTURA BME280 ---
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F; // Convertir a hPa

    // --- LECTURA TSL2561 ---
    sensors_event_t event;
    tsl.getEvent(&event);
    float lux = event.light;

    // --- LECTURA SPS30 ---
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

    // --- IMPRESIÓN DE DATOS EN MONITOR SERIE ---
    Serial.println("===== NUEVA LECTURA =====");
    
    // Ambientales
    Serial.print("Temperatura: "); Serial.print(temp); Serial.println(" °C");
    Serial.print("Humedad:     "); Serial.print(hum); Serial.println(" %");
    Serial.print("Presión:     "); Serial.print(pres); Serial.println(" hPa");
    Serial.print("Luz:         "); 
    if (event.light) {
        Serial.print(lux); Serial.println(" lux");
    } else {
        Serial.println("Sobrecarga de luz / Error");
    }

    // Partículas (Solo imprimimos masa para no saturar la pantalla, puedes agregar las demás)
    Serial.print("PM 1.0:      "); Serial.print(mc1p0); Serial.println(" ug/m3");
    Serial.print("PM 2.5:      "); Serial.print(mc2p5); Serial.println(" ug/m3");
    Serial.print("PM 10.0:     "); Serial.print(mc10p0); Serial.println(" ug/m3");
    
    Serial.println("=========================\n");

    delay(2000); // Esperar 2 segundos antes de la siguiente lectura
}