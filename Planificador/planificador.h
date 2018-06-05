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
#include <commons/collections/dictionary.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "../Colores.h"
#include "sharedlib.h"

char* PUERTOPLANIFICADOR;
char* PUERTOCOORDINADOR;
char* IPCOORDINADOR;
char* algoritmoPlanificacion;
int listenSocket;
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
	float tasaDeRespuesta;
	int tiempoDeEspera;
}cliente;

t_log* logger;
t_config* config;
t_list* listos;
t_list* ejecutando;
t_list* finalizados;
t_dictionary* bloqueados;
t_dictionary* diccionarioClaves;
sem_t mutexListos;
sem_t mutexEjecutando;
sem_t mutexBloqueados;
sem_t mutexFinalizados;
fd_set descriptoresLectura;
cliente socketCliente[NUMEROCLIENTES];		//ARRAY DE ESTRUCTURA CLIENTE
int fdmax = 10;
char* ESIABuscarEnDiccionario;

void configurarLogger();
void crearConfig();
void crearDiccionarioDeClaves();
void crearDiccionarioDeESIsBloqueados();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
int conectarSocketCoordinador();
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void conectarConCoordinador();
void eliminarClavesTomadasPorEsiFinalizado(char* clave, void* ESI);
cliente* buscarESI(int* IDESI);
void abortarESI(int* IDESI, char* nombreESI);
void bloquearESI(char* clave, char* nombreESI);
void escuchar(int socket);
int envioHandshake(int socketCliente);
int envioIDDeESI(int socketCliente, int identificador);
void manejoDeClientes(int socket, cliente* socketCliente);
void aceptarCliente(int socket, cliente* socketCliente);
void recibirHeader(int socket, header_t* header);
void recibirClave(int socket, int tamanioClave, char* clave);
void recibirIDDeESI(int socket, int* ID);
void recibirMensaje(cliente* socketCliente, int posicion);
void ordenarProximoAEjecutar();
cliente* getPrimerESIListo();
void ordenarColaDeListosPorSJF(cliente* ESIEjecutando);
void ordenarColaDeListosPorHRRN(cliente* ESIEjecutando);
void enviarOrdenDeEjecucion(cliente* esiProximoAEjecutar, char* ordenEjecucion);
int comparadorRafaga(cliente* cliente, struct Cliente* cliente2);
int comparadorResponseRatio(cliente* cliente, struct Cliente* cliente2);
void sumarUnoAlWaitingTime(cliente* cliente);
void calcularResponseRatio(cliente* cliente);
void _exit_with_error(char* mensaje);
