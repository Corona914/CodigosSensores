#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

// Crear la instancia del sensor con la dirección I2C por defecto
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Iniciando prueba del TSL2561...");

  // Verificar si el sensor responde
  if(!tsl.begin()) {
    Serial.println("No se detectó el sensor TSL2561. ¡Revisa los cables SDA y SCL!");
    while(1); // Detener ejecución
  }

  // Configurar ganancia automática (ajusta la sensibilidad según la luz ambiente)
  tsl.enableAutoRange(true);            
  
  // Configurar tiempo de integración (13ms = rápido, 402ms = lento pero más preciso en luz baja)
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); 

  Serial.println("Sensor inicializado correctamente.");
}

void loop(void) {
  // Crear un evento para almacenar la lectura
  sensors_event_t event;
  tsl.getEvent(&event);

  // Si la luz es válida (no está saturado ni en 0 absoluto)
  if (event.light) {
    Serial.print("Luminosidad detectada: ");
    Serial.print(event.light);
    Serial.println(" lux");
  } else {
    Serial.println("Luz no medible (Sensor tapado o luz demasiado intensa)");
  }

  delay(1000); // Tomar una lectura cada segundo
}