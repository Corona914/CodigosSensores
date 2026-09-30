#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Crear la instancia del sensor
// La dirección I2C por defecto suele ser 0x76 o 0x77
Adafruit_BME280 bme; 

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando prueba del BME280...");

  // Iniciar el sensor. Si tu sensor usa 0x77, cámbialo aquí: bme.begin(0x77)
  unsigned status = bme.begin(0x76);  
  
  if (!status) {
    Serial.println("No se encontró el sensor BME280. ¡Revisa el cableado o la dirección I2C!");
    while (1); // Detener la ejecución si no se encuentra el sensor
  }
  
  Serial.println("Sensor inicializado correctamente.");
  Serial.println();
}

void loop() { 
  // Imprimir la temperatura en grados Celsius
  Serial.print("Temperatura: ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  // Imprimir la humedad relativa
  Serial.print("Humedad: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  // Imprimir la presión en hPa (hectopascales)
  Serial.print("Presión: ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.println("-------------------------");
  
  delay(2000); // Esperar 2 segundos entre lecturas
}