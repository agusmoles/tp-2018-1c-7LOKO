#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <signal.h>
#include "../../Colores.h"

char* PUERTO;
#define NUMEROCLIENTES 10

struct Cliente{
	char* nombre;
	int fd;						//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
};

t_log* logger;
t_config* config;
fd_set descriptoresLectura;
int fdmax = NUMEROCLIENTES;

void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
void escuchar(int socket);
void manejoDeClientes(int socket, struct Cliente* socketCliente);
void aceptarCliente(int socket, struct Cliente* socketCliente);
void recibirMensaje(int socket, struct Cliente* socketCliente, int posicion);
int envioHandshake(int socketCliente);
void _exit_with_error(int socket, struct Cliente* socketCliente, char* mensaje);
