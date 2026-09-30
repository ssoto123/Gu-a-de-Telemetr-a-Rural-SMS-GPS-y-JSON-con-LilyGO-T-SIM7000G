/**************************************************************
 * Código: Telemetría por SMS con JSON y GPS
 * Placa: LilyGO T-SIM7000G
 * 
 * Descripción: Este código prescinde de la conexión a internet.
 * En su lugar, usa "Telegramas Directos" (SMS). El ESP32 pregunta
 * un número al usuario, enciende el GPS, recolecta datos, los
 * empaqueta en formato JSON (como un formulario estructurado) y
 * los envía por mensaje de texto. Además, se queda escuchando
 * respuestas para ejecutar acciones (ej. "ENCENDER").
 **************************************************************/

// 1. DEFINICIONES DE HARDWARE Y LIBRERÍAS
#define TINY_GSM_MODEM_SIM7000 // Le decimos a la librería quién es nuestro "Chofer"

#define SerialMon Serial       // Consola de la computadora
#define SerialAT Serial1       // Cable de comunicación interno ESP32 <-> SIM7000G

#include <TinyGsmClient.h>     // Librería principal del módem

TinyGsm modem(SerialAT);       // Creamos al "Chofer"

// Pines de la LilyGO T-SIM7000G
#define LED_PIN 12             // Pin del LED azul
#define PWR_PIN 4              // Pin del botón de encendido del módem

String numeroDestino = "";     // Aquí guardaremos el número que escribas en el Monitor Serie

// ============================================================
// FUNCIÓN: CONSTRUIR EL MENSAJE JSON (La caja empaquetada)
// ============================================================
// Analogía: En lugar de enviar un papel con letras desordenadas, 
// un JSON es como un formulario oficial con casillas claras. 
String generarMensajeJSON() {
  // Simularemos lecturas de un sensor
  float valorTemperatura = 35.5;
  String idSensor = "TEMP_AGRO_01";
  String nombreSensor = "Sensor Suelo Sur";
  String alerta = "ALERTA_MAXIMO"; 
  
  // Le pedimos al "Chofer" que mire su navegador satelital
  float latitud = 0.0;
  float longitud = 0.0;
  bool gpsFijo = modem.getGPS(&latitud, &longitud);
  
  // Marca de tiempo
  String fecha = "2026-09-30";
  String hora = "08:45:00";

  // Construimos el texto JSON 
  String json = "{";
  json += "\"id\":\"" + idSensor + "\",";
  json += "\"sensor\":\"" + nombreSensor + "\",";
  json += "\"fecha\":\"" + fecha + "\",";
  json += "\"hora\":\"" + hora + "\",";
  json += "\"valor\":" + String(valorTemperatura) + ",";
  json += "\"alerta\":\"" + alerta + "\",";
  
  // Agregamos las coordenadas GPS
  if (gpsFijo) {
    json += "\"lat\":" + String(latitud, 6) + ",";
    json += "\"lon\":" + String(longitud, 6);
  } else {
    json += "\"lat\":null,";
    json += "\"lon\":null";
  }
  
  json += "}";
  return json;
}

// ============================================================
// SETUP: PREPARACIÓN Y ENVÍO DEL PRIMER SMS
// ============================================================
void setup() {
  // Iniciar la consola
  SerialMon.begin(115200);
  delay(10);

  // Configurar el LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Empezamos apagados

  // ENCENDIDO DEL MÓDEM (Pulso de 1 segundo al pin PWR)
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH); 
  delay(1000);                 
  digitalWrite(PWR_PIN, LOW);  

  SerialMon.println("\n--- INICIANDO SISTEMA ---");
  SerialMon.println("Esperando a que el módem despierte...");

  // Iniciar comunicación UART con el módem
  SerialAT.begin(115200, SERIAL_8N1, 26, 27);
  delay(6000); // Dar tiempo para que el chip arranque

  SerialMon.println("Inicializando módem celular...");
  modem.restart(); 
  
  // Asegurarnos de que el chip tenga señal de telefonía
  SerialMon.print("Buscando red celular...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" -> Falló. Revisa la antena LTE y que el chip esté insertado.");
    while (true); // Detener el programa si no hay señal
  }
  SerialMon.println(" -> Red encontrada (Tenemos señal para SMS).");

  // ACTIVAR EL NAVEGADOR GPS
  SerialMon.println("Activando la antena GPS...");
  modem.enableGPS();
  SerialMon.println("NOTA: Triangular satélites (Cold Start) puede tomar");
  SerialMon.println("de 1 a 5 minutos y requiere estar al aire libre.");

  // INTERACCIÓN CON EL USUARIO: Pedir el número de teléfono
  SerialMon.println("\n=============================================");
  SerialMon.println("POR FAVOR, ESCRIBE TU NÚMERO DE TELÉFONO");
  SerialMon.println("EN LA CAJA DE TEXTO DE ARRIBA Y PRESIONA ENTER.");
  SerialMon.println("Ejemplo: +525512345678 (No olvides el código de país)");
  SerialMon.println("=============================================\n");

  // Esperar a que el usuario escriba
  while (SerialMon.available() == 0) {
    delay(100);
  }

  // Leer lo que el usuario escribió
  numeroDestino = SerialMon.readStringUntil('\n');
  numeroDestino.trim(); 

  SerialMon.print("-> Número registrado: ");
  SerialMon.println(numeroDestino);

  // ESPERAR AL NAVEGADOR GPS
  SerialMon.println("\n[GPS] Buscando satélites (Máximo 2 minutos)...");
  SerialMon.println("¡Asegúrate de estar cerca de una ventana o al aire libre!");
  
  bool gpsFijo = false;
  for (int i = 0; i < 120; i++) { // Intentará durante 120 segundos (2 minutos)
    float latLocal = 0, lonLocal = 0;
    
    if (modem.getGPS(&latLocal, &lonLocal)) {
      if (latLocal != 0.0 && lonLocal != 0.0) { 
        gpsFijo = true;
        SerialMon.println("\n[GPS] ¡Satélites encontrados con éxito!");
        break; 
      }
    }
    SerialMon.print("."); 
    delay(1000); 
  }

  if (!gpsFijo) {
    SerialMon.println("\n[GPS] Tiempo agotado. Se enviará el SMS sin coordenadas (null).");
  }

  // Generar el mensaje JSON y enviarlo
  String mensajeAEnviar = generarMensajeJSON();
  SerialMon.println("\n[JSON a enviar]:");
  SerialMon.println(mensajeAEnviar);
  
  SerialMon.print("\nEnviando SMS al chófer para entrega... ");
  if (modem.sendSMS(numeroDestino, mensajeAEnviar)) {
    SerialMon.println("¡SMS Enviado con éxito!");
  } else {
    SerialMon.println("Error al enviar el SMS. Revisa tu saldo.");
  }

  // CONFIGURAR LA RECEPCIÓN DE SMS
  // Comando AT estricto: Poner el módem en "Modo Texto"
  modem.sendAT("+CMGF=1");
  modem.waitResponse();

  // Comando AT mágico: Escupir SMS inmediatamente por el cable serial
  modem.sendAT("+CNMI=2,2,0,0,0");
  modem.waitResponse();

  SerialMon.println("\n=============================================");
  SerialMon.println(" SISTEMA LISTO Y EN ESCUCHA.");
  SerialMon.println(" Envía un SMS a esta placa con la palabra:");
  SerialMon.println(" - ENCENDER (Para prender el LED)");
  SerialMon.println(" - APAGAR   (Para apagar el LED)");
  SerialMon.println("=============================================\n");
}

// ============================================================
// LOOP: VIGILANCIA DE SMS ENTRANTES
// ============================================================
unsigned long ultimoLatido = 0; // Variable para nuestro "latido" visual

void loop() {
  
  // Imprimir un punto cada 5 segundos para confirmar que la placa está viva
  if (millis() - ultimoLatido > 5000) {
    SerialMon.print(".");
    ultimoLatido = millis();
  }
  
  // Si el "Chofer" (SIM7000G) nos está pasando un mensaje por el cable serial
  if (SerialAT.available()) {
    
    // Leemos todo el texto que nos mandó
    String mensajeEntrante = SerialAT.readString();
    
    // Lo imprimimos en pantalla para depuración
    SerialMon.println("\n\n--- NUEVO TELEGRAMA RECIBIDO ---");
    SerialMon.print("Texto crudo: ");
    SerialMon.println(mensajeEntrante);
    
    // Convertimos a mayúsculas para evitar errores (ej. "Encender", "encender")
    mensajeEntrante.toUpperCase();

    // Verificamos si el telegrama contiene la palabra clave
    if (mensajeEntrante.indexOf("ENCENDER") != -1) {
      // AQUÍ ESTABA EL ERROR DE SINTAXIS ANTES. AHORA ESTÁ CORREGIDO.
      SerialMon.println(">>> ¡COMANDO RECONOCIDO! Encendiendo Motor/LED.");
      digitalWrite(LED_PIN, HIGH);
    } 
    else if (mensajeEntrante.indexOf("APAGAR") != -1) {
      SerialMon.println(">>> ¡COMANDO RECONOCIDO! Apagando Motor/LED.");
      digitalWrite(LED_PIN, LOW);
    }
    SerialMon.println("--------------------------------\n");
  }

  delay(10);
}
