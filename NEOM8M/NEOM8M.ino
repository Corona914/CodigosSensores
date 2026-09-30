#include <TinyGPS++.h>

// Conexiones del ESP32
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

// Crea el objeto TinyGPS++
TinyGPSPlus gps;

void setup() {
  Serial.begin(115200);
  
  // Inicializa el puerto serial para el GPS
  Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("Iniciando parseo de GPS con TinyGPS++...");
  Serial.println("Esperando nueva lectura de coordenadas...");
}

void loop() {
  // Mientras sigan llegando datos desde el NEO-M8N...
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    
    // Alimenta la librería. Si devuelve 'true', armó una frase válida
    if (gps.encode(c)) {
      
      // Verifica si la ubicación se actualizó en este ciclo
      if (gps.location.isUpdated()) {
        
        // Extrae las coordenadas en variables 'double'
        double latitud = gps.location.lat();
        double longitud = gps.location.lng();
        
        // Imprime los resultados con 6 decimales de precisión
        Serial.print("Latitud: ");
        Serial.println(latitud, 6); 
        
        Serial.print("Longitud: ");
        Serial.println(longitud, 6);
        
        // También puedes extraer otros datos muy útiles:
        Serial.print("Altitud (m): ");
        Serial.println(gps.altitude.meters());
        
        Serial.print("Satélites conectados: ");
        Serial.println(gps.satellites.value());
        
        Serial.println("---------------------------------");
      }
    }
  }

  // Pequeña alerta por si el cableado falla durante la ejecución
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No se detectan datos del GPS. Revisa el cableado."));
    while(true);
  }
}