#include "sim800l.h"

sim800l::sim800l(int txPin, int rxPin, int dtrPin)
    : serialSim800l(txPin, rxPin), dtrPin(dtrPin), signalLevel(""), batteryLevel(""),
      rxBuffer(""), awaitingSmsContent(false), smsHeader("")
{
    serialSim800l.begin(115200); // Inicializa la comunicación..
    pinMode(dtrPin, OUTPUT);
    digitalWrite(dtrPin, LOW); // LOW activa la comunicación.
}

void sim800l::sendCommand(const String &command)
{
    serialSim800l.println(command);
}

void sim800l::sendMessage(const String &msj, unsigned long number)
{

    String numArg = "AT+CMGS=" + String("\"+549" + String(number) + "\"");
    // Se envían los comandos necesarios sin bloqueos:
    //   serialSim800l.println("AT+CFUN=1");
    //   serialSim800l.println("AT+CMGF=1");
    Serial.print("Comando enviado: ");
    Serial.println(numArg);
    Serial.print("Mensaje: ");
    Serial.println(msj);

    serialSim800l.println(numArg);
    delay(100);
    serialSim800l.print(msj);
    delay(100);
    // Finaliza el mensaje enviando Ctrl+Z (valor 26)
    serialSim800l.write(26);
    Serial.println("Mensaje enviado (pendiente de confirmación)");
}

void sim800l::update()
{
    // Acumula todos los caracteres disponibles en el buffer interno
    while (serialSim800l.available())
    {
        char c = serialSim800l.read();
        rxBuffer += c;
    }

    // Procesa líneas completas que estén delimitadas por "\r\n"
    int pos;
    while ((pos = rxBuffer.indexOf("\r\n")) != -1)
    {
        String line = rxBuffer.substring(0, pos);
        rxBuffer.remove(0, pos + 2); // Elimina la línea procesada (incluyendo delimitador)
        line.trim();
        if (line.length() == 0)
            continue; // Ignora líneas vacías

        // Si se esperaba el contenido del SMS, esta línea corresponde al mensaje
        if (awaitingSmsContent)
        {
            extractSms(smsHeader, line);
            awaitingSmsContent = false;
            smsHeader = "";
        }
        else
        {
            processLine(line);
        }
    }
}

void sim800l::processLine(String line)
{

    //   Serial.println("Recibido: " + line);

    // Detecta encabezado de SMS: puede ser +CMGR: o +CMT:
    if (line.startsWith("+CMGR:") || line.startsWith("+CMT:"))
    {
        smsHeader = line;
        awaitingSmsContent = true; // Se espera la siguiente línea con el contenido del SMS
    }
    // Notificación de nuevo SMS recibido (por ejemplo, +CMTI: indica que se almacenó un SMS)
    else if (line.startsWith("+CMTI:"))
    {
        int commaIndex = line.indexOf(",");
        if (commaIndex != -1)
        {
            String indexStr = line.substring(commaIndex + 1);
            indexStr.trim();
            String cmd = "AT+CMGR=" + indexStr;
            serialSim800l.println(cmd);
        }
    }
    else if (line.startsWith("+CSQ:"))
    {
        int colonIndex = line.indexOf(":");
        if (colonIndex != -1)
        {
            String rssi = line.substring(colonIndex + 1);
            rssi.trim();
            int commaIndex = rssi.indexOf(",");
            if (commaIndex != -1)
            {
                rssi = rssi.substring(0, commaIndex);
                rssi.trim();
            }
            signalLevel = getSignalLevel(rssi);
            //  Serial.println("Signal Quality: " + signalLevel);
        }
    }
    else if (line.startsWith("+CBC:"))
    {
        int colonIndex = line.indexOf(":");
        if (colonIndex != -1)
        {
            String batteryData = line.substring(colonIndex + 1);
            batteryLevel = getBatteryLevel(batteryData);
            //  Serial.println("Battery Percent: " + batteryLevel);
        }
    }
    else if (line.startsWith("+CMGS"))
    {

        Serial.println("Mensaje Enviado");
        // Envía comandos para borrar los SMS sin bloqueos.
        serialSim800l.println("AT+CMGD=1,4");
        serialSim800l.println("AT+CMGDA= \"DEL ALL\"");
        Serial.println("Mensaje borrado de la memoria");
    }
    else if (line == "OK" || line == ">")
    { // Se coloca los corchetes angulares para no cargar la salida.
      // Serial.println("COMM OK");
    }
    else
    {
        Serial.println("Comando no manejado: " + line);
    }
}

void sim800l::extractSms(String header, String content)
{
    // Diferencia entre encabezados +CMGR: y +CMT:
    if (header.startsWith("+CMGR:"))
    {
        // Se espera el formato:
        // +CMGR: "REC UNREAD","xxxxxxxxxx","","25/04/01,10:12:41-12"
        int colonIndex = header.indexOf(":");
        if (colonIndex != -1)
        {
            header = header.substring(colonIndex + 1);
            header.trim();
        }
        // Extraer el estado del SMS (entre la primera pareja de comillas)
        int pos1 = header.indexOf('"');
        int pos2 = header.indexOf('"', pos1 + 1);
        if (pos1 == -1 || pos2 == -1)
            return;
        lastMessage.smsStatus = header.substring(pos1 + 1, pos2);

        // Extraer el número del remitente (entre la segunda pareja de comillas)
        int pos3 = header.indexOf('"', pos2 + 1);
        int pos4 = header.indexOf('"', pos3 + 1);
        if (pos3 == -1 || pos4 == -1)
            return;
        lastMessage.senderNumber = header.substring(pos3 + 1, pos4);

        // Extraer la fecha/hora (entre la tercera pareja de comillas)
        int pos5 = header.indexOf('"', pos4 + 1);
        int pos6 = header.indexOf('"', pos5 + 1);
        if (pos5 == -1 || pos6 == -1)
            return;
        lastMessage.receivedDate = header.substring(pos5 + 1, pos6);
    }
    else if (header.startsWith("+CMT:"))
    {
        // Se espera el formato:
        // +CMT: "xxxxxxxxxx","","25/04/03,17:04:46-12"
        int colonIndex = header.indexOf(":");
        if (colonIndex != -1)
        {
            header = header.substring(colonIndex + 1);
            header.trim();
        }
        // Para +CMT, se tienen tres campos:
        // Campo 1: Número del remitente
        // Campo 2: Campo "alpha" (generalmente vacío)
        // Campo 3: Fecha/hora del mensaje
        int pos1 = header.indexOf('"');
        int pos2 = header.indexOf('"', pos1 + 1);
        if (pos1 == -1 || pos2 == -1)
            return;

        // Se le asigna un estado genérico para SMS directos
        lastMessage.smsStatus = "REC";
        lastMessage.senderNumber = header.substring(pos1 + 1, pos2);

        // Se salta el segundo campo (alpha) ya que generalmente es vacio.
        int pos3 = header.indexOf('"', pos2 + 1);
        int pos4 = header.indexOf('"', pos3 + 1);
        if (pos3 == -1 || pos4 == -1)
            return;
        // Extraer el tercer campo (fecha/hora)
        int pos5 = header.indexOf('"', pos4 + 1);
        int pos6 = header.indexOf('"', pos5 + 1);
        if (pos5 == -1 || pos6 == -1)
            return;
        lastMessage.receivedDate = header.substring(pos5 + 1, pos6);
    }

    // Se asigna el contenido del SMS
    lastMessage.msg = content;
    lastMessage.msg.toLowerCase(); // Se transforma todo a minuscula para que las comparaciones sean mas simples.

    // Serial.println("SMS Extraído:");
    // Serial.println("Estado: " + lastMessage.smsStatus);
    // Serial.println("Remitente: " + lastMessage.senderNumber);
    // Serial.println("Fecha: " + lastMessage.receivedDate);
    // Serial.println("Mensaje: " + lastMessage.msg);

    // Envía comandos para borrar los SMS sin bloqueos.
    serialSim800l.println("AT+CMGD=1,4");
    serialSim800l.println("AT+CMGDA= \"DEL ALL\"");
}

String sim800l::getLastMessageStatus() const
{
    return lastMessage.smsStatus;
}

String sim800l::getLastMessage() const
{
    return lastMessage.msg;
}

void sim800l::changeMessageStatus()
{
    lastMessage.smsStatus = "";
}

String sim800l::getBattery() const
{
    return batteryLevel;
}

String sim800l::getSignal() const
{
    return signalLevel;
}

// El calculo para la calidad de la señal es empirico, ya que se linealizo para simplificar
String sim800l::getSignalLevel(String rssi)
{
    int intRssi = rssi.toInt();
    if (intRssi == 0)
        return "Sin señal";
    else if (intRssi > 0 && intRssi <= 10)
        return "Mala";
    else if (intRssi > 10 && intRssi <= 20)
        return "Regular";
    else if (intRssi > 20 && intRssi <= 26)
        return "Buena";
    else if (intRssi > 26 && intRssi <= 31)
        return "Excelente";
    else
        return "error";

    // Calculo lineal porcentual.
    /*
    if(intRssi == 0)return String("0%");
    else if(intRssi == 1) return("3%");
    else if(intRssi == 2) return ("6%");
    else if(intRssi > 2 && intRssi <= 31){
      int aux = (intRssi * 2) + 1;
      return String(aux + "%");
    }
    */
}

String sim800l::getBatteryLevel(String buff)
{
    int comma1 = buff.indexOf(",");
    int comma2 = buff.indexOf(",", comma1 + 1);
    if (comma1 != -1 && comma2 != -1)
    {
        String bcl = buff.substring(comma1 + 1, comma2);
        bcl.trim();
        return bcl;
    }
    else
    {
        Serial.println("Error al parsear la respuesta de batería.");
        return "Error";
    }
}
