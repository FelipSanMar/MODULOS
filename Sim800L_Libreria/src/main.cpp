#include <Arduino.h>
#include "sim800l.h"



sim800l sim800(SIM800L_TX, SIM800L_RX, SIM800L_DTR);

void setup()
{
  Serial.begin(115200);
  Serial.println("Inicializando SIM800L...");

  sim800.sendCommand("ATE0"); // Desactivar eco

  // Enviar comandos de inicialización sin bloqueos:
  sim800.sendCommand("AT");
  sim800.sendCommand("AT+CFUN=1");
  sim800.sendCommand("AT+CMGF=1");
  sim800.sendCommand("AT+CNMI=1,2,0,0,0"); // Configuración para recibir SMS en dos líneas
  sim800.sendCommand("AT+CSQ");            // Nivel de señal.
  sim800.sendCommand("AT+CBC");            // Nivel de bateria.
}

unsigned long previousMillis = 0;     // Guarda el último tiempo en que se ejecutó el condicional
const unsigned long interval = 10000; // Intervalo de 10 segundos (10000 ms)

unsigned long previous = 0;

void loop()
{

  unsigned long currentMillis = millis();
  unsigned long current = millis();

  // Actualiza la librería; este método procesa la entrada serial de forma no bloqueante.
  sim800.update();
  // Se omiten delays para mantener el sistema eficiente.

  if (currentMillis - previousMillis >= interval)
  {
    // Cada 10s checkea los parametros de señal y bateria.
    sim800.sendCommand("AT+CSQ"); // Nivel de señal.
    sim800.sendCommand("AT+CBC"); // Nivel de bateria.

    previousMillis = currentMillis; // Actualiza el tiempo
  }

  if (current - previous >= 15000)
  {

    // Cada 15s imprime los valores de los parametros y si llego algun mensaje.
    Serial.print("Nivel Bateria:");
    Serial.println(sim800.getBattery());
    Serial.print("Nivel Señal:");
    Serial.println(sim800.getSignal());

    if (sim800.getLastMessageStatus() == "REC")
    {
      String mensaje = sim800.getLastMessage();
      unsigned long myNumber = 2314412792;

      Serial.print("El ultimo msj es:");
      Serial.println(mensaje);
      Serial.print("Status Mensaje liberado:");
      sim800.changeMessageStatus();
      Serial.print("\n Status Mensaje:");
      Serial.println(sim800.getLastMessageStatus());

      Serial.println("Se envia respuesta");
      sim800.sendMessage(mensaje + "puto", myNumber);
    }

    previous = current;
  }
}
