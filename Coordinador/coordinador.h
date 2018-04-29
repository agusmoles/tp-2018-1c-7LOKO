#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <signal.h>
#include <pthread.h>
#include "../Colores.h"

char* PUERTO;
#define NUMEROCLIENTES 10

struct Cliente{
	char* nombre;
	int fd;						//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
};

struct arg_struct {
	int socket;
	struct Cliente socketCliente;
};

t_log* logger;
t_config* config;
struct Cliente socketCliente[NUMEROCLIENTES];

void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
void escuchar(int socket);
void aceptarCliente(int socket, struct Cliente* socketCliente);
void recibirMensaje(void* argumentos);
void crearHiloParaCliente(int socket, struct Cliente socketCliente);
int envioHandshake(int socketCliente);
int reciboIdentificacion(int socketCliente);
void intHandler();

