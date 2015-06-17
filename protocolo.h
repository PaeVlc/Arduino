/* 
 * File:   protocolo.h
 * Author: alejandro
 *
 * Created on 10 de marzo de 2015, 12:36
 */

#ifndef PROTOCOLO_H
#define	PROTOCOLO_H
#define MAX_BUFFER 1024
#define SIZE_DATA 8
#define SIZE_TRAMA_ENVIO 13
#define SIZE_ID 6
#define SIZE_CRC_REC 3
#define SIZE_LENGTH 18

typedef struct{
    
    char ack_envio;
    char ack_recibido;
    char fin_recibido;
    int crc_buff[SIZE_CRC_REC];
    char id[SIZE_ID];
    int envio;
    char crc; // si crc = 1 esta bien si crc = 0 esta mal,.
    char fin_envio;
    //int bufferRecibido[MAX_BUFFER]; // buffer para lo que recibamos de la camara
    //int size_buffer_recibido;  // tama?o del buffer de datos recibidos
    char bufferDatos[MAX_BUFFER];    // buffer para lo que qeramos enviar
    int size_buffer_datos; //tama?o del buffer de datos para enviar
    //int trama[SIZE_TRAMA_ENVIO];
    int firstSend; // cuando es uno es que aun n se ha enviado la primera trama
    int preambulo; // 1 bien 0 mal
    
}pupload;

void crear_trama(pupload *proto,char *trama);
void getVars(pupload *proto, char received[], char datos[]);  // CANVI
char getACK(char received[]);
char getFIN(char received[]);
void save_data(int received[], pupload *proto); //guarda los bits de lo que recibimos de la camara
void save_to_send(char *sending, pupload* proto);  // guarda los bits en un buffer para enviarlos.
void crear_trama_fin(pupload *proto,char *trama); 
void eliminar_trama(pupload *proto); //elimina la ultima trama del buffer.
int p_envio(pupload *proto, char *trama);
void p_recibo(pupload *proto, char received[], char *dataReceived);
char crearCRC(char datos[]); //crear CRC a partir de la paridad de los datos
int getLength(char received[]); // Sacar la longitud de la trama de la estacion base
char comprobarCRC(pupload *proto,char received[]);
void preambulo(char *trama, pupload *proto);


void save_to_send(char *sending, pupload *proto){
    int i;
    for( i= 0; i < SIZE_DATA; i++){
        proto->bufferDatos[proto->size_buffer_datos+i] = sending[i];
    }
    proto->size_buffer_datos = proto->size_buffer_datos+SIZE_DATA;
}

void crear_trama(pupload *proto, char *trama){
    char sub[13] = "0", crc;
    int i, d = 0;
    sub[0] = proto->ack_envio;
    sub[1] = proto->fin_envio;
    crc = crearCRC(proto->bufferDatos);
    strncat(sub, proto->bufferDatos, SIZE_DATA);
    for(i=10; i < SIZE_TRAMA_ENVIO;i++){
        sub[i] = crc;
        d++;
    }
    strncpy(trama, sub, SIZE_TRAMA_ENVIO); 
}

void eliminar_trama(pupload *proto){
    int i;
    for(i =0; i < SIZE_DATA; i++){
        proto->bufferDatos[i] = proto->bufferDatos[i+SIZE_DATA];
    }
    proto->size_buffer_datos = proto->size_buffer_datos-SIZE_DATA;
}

int p_envio(pupload *proto, char *trama){      
    // Esto hay que cambiarlo
    if(proto->size_buffer_datos > 0 ){                       // Hay información en el buffer
    if (proto->ack_recibido == '1' ) proto->firstSend = 0;   // Si ACK recibido es correcto --> ENVIAMOS TRAMA
      proto->fin_envio = '0';                                // Trama
    }else{
      proto->fin_envio = '1';                                // Fin de transmision
    }
    crear_trama(proto, trama);
    proto->envio = 1;
}

char crearCRC(char datos[]){
    int i, j = 0;
    char crc;
    for(i = 0; i < SIZE_DATA + 2; i++){
        if(datos[i] == '1'){
            j++;
        }
    }
    if(j%2 > 0){
        crc = '0';
    }
    else crc = '1';
    return crc;
}

   
void p_recibo(pupload *proto, char received[], char dataReceived[]){
    int crc, i;
    if(proto->preambulo == 0) proto->ack_envio = '0';
    else{
      proto->ack_recibido = getACK(received);  
      proto->fin_recibido = getFIN(received);
      switch(proto->fin_recibido){
        case '0':
          getVars(proto, received, dataReceived);
          proto->crc = comprobarCRC(proto, received);
          if(proto->crc != '1'){ proto->ack_envio = '0'; break;}
        case '1':
          if (proto->ack_recibido == '1'){
              if (proto->firstSend == 0) eliminar_trama(proto);//borra los 8 ultimos de buffer porque no es primera trama
              else proto->firstSend = 0; 
              proto->ack_envio = '1'; 
          }
          
      }
    }
    proto->envio = 0;
}

int getLength(char received[]){
    int i, j = 0, fin = 0, length;
    for(i = 2 + SIZE_ID; i < 2 + SIZE_ID + SIZE_LENGTH; i++){
        length = received[i] - '0';
        fin = fin + (length << (26-i))/2;
    }
    return fin;
}

char comprobarCRC(pupload *proto,char received[]){
  int suma = 0, i, crc=0, cont = 0, result;
  suma = proto->crc_buff[0] + proto->crc_buff[1] + proto->crc_buff[2]; // Suma el valor del CRC. Si aquest ?s 3 vol dir "111". Altrament "000".
  if (suma == 3) crc = 1;
  for (i = 0; i < 26 + getLength(received); i++){ //crc 1 quiere decir par
    if (received[i] == '1') cont++;                 //crc 0 quiere decir impar
  }
  if (((cont % 2 == 0) && (crc == 1)) || ((cont % 2 != 0) && (crc == 0))) result = '1';
  else result = '0';
  return result;
}

char getACK(char received[]){
    return received[0];
}
char getFIN(char received[]){
    return received[1];
}

void getVars(pupload *proto, char received[], char datos[]){ // coger id y crc
    int i, j = 0, length;
    length = getLength(received);
    for(i= 2 + SIZE_ID + SIZE_LENGTH; i <= 2 + SIZE_ID + SIZE_LENGTH + length - 1 ; i++){
      datos[j]=received[i]; 
      j++;
    }
    strncpy(proto->id, received + 2, 6);
    //strncpy(datos, received + 2 + SIZE_ID + SIZE_LENGTH, length);
    for(i = 2 + SIZE_ID + SIZE_LENGTH +length; i< 26+length+SIZE_CRC_REC; i++){
       proto->crc_buff[j] = received[i] -'0';
       j++;
    }
}
#endif	/* PROTOCOLO_H */
