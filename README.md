🛰️ Guía de Telemetría Rural: SMS, GPS y JSON con LilyGO T-SIM7000G

¡Bienvenidos a la siguiente fase de nuestro proyecto IoT!

En los ejercicios anteriores aprendimos a usar la "Autopista de Internet" (LTE-M/GPRS) y la "Oficina de Correos" (MQTT). Pero, ¿qué pasa si nuestro dispositivo está en una zona tan remota que no hay cobertura de datos de internet, sino solo señal básica de celular?

Este documento explica cómo prescindir del internet y usar Mensajes de Texto (SMS) combinados con Geolocalización satelital (GPS) y estructuras de datos profesionales (JSON).

1. El Concepto: Telemetría sin Internet

✉️ Analogía: El Telegrama vs. El Paquete de Internet

Si el internet (MQTT) es como enviar cajas por una empresa de paquetería logística, el SMS es como enviar un Telegrama directo.

Ventaja: Funciona en casi cualquier parte del mundo donde haya una mínima señal de voz/texto. No requiere negociar IPs, ni APNs, ni servidores externos.

Desventaja: Cuesta dinero por cada mensaje enviado y tiene un límite estricto de 160 caracteres (aunque los módems modernos los concatenan).

📋 El Formulario JSON

En lugar de enviarle un SMS a nuestro jefe que diga: "Hola, soy el sensor de la parcela sur, hace mucho calor, estamos a 35 grados y estoy en las coordenadas X, Y", enviamos un texto estructurado llamado JSON (JavaScript Object Notation).

Analogía: JSON es como un formulario oficial del gobierno. Tiene "casillas" (llaves) y "respuestas" (valores).

¿Por qué usarlo en la Maestría? Porque si este SMS llega a un servidor automatizado (o a una app en tu celular), la computadora no sabe leer español, pero sabe leer JSON perfectamente para extraer los datos y graficarlos al instante.

{
  "id": "TEMP_AGRO_01",
  "sensor": "Sensor Suelo Sur",
  "valor": 35.50,
  "alerta": "ALERTA_MAXIMO",
  "lat": 20.266,
  "lon": -99.213
}


2. Arquitectura del Hardware y Nuevos Componentes

Nuestra placa LilyGO T-SIM7000G sigue usando a sus dos protagonistas:

ESP32 (El Ingeniero): Arma el paquete de datos y le dice al módem a qué número enviarlo.

SIM7000G (El Chofer): Envía el SMS por las antenas de Telcel.

🛰️ El Nuevo Invitado: El Navegador (Antena GPS)

El chip SIM7000G tiene un receptor GNSS (GPS) independiente de la red celular. Para que funcione, el "Chofer" debe encender su antena de navegación y mirar al cielo.

El Reto del "Cold Start" (Inicio en Frío): Cuando enciendes el GPS, este no sabe dónde está. Tiene que buscar en el cielo las señales de al menos 4 satélites moviéndose a 14,000 km/h. Este proceso matemático puede tardar de 1 a 5 minutos. ¡Por eso el código tiene un contador de espera!

3. Los Comandos AT de esta Práctica

En lugar de usar comandos para abrir internet, en este código usamos "Comandos AT" mágicos para dominar los SMS:

AT+CMGF=1 -> Modo Texto Estricto. Le dice al módem: "No me des los mensajes en código hexadecimal o PDU (formato de máquinas), dámelos en texto legible para humanos".

AT+CNMI=2,2,0,0,0 -> Ruteo Directo. Normalmente, cuando llega un SMS, el módem lo guarda en la tarjeta SIM y se queda callado. Este comando le dice: "En cuanto recibas un telegrama, no lo guardes, ¡escupelo inmediatamente por el cable serial para que el ESP32 lo lea!".

4. Flujo del Programa (Paso a Paso)

Arranque: El ESP32 despierta al SIM7000G y enciende su módulo satelital (modem.enableGPS()).

Interacción: El sistema se detiene y espera a que el usuario escriba su número de celular en el Monitor Serie de la computadora.

Triangulación: Una vez que le das Enter a tu número, el código entra en un ciclo de 120 segundos. Imprime . mientras espera que el GPS logre leer los satélites.

Empaquetado y Envío: Si encuentra satélites, pone las coordenadas reales; si se acaba el tiempo, pone null. Arma el JSON y lo dispara como SMS a tu celular.

Escucha Activa: El ESP32 se queda en un bucle infinito (imprimiendo un latido . cada 5 segundos) escuchando el puerto serial. Si detecta la palabra ENCENDER o APAGAR en un mensaje entrante, actúa sobre el LED físico (Pin 12).

5. Guía de Pruebas en Vivo

Asegúrate de conectar AMBAS antenas a la placa (La LTE al puerto celular y la GPS al puerto GPS).

Coloca el dispositivo cerca de una ventana o en el exterior. ¡El GPS no funciona bajo techos gruesos!

Sube el código y abre el Monitor Serie a 115200 baudios.

Cuando el sistema lo pida, escribe tu número telefónico (ej. +527712345678) y presiona Enter.

Espera pacientemente. Verás puntos en la pantalla mientras busca satélites.

¡Revisa tu celular! Deberías recibir un SMS con formato JSON.

Responde a ese mismo mensaje de texto enviando la palabra ENCENDER.

Observa tu placa: en unos segundos, el Monitor Serie mostrará el comando reconocido y el LED azul se encenderá.

6. Troubleshooting: Problemas Comunes

1. El SMS llega, pero dice "lat":null, "lon":null

Causa: El tiempo de espera (2 minutos) se agotó antes de que el GPS pudiera triangular los satélites.

Solución: Sal al patio. En interiores, las ondas satelitales rebotan o no penetran. La primera vez que el módulo GPS se enciende en una ubicación nueva, necesita un cielo totalmente despejado.

2. Le envío "ENCENDER" desde mi celular pero la placa no hace nada

Causa A (Demora de Red): Los SMS no son instantáneos. Dependiendo de Telcel, pueden tardar de 5 segundos a un par de minutos en ser enrutados.

Causa B (El formato): Verifica en el Monitor Serie cómo está llegando el texto crudo. Aunque el código usa .toUpperCase() para convertir tu texto a mayúsculas y evitar errores si escribes "Encender" o "encender", asegúrate de no enviar espacios extra ni emojis.

3. El Monitor Serie no me deja escribir mi número

Causa: El Monitor de Arduino no está enviando el salto de línea al presionar Enter.

Solución: En la esquina inferior derecha del Monitor Serie, cambia la opción que dice "Sin ajuste de línea" por "Ambos NL & CR" o "Nueva línea". Esto asegura que el código detecte que ya terminaste de escribir tu número.
