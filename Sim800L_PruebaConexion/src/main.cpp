#include <Arduino.h>
#include <SoftwareSerial.h>

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(5, 4); //
//Se coloca TX y RX correspondientes al modulo sim800L, no se declara cruzado, ni nada de eso. 
//ESP8266- ESP12F -> 5 y 4 Funcionan (Unica combinacion que encontre)

void updateSerial();

void setup()
{
//Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
Serial.begin(9600);

//Begin serial communication with Arduino and SIM800L
mySerial.begin(9600);

pinMode(0, OUTPUT);
digitalWrite(0, LOW);

Serial.println("Initializing...");
delay(1000);

mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
updateSerial();
mySerial.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
updateSerial();
mySerial.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
updateSerial();
mySerial.println("AT+CREG?"); //Check whether it has registered in the network
updateSerial();
}

void loop()
{
updateSerial();
}

void updateSerial()
{
delay(500);
while (Serial.available())
{
mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
}
while(mySerial.available())
{
Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
}
}