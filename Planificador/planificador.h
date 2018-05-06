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

typedef struct Cliente{
	char nombre[14];
	int fd;						//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
	int identificadorESI;
	int rafagaActual;
	float estimacionRafagaActual;
	float estimacionProximaRafaga;
}cliente;

t_log* logger;
t_config* config;
t_list* listos;
t_list* ejecutando;
t_list* finalizados;
t_list* bloqueados;
fd_set descriptoresLectura;
cliente socketCliente[NUMEROCLIENTES];		//ARRAY DE ESTRUCTURA CLIENTE
int fdmax = 10;

void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
int conectarSocketCoordinador();
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void conectarConCoordinador();
void escuchar(int socket);
int envioHandshake(int socketCliente);
int envioIDDeESI(int socketCliente, int identificador);
void manejoDeClientes(int socket, cliente* socketCliente);
void aceptarCliente(int socket, cliente* socketCliente);
void recibirMensaje(int socket, cliente* socketCliente, int posicion);
void ordenarProximoAEjecutar(int socket, cliente* socketCliente);
cliente* getESIProximoAEJecutar();
int ordenarColaDeListos(cliente* cliente);
int comparador(cliente* cliente, struct Cliente* cliente2);
void _exit_with_error(int socket, char* mensaje);
