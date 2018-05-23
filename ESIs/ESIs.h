#ifndef ESIS_ESIS_H_
#define ESIS_ESIS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include "../Colores.h"
#include "../sharedlib.h"
#include "parser.h"

char* IPPLANIFICADOR;
char* PUERTOPLANIFICADOR;
char* IPCOORDINADOR;
char* PUERTOCOORDINADOR;
#define PACKAGESIZE 1024

typedef struct Header_Operacion {
	int id_operacion; //GET: 0 , SET: 1, STORE: 2
	int sentencia_tam;
}header_operacion;

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
void administrarID(int socketPlanificador, int socketCoordinador);


void recibirMensaje(int socketServidor,char* mensaje);
void enviarMensaje(int socketServidor, void* instruccion);


void ejecutarInstruccion(char*,int,int);

int cantidadSentencias(FILE*);  //Cuenta cantidad de sentencias que tiene el script

#endif /* ESIS_ESIS_H_ */
