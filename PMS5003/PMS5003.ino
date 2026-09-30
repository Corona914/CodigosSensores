#include <Adafruit_PM25AQI.h>

Adafruit_PM25AQI aqi = Adafruit_PM25AQI();

// Definir pines para Serial2 en la ESP32
#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200);
  
  Serial.println("Iniciando prueba individual del PMS5003...");

  // El PMS5003 se comunica a 9600 baudios por UART
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Inicializar el sensor
  if (!aqi.begin_UART(&Serial2)) {
    Serial.println("Error: No se encontró el PMS5003. Revisa la conexión del cable TX al pin 16.");
    while (1) {
      delay(10); // Detener ejecución
    }
  }

  Serial.println("PMS5003 inicializado correctamente.\n");
}

void loop() {
  PM25_AQI_Data data;
  
  // Intentar leer los datos
  if (aqi.read(&data)) {
    Serial.println("=== Calidad del Aire ===");
    Serial.print("PM 1.0 (Estándar): "); Serial.print(data.pm10_env); Serial.println(" ug/m3");
    Serial.print("PM 2.5 (Estándar): "); Serial.print(data.pm25_env); Serial.println(" ug/m3");
    Serial.print("PM 10  (Estándar): "); Serial.print(data.pm100_env); Serial.println(" ug/m3");
    Serial.println();
  } else {
    Serial.println("Esperando datos del sensor...");
  }

  delay(1000); // Tomar una lectura cada segundo
}