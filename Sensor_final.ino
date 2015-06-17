#define F_CPU 1000000UL
#include <avr/io.h>
#include <string.h>
#include "protocolo.h"

#define PACKET_LENGTH 80
#define PACKET_SEND 14
#define ID_LENGTH 6
#define LENGTH 18
#define CRC_LENGTH 3
#define ID_ORIGIN "000001"
pupload proto = {'1', '1', '0', {0}, {'0'}, 0, '1', '0', {'0'}, 0, 0, 1};

int i, pulse[PACKET_LENGTH], counter = 0, valor = 0;
String info, datos;
unsigned long duration;
boolean voltage = true, start = true; //voltage = true --> 1 | voltage = false --> 0
char sensor[9];

void setup() {
  Serial.begin(9600);
  pinMode(12, OUTPUT);   // Fotoreflector. PIN12 for NANO
  pinMode(2,INPUT);      // Temperature sensor
  preambleUP();
  // Interruptions
  attachInterrupt(1, change, CHANGE);  // A1 = PIN3
  
}


void loop() {
  // RECEPTION
  //ReadPacket(datos);
  // TRANSMISION
  if(proto.envio == 0){
    readInfo(sensor);
    sendPacket(sensor);
  }
}

// INTERRUPT -> SCAN INPUT VOLTAGE
void change() {
  //  DOWNLINK PREAMBLE = 01010111
  if(pulseIn(3,HIGH) < 80) counter++;           // Detects 0101
  if (counter > 2) {
    if ((voltage && start)) {
      if ((pulseIn(3, HIGH)) > 165) info = "0"; // If ACK=0 --> 111 10 xxx... pulseIn > 150
      voltage = false;
      start = false;
    } else if (voltage) {
      // Save INPUT pulse duration.
      duration = pulseIn(3, HIGH);
      info = info + "1";
      voltage = false;
    } else {
      duration = pulseIn(3, LOW);
      info = info + "0";
      voltage = true;
    }
    pulse[i] = duration;
    i++;
  }
  if (i >= PACKET_LENGTH) readVoltage(pulse, info);
}

// Function to review. Read the values obtained from the A0 pin and defines the package received
void readVoltage(int pulse[], String info) {
  noInterrupts(); // Disable Interrupts
  for (int j = 0; j < PACKET_LENGTH; j++) {
    // The length of a bit is 50ms approx. We check to see if the duration is a bit or two
    if ((pulse[j] <= 120) && (pulse[j] >= 80)) datos = datos + info[j] + info[j];
    else if ((pulse[j] > 120) && (pulse[j] < 30)) return; // Check Error
    else datos = datos + info[j];
  }
  // Clean global variables
  info = "";
  memset(pulse, 0, sizeof(pulse));
  ReadPacket(datos);
  i = 0;
}

void ReadPacket(String datos) {
  String data;
  // Manchester code translation
  for (int j = 0; j < datos.length() + 1 ; j++) {
    if ((datos[j] == '1') && (datos[j + 1] == '0')) data = data + '0';
    else if ((datos[j] == '0') && (datos[j + 1] == '1')) data = data + '1' ;
    j++;
  }
  // Data extraction using protocol's function p_recibo
  char prova[data.length() + 1];
  data.toCharArray(prova, sizeof(prova));
  char dataExtract[getLength(prova)];
  p_recibo(&proto, prova, dataExtract);
  BinarytoASCII(dataExtract);
  datos = "";
}

// Transforming binary information to ASCII code
void BinarytoASCII(char binaryText[]) {
  byte textBuf[ 16 ], *text;
  char *bT = binaryText;
  int counter = 0;
  text = textBuf;
  *text = 0;
  while (*bT) { // While BT is not null
    if (counter < 8) { // Counter to get bytes. Because we do not put spaces (* bT! = "") Plays because they hold.
      (*text) *= 2;
      if (*bT == '1')  (*text)++;
      counter++;
    } else { // Once we crossed the byte buffer touches restart. Let's value back to BT that jumps around with count = 9. If we do not we eat one bit each cycle.
      text++;
      *text = 0;
      counter=0;
      bT--;
    }
    bT++;
  }
  Serial.print((char *) textBuf );
  Serial.println("");
  interrupts(); // Ya s'ha llegit i mostrat per pantalla. Retornem a activa les interrupcion
}

// Read information from the sensor temperature and humidity
void readInfo(char sensor[]) {
  valor = analogRead(A5);
  String str;
  if (valor >= 100) str = "01011100", Serial.print("Detection --> "); // --> D = Detection
  else  str = "00011101", Serial.print("No detection --> "); // ---> N = No detection
  str.toCharArray(sensor, str.length()+1);
}

// Prepares and sends package
void sendPacket(char buf[]) {
  char paquet_to_send[PACKET_SEND];
  save_to_send(buf, &proto);
  p_envio(&proto, paquet_to_send);
  for (i = 0; i < PACKET_SEND; i++) {
    if ((paquet_to_send[i - 1] == '1') || (i < 1)) sendOne();
    else sendZero();
  }
  Serial.println("");
  // Possible inicialització de TimeOut
  start = true;
  voltage = true;
  //eliminar_trama(&proto);
  timeOut(start, &proto);
}

//62.5 -> 16bits/s    80 --> 12,5 bits/s
void sendOne() {
  digitalWrite(12, LOW);
  delay(100);
  Serial.print('1');
}

void sendZero() {
  digitalWrite(12, HIGH);
  delay(100);
  Serial.print('0');
}

void timeOut(boolean start, pupload *proto) {
  unsigned long time = millis(); // Temps inici
  int tout = 3000; //milisegundos
  while (((millis() - time) < tout) && voltage);
  if (voltage) sendPacket(proto->bufferDatos); // Salto del paquete por Error
}

void preambleUP() {
  for (i = 0; i < 30; i++) {
    sendOne();
    sendZero();
  }
  Serial.println("");
}
