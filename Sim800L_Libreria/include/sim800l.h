#ifndef SIM800L_H
#define SIM800L_H

#include <SoftwareSerial.h>
#include <Arduino.h>

// Definicion de los pines
#define SIM800L_TX 5
#define SIM800L_RX 4
#define SIM800L_DTR 0


// Estructura para almacenar los datos del SMS recibido.
struct Message
{
  String smsStatus;    // Estado del mensaje, ej. "REC UNREAD"
  String senderNumber; // Número del remitente
  String receivedDate; // Fecha y hora del mensaje
  String msg;          // Contenido del mensaje
};

class sim800l
{
private:
  SoftwareSerial serialSim800l; // Comunicación con el módulo SIM800L.
  int dtrPin;                   // Pin de control DTR.
  Message lastMessage;          // Último mensaje SMS extraído.
  String signalLevel;           // Nivel de señal.
  String batteryLevel;          // Nivel de batería.

  // Variables para el buffering y manejo de estado
  String rxBuffer;         // Acumula los datos recibidos del módulo
  bool awaitingSmsContent; // Indica si ya se recibió la cabecera de un SMS y se espera la línea de contenido
  String smsHeader;        // Almacena la cabecera del SMS recibido

  // Métodos internos para el procesamiento de líneas recibidas
  void processLine(String line);
  // Se modifica extractSms para recibir la cabecera y la línea de contenido por separado
  void extractSms(String header, String content);
  String getSignalLevel(String rssi);
  String getBatteryLevel(String buff);

public:
  // Constructor: configura los pines y la comunicación
  sim800l(int txPin, int rxPin, int dtrPin);

  // Envía un comando AT de forma no bloqueante.
  void sendCommand(const String &command);
  // Envía un mensaje SMS (se envían los comandos sin esperar respuesta de forma bloqueante)
  void sendMessage(const String &msj, unsigned long number);
  // Método que debe llamarse en loop() para procesar la entrada serial sin bloquear
  void update();

  // Métodos para acceder al último SMS y al estado de la señal/batería
  String getLastMessage() const;
  String getLastMessageStatus() const;
  String getBattery() const;
  String getSignal() const;

  void changeMessageStatus(); // Cambia el estado del mensaje por vacio.
};

#endif // SIM800L_H
