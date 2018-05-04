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
#define NUMEROCLIENTES 20

typedef struct Cliente{
	char nombre[14];
	int fd;						//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
	int identificadorESI;
}cliente;

struct arg_struct {
	int socket;
	cliente socketCliente;
};

t_log* logger;
t_config* config;
cliente socketCliente[NUMEROCLIENTES];

void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
void escuchar(int socket);
void asignarNombreAlSocketCliente(cliente* socketCliente, char* nombre);
void enviarMensaje(int socketCliente, char* msg);
void recibirSentenciaESI(int socketCliente);
void recibirIDdeESI(cliente* cliente);
void aceptarCliente(int socket, cliente* socketCliente);
void recibirMensaje(void* argumentos);
void crearHiloPlanificador(cliente socketCliente);
void crearHiloInstancia(cliente socketCliente);
void crearHiloESI(cliente socketCliente);
int envioHandshake(int socketCliente);
int reciboIdentificacion(int socketCliente);
void intHandler();

