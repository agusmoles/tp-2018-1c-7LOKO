#ifndef ESIS_ESIS_H_
#define ESIS_ESIS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include "../Colores.h"
#include "parser.h"

char* IPPLANIFICADOR;
char* PUERTOPLANIFICADOR;
char* IPCOORDINADOR;
char* PUERTOCOORDINADOR;
#define PACKAGESIZE 1024


t_log * logger;
t_config* config;

void configure_logger();
void crearConfig();
void setearConfigEnVariables();
void exitError(int socket,char* error_msg);
void exitErrorBuffer(int socket, char* error_msg, char* buffer);

void verificarResultado(int,int);

int conect_to_server(char *ip,char*puerto);
void recibirHandshake(int socketServidor, char* handshake);
void envioIdentificador(int socket);


void recibirOrdenDeEjecucion(int socketServidor);
void enviarMensaje(int socketServidor, char* instruccion);


void ejecutarInstruccion(char*,int,int);

void instruccionGet(t_esi_operacion*,int,int);
void instruccionSet(t_esi_operacion*,int,int);
void instruccionStore(t_esi_operacion*,int,int);



#endif /* ESIS_ESIS_H_ */
