#ifndef ESIS_ESIS_H_
#define ESIS_ESIS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "commons/log.h"
#include "../Colores.h"

#define IPPLANIFICADOR "127.0.0.1"
#define PUERTOPLANIFICADOR "6666"
#define IPCOORDINADOR "127.0.0.1"
#define PUERTOCOORDINADOR "6667"
#define PACKAGESIZE 1024

t_log * logger;

void configure_logger();
void exitError(int socket,char* error_msg);
void exitErrorBuffer(int socket, char* error_msg, char* buffer);

void verificarResultado(int,int);

int conect_to_server(char *ip,char*puerto);
void recibirHandshake(int socketServidor, char* handshake);
void envioIdentificador(int socket);


void recibirOrdenDeEjecucion(int socketServidor);
void pedirRecursos(int socketServidor, char* instruccion);



#endif /* ESIS_ESIS_H_ */
