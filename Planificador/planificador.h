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

char* PUERTOPLANIFICADOR;
char* PUERTOCOORDINADOR;
char* IPCOORDINADOR;
char* algoritmoPlanificacion;
double alfaPlanificacion;
int estimacionInicial;
#define NUMEROCLIENTES 20

struct Cliente{
	char nombre[14];
	int fd;						//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
};

t_log* logger;
t_config* config;
t_list* estado;
t_list* listos;
t_list* ejecutando;
t_list* finalizados;
t_list* bloqueados;
fd_set descriptoresLectura;
struct Cliente socketCliente[NUMEROCLIENTES];		//ARRAY DE ESTRUCTURA CLIENTE
int fdmax = NUMEROCLIENTES;

void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
int conectarSocketCoordinador();
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void conectarConCoordinador();
void escuchar(int socket);
void manejoDeClientes(int socket, struct Cliente* socketCliente);
void aceptarCliente(int socket, struct Cliente* socketCliente);
void recibirMensaje(int socket, struct Cliente* socketCliente, int posicion);
int envioHandshake(int socketCliente);
void _exit_with_error(int socket, char* mensaje);
